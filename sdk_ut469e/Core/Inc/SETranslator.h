/*=============================================================================
	SETranslator.h: Crash handler windows-specific code.
	Copyright 2021-2024 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Buggie.
	
	TODO:
		* Add Windows 64-bit support.
=============================================================================*/

#pragma once

#define USE_DBGHELP

#include <windows.h>
#include <eh.h>
#include "Core.h"

#ifndef MAX_SYM_NAME
#define MAX_SYM_NAME 2000
#endif

#if BUILD_64

inline void SetSETranslator()
{
}

inline void MaybeBackTrace()
{	
}

#else

#include "FOutputDeviceFile.h"

#ifdef USE_DBGHELP
#include <dbghelp.h>

// Typedefs for the function signatures
typedef BOOL(WINAPI* SymCleanupFunc)(HANDLE hProcess);
typedef BOOL(WINAPI* SymFromAddrWFunc)(HANDLE hProcess, DWORD64 qwAddr, PDWORD64 pdwDisplacement, PSYMBOL_INFOW pSymInfo);
typedef BOOL(WINAPI* SymFromAddrFunc)(HANDLE hProcess, DWORD64 qwAddr, PDWORD64 pdwDisplacement, PSYMBOL_INFO pSymInfo);
typedef BOOL(WINAPI* SymGetLineFromAddrW64Func)(HANDLE hProcess, DWORD64 qwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line);
typedef BOOL(WINAPI* SymGetLineFromAddr64Func)(HANDLE hProcess, DWORD64 qwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line);
typedef BOOL(WINAPI* SymInitializeFunc)(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);
typedef BOOL(WINAPI* StackWalk64Func)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddressRoutine);

#endif // USE_DBGHELP

static _se_translator_function PrevSETranslator = NULL;
static TCHAR GSEError[65536] = {};
static TCHAR PrintBuffer[4096] = {};

// Safe version of debugf, for avoid unsafe additional memory allocations inside GLog->Logf
#define debugfSafe(Type, ...) if (GLog) \
	{ \
		appSnprintf(PrintBuffer, ARRAY_COUNT(PrintBuffer) - 1, __VA_ARGS__); \
		PrintBuffer[ARRAY_COUNT(PrintBuffer) - 1] = TEXT('\0'); \
		GLog->Log(Type, PrintBuffer); \
	}

INT FileSize(const TCHAR* name)
{
	HANDLE hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
	{
		CloseHandle(hFile);
		return -1;
	}

	CloseHandle(hFile);
	return size.QuadPart;
}

void FlushGLog()
{
	if (!GLog)
		return;
	FOutputDeviceFile* OutDevFile = dynamic_cast<FOutputDeviceFile*>(GLog);
	if (OutDevFile)
		OutDevFile->Flush();
}

#define UNUSED_CHARS_COUNT(CharBuffer) (ARRAY_COUNT(CharBuffer) - Len - 1)

void GetModuleInfo(HINSTANCE hModule, INT& ModuleSize, TCHAR*& moduleName, TCHAR* moduleBuffer, INT moduleBufferSize)
{
	if (hModule) 
	{
		if (GetModuleFileNameW(hModule, moduleBuffer, moduleBufferSize))
		{
			ModuleSize = FileSize(moduleBuffer);
			moduleName = moduleBuffer;
			TCHAR* LastPart = moduleBuffer;
			while ((LastPart = appStrstr(moduleName, PATH_SEPARATOR)) != NULL)
				moduleName = LastPart + 1;
		}
		else
		{
			INT Error = appGetSystemErrorCode();
			debugfSafe(NAME_Warning, TEXT("GetModuleFileNameW failed to resolve %p: %ls (%i)"), hModule, appGetSystemErrorMessage(Error), Error);
			FlushGLog();
		}
	}
}

void EnumerateExports(HMODULE hModule, TCHAR* moduleName, BYTE* FramePointer, INT& FunctionAddress, INT& FunctionOffset, TCHAR* functionName, INT functionNameSize) {
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);

	if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
	{
		debugfSafe(NAME_Warning, TEXT("No exports found for %ls at %p"), moduleName, hModule);
		FlushGLog();
		return;
	}

	DWORD exportDirRVA = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportDirRVA);

	DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
	WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);
	DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);

	INT Best = -1;
	INT BestDist = 0;
	for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
		DWORD funcRVA = pFunctions[pOrdinals[i]];
		BYTE* funcAddress = (BYTE*)hModule + funcRVA;
		if (funcAddress <= FramePointer && 
			(Best < 0 || BestDist > FramePointer - funcAddress))
		{
			Best = i;
			BestDist = FramePointer - funcAddress;
		}
	}
	if (Best < 0)
	{
		FunctionAddress = 0;
		FunctionOffset = FramePointer - (BYTE*)hModule;
		debugfSafe(NAME_Warning, TEXT("No good exports found for %ls at %p for %p"), moduleName, hModule, FramePointer);
		FlushGLog();
		return;
	}

	FunctionAddress = pFunctions[pOrdinals[Best]];
	FunctionOffset = BestDist;
	const ANSICHAR* pName = (const ANSICHAR*)((BYTE*)hModule + pNames[Best]);
	const TCHAR* pNameW = appFromAnsi(pName);
	if (pNameW)
		appStrncpy(functionName, pNameW, functionNameSize);
}

#ifdef USE_DBGHELP

struct FDbgHelpInfo
{
	HMODULE hDbgHelp;
	UBOOL bInit;

	SymInitializeFunc SymInitialize;
	StackWalk64Func StackWalk64;
	SymCleanupFunc SymCleanup;
	// Can be NULL
	PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64;
	PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64;
	SymGetLineFromAddrW64Func SymGetLineFromAddrW64;
	SymGetLineFromAddr64Func SymGetLineFromAddr64;
	SymFromAddrWFunc SymFromAddrW;
	SymFromAddrFunc SymFromAddr;
} DbgHelpInfo = {};

UBOOL LoadDbgHelp()
{
	if (DbgHelpInfo.hDbgHelp)
		return TRUE;
	// Load dbghelp.dll dynamically
	DbgHelpInfo.hDbgHelp = LoadLibrary(TEXT("dbghelp.dll"));
	if (!DbgHelpInfo.hDbgHelp) {
		INT Error = appGetSystemErrorCode();
		debugfSafe(NAME_Warning, TEXT("Failed to load dbghelp.dll: %ls (%i)"), appGetSystemErrorMessage(Error), Error);
		FlushGLog();
		return FALSE;
	}

	#define GetFuncAddr(Type, Name) DbgHelpInfo.Name = (Type)GetProcAddress(DbgHelpInfo.hDbgHelp, #Name); \
	if (!DbgHelpInfo.Name) \
	{ \
		INT Error = appGetSystemErrorCode(); \
		debugfSafe(NAME_Warning, TEXT("%d. Failed to resolve ") #Name TEXT(" from dbghelp.dll: %ls (%i)"), ++Num, appGetSystemErrorMessage(Error), Error); \
		FlushGLog(); \
	}

	// Get the function addresses
	INT Num = 0;
	GetFuncAddr(SymInitializeFunc, SymInitialize);
	GetFuncAddr(StackWalk64Func, StackWalk64);
	GetFuncAddr(SymCleanupFunc, SymCleanup);
	// Can be NULL
	GetFuncAddr(PFUNCTION_TABLE_ACCESS_ROUTINE64, SymFunctionTableAccess64);
	GetFuncAddr(PGET_MODULE_BASE_ROUTINE64, SymGetModuleBase64);
	GetFuncAddr(SymGetLineFromAddrW64Func, SymGetLineFromAddrW64);
	GetFuncAddr(SymGetLineFromAddr64Func, SymGetLineFromAddr64);
	GetFuncAddr(SymFromAddrWFunc, SymFromAddrW);
	GetFuncAddr(SymFromAddrFunc, SymFromAddr);

#if 0 // Check for debug
	DbgHelpInfo.SymFunctionTableAccess64 = NULL;
	DbgHelpInfo.SymGetModuleBase64 = NULL;
	DbgHelpInfo.SymGetLineFromAddrW64 = NULL;
	DbgHelpInfo.SymGetLineFromAddr64 = NULL;
	DbgHelpInfo.SymFromAddrW = NULL;
	DbgHelpInfo.SymFromAddr = NULL;
#endif

	if (!DbgHelpInfo.SymInitialize || !DbgHelpInfo.SymCleanup || !DbgHelpInfo.StackWalk64)
	{
		FreeLibrary(DbgHelpInfo.hDbgHelp);
		DbgHelpInfo.hDbgHelp = NULL;
		return FALSE;
	}

	if (!DbgHelpInfo.bInit)
	{
		HANDLE process = GetCurrentProcess();

		// Initialize DbgHelp
		if (!DbgHelpInfo.SymInitialize(process, nullptr, TRUE))
		{
			INT Error = appGetSystemErrorCode();
			debugfSafe(NAME_Warning, TEXT("SymInitialize failed: %ls (%i)"), appGetSystemErrorMessage(Error), Error);
			FlushGLog();
			return FALSE;
		}
		DbgHelpInfo.bInit = TRUE;
	}

	return TRUE;
}

#define SymInitialize DbgHelpInfo.SymInitialize
#define StackWalk64 DbgHelpInfo.StackWalk64
#define SymCleanup DbgHelpInfo.SymCleanup

#define SymFunctionTableAccess64 DbgHelpInfo.SymFunctionTableAccess64
#define SymGetModuleBase64 DbgHelpInfo.SymGetModuleBase64
#define SymGetLineFromAddrW64 DbgHelpInfo.SymGetLineFromAddrW64
#define SymGetLineFromAddr64 DbgHelpInfo.SymGetLineFromAddr64
#define SymFromAddrW DbgHelpInfo.SymFromAddrW
#define SymFromAddr DbgHelpInfo.SymFromAddr

void DumpFrameAdvanced(HANDLE process, STACKFRAME64& stackFrame, INT& i)
{
	// Get module base
	DWORD64 moduleBase = !SymGetModuleBase64 ? 0 : SymGetModuleBase64(process, stackFrame.AddrPC.Offset);
	TCHAR moduleBuffer[MAX_PATH] = {};
	TCHAR* moduleName = TEXT("Unknown");
	INT ModuleSize = -1;
	GetModuleInfo((HINSTANCE)moduleBase, ModuleSize, moduleName, moduleBuffer, ARRAY_COUNT(moduleBuffer));

	// Get function name and offset
	TCHAR SymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
	SYMBOL_INFOW* SymbolW = (SYMBOL_INFOW*)SymbolBuffer;
	SYMBOL_INFO* Symbol = (SYMBOL_INFO*)SymbolBuffer;
	if (SymFromAddrW)
	{
		SymbolW->SizeOfStruct = sizeof(SYMBOL_INFOW);
		SymbolW->MaxNameLen = MAX_SYM_NAME;
	}
	else
	{
		Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		Symbol->MaxNameLen = MAX_SYM_NAME;
	}

	DWORD64 displacement = 0;
	const TCHAR* functionName = SymbolW->Name;
	INT FunctionAddress = 0;
	if (SymFromAddrW && SymFromAddrW(process, stackFrame.AddrPC.Offset, &displacement, SymbolW))
	{
		functionName = SymbolW->Name;
		FunctionAddress = SymbolW->Address - moduleBase;
	}
	else if (!SymFromAddrW && SymFromAddr && SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, Symbol))
	{
		functionName = appFromAnsi(Symbol->Name);
		FunctionAddress = Symbol->Address - moduleBase;
	}
	else
		appSnprintf(SymbolW->Name, MAX_SYM_NAME, TEXT("Unknown_%08x"), stackFrame.AddrPC.Offset - moduleBase);

	// Get source file and line number
	IMAGEHLP_LINEW64 LineInfoW = {};
	LineInfoW.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);
	IMAGEHLP_LINE64 LineInfo = {};
	LineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD lineDisplacement = 0;
	TCHAR LineNumber[4096] = {};
	INT Len = 0;
	appSnprintf(LineNumber, ARRAY_COUNT(LineNumber), TEXT("+0x%X"), (INT)displacement);
	if (SymGetLineFromAddrW64 && SymGetLineFromAddrW64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &LineInfoW))
	{
		Len = appStrlen(LineNumber);
		if (UNUSED_CHARS_COUNT(LineNumber) > 0)
			appSnprintf(&LineNumber[Len], UNUSED_CHARS_COUNT(LineNumber), TEXT("\n\t%ls Line %d"), LineInfoW.FileName, LineInfoW.LineNumber);
	}
	else if (!SymGetLineFromAddrW64 && SymGetLineFromAddr64 && SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &LineInfo))
	{
		Len = appStrlen(LineNumber);
		if (UNUSED_CHARS_COUNT(LineNumber) > 0)
			appSnprintf(&LineNumber[Len], UNUSED_CHARS_COUNT(LineNumber), TEXT("\n\t%ls Line %d"), appFromAnsi(LineInfo.FileName), LineInfo.LineNumber);
	}
	if (lineDisplacement != 0)
	{
		Len = appStrlen(LineNumber);
		if (UNUSED_CHARS_COUNT(LineNumber) > 0)
			appSnprintf(&LineNumber[Len], UNUSED_CHARS_COUNT(LineNumber), TEXT(" +0x%X"), lineDisplacement);
	}
	if (stackFrame.AddrPC.Mode == AddrModeFlat)
	{
		Len = appStrlen(LineNumber);
		if (UNUSED_CHARS_COUNT(LineNumber) > 0)
			appSnprintf(&LineNumber[Len], UNUSED_CHARS_COUNT(LineNumber), TEXT("\n\tPossible params: %lld (%llX), %lld (%llX), %lld (%llX), %lld (%llX)"), 
				stackFrame.Params[0], stackFrame.Params[0],
				stackFrame.Params[1], stackFrame.Params[1],
				stackFrame.Params[2], stackFrame.Params[2],
				stackFrame.Params[3], stackFrame.Params[3]);
	}

	Len = appStrlen(GSEError);
	if (UNUSED_CHARS_COUNT(GSEError) > 0)
		appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("%d. %p: %ls(%d bytes)!%ls @ %X %ls\n"), 
			i++, (void*)stackFrame.AddrPC.Offset, moduleName, ModuleSize, functionName, FunctionAddress, LineNumber);
}

#endif // USE_DBGHELP

const TCHAR* DumpSimpleBackTrace(void** stack, INT frames, void* InAllocationBase = NULL, EXCEPTION_POINTERS* data = NULL)
{
#ifdef USE_DBGHELP
	HANDLE process = GetCurrentProcess();
	STACKFRAME64 stackFrame = {};
	if (!data)
		LoadDbgHelp();
#endif;

	INT FirstFrame = 0;
	if (data && !data->ExceptionRecord)
		for (INT i = 1; i < frames && FirstFrame == 0; i++)
			if (stack[i] == stack[0])
				FirstFrame = i;

	INT Len = 0;
	if (!data)
		GSEError[0] = TEXT('\0');
	else
	{
		Len = appStrlen(GSEError);
		if (UNUSED_CHARS_COUNT(GSEError) > 0)
			appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("Simple backtrace (from SEH handler, not from exception point) (most recent call first), with relative to module base %p, if module not found:\n"), InAllocationBase);
	}
	for (INT i = 0; i < frames; i++)
	{
		MEMORY_BASIC_INFORMATION mbi = {};

		void* AllocationBase = InAllocationBase;

		TCHAR moduleBuffer[MAX_PATH] = {};
		TCHAR* moduleName = TEXT("Unknown");
		INT ModuleSize = -1;
		TCHAR functionBuffer[MAX_SYM_NAME] = {};
		TCHAR* functionName = functionBuffer;
		INT FunctionOffset = 0;
		INT FunctionAddress = 0;
		if (VirtualQuery(stack[i], &mbi, sizeof(mbi)))
		{
			HMODULE hModule = (HMODULE)mbi.AllocationBase;
			AllocationBase = mbi.AllocationBase;
			GetModuleInfo(hModule, ModuleSize, moduleName, moduleBuffer, ARRAY_COUNT(moduleBuffer));
			if (hModule && moduleBuffer[0] != TEXT('\0'))
				EnumerateExports(hModule, moduleName, (BYTE*)stack[i], FunctionAddress, FunctionOffset, functionName, ARRAY_COUNT(functionBuffer));
		}
		else
		{
			INT Error = appGetSystemErrorCode();
			debugfSafe(NAME_Warning, TEXT("VirtualQuery failed to resolve %p: %ls (%i)"), stack[i], appGetSystemErrorMessage(Error), Error);
			FlushGLog();
		}
		if (*functionName == TEXT('\0'))
			functionName = TEXT("Unknown");

		UBOOL bNegativeOffset = stack[i] < AllocationBase;
		TCHAR* Sign = bNegativeOffset ? TEXT("-") : TEXT("+");
		Len = appStrlen(GSEError);
		if (UNUSED_CHARS_COUNT(GSEError) > 0)
			appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("%d. %p (%p %ls%X): %ls(%d bytes)!%ls @ %X +0x%X\n"), 
				i, stack[i], AllocationBase, Sign, bNegativeOffset ? (DWORD)AllocationBase - (DWORD)stack[i] : (DWORD)stack[i] - (DWORD)AllocationBase,
				moduleName, ModuleSize, functionName, FunctionAddress, FunctionOffset);

		if (!data)
		{
#ifdef USE_DBGHELP
			if (DbgHelpInfo.bInit)
			{
				stackFrame.AddrPC.Offset = (DWORD64)stack[i];
				INT j = i;
				DumpFrameAdvanced(process, stackFrame, j);
			}
#endif;
			continue;
		}
		if (i == 0)
		{
			Len = appStrlen(GSEError);
			if (UNUSED_CHARS_COUNT(GSEError) > 0)
				appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("------------ Exception handler frames start ------------\n"));
		}
		else if ((data->ExceptionRecord && appStrstr(functionName, TEXT("KiUserExceptionDispatcher"))) || i == FirstFrame)
		{
			Len = appStrlen(GSEError);
			if (UNUSED_CHARS_COUNT(GSEError) > 0)
				appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("------------ Exception handler frames end ------------\n"));
		}
	}
	return GSEError;
}

void DumpSimpleBackTrace(EXCEPTION_POINTERS* data, void* InAllocationBase)
{
#if _MSC_VER < 1920
	void* stack[1 + 62] = {}; // for Windows XP must be less from 63 according to MSDN
#else
	void* stack[1 + 255] = {};
#endif
	USHORT frames = CaptureStackBackTrace(0, ARRAY_COUNT(stack) - 1, &stack[1], nullptr) + 1;
	stack[0] = (void*)data->ContextRecord->Eip;

	DumpSimpleBackTrace(stack, frames, InAllocationBase, data);
}

void DumpAdvancedBackTrace(EXCEPTION_POINTERS* data)
{
#ifdef USE_DBGHELP
	if (!LoadDbgHelp() || !DbgHelpInfo.bInit)
		return;

	PCONTEXT context = data->ContextRecord;
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	STACKFRAME64 stackFrame = {};
	DWORD machineType;

#ifdef _M_X64
	machineType = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context->Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context->Rsp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context->Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_IX86
	machineType = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = context->Eip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context->Ebp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context->Esp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

	INT Len = appStrlen(GSEError);
	if (UNUSED_CHARS_COUNT(GSEError) > 0)
		appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("Advanced backtrace (most recent call first):\n"));
	INT i = 0;
	while (StackWalk64(machineType, process, thread, &stackFrame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr) &&
		stackFrame.AddrPC.Offset != 0)
		DumpFrameAdvanced(process, stackFrame, i);

	// Cleanup
	SymCleanup(process);
	DbgHelpInfo.bInit = FALSE;
#endif // USE_DBGHELP
}

UBOOL IsSETranslatorCalled(UBOOL bMakeCalled = FALSE)
{
	static UBOOL bCalled = FALSE;
	if (bMakeCalled)
		bCalled = TRUE;
	return bCalled;
}

void SETranslator(unsigned int code, EXCEPTION_POINTERS* data)
{
	//debugfSafe(NAME_Critical, TEXT("SETranslator: %d, %p"), bCalled, data);
	if (!IsSETranslatorCalled() && data)
	{
		IsSETranslatorCalled(TRUE);
		GSEError[0] = TEXT('\0');
		INT Len = 0;
		if (data->ExceptionRecord)
		{
			Len = appStrlen(GSEError);
			if (UNUSED_CHARS_COUNT(GSEError) > 0)
				appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT("%lsSE(%X) @ %08X\nfl=%x rec=%x n=%i"),
					appStrlen(GErrorHist) == 0 ? TEXT("") : TEXT("\n"),
					code, 
					data->ExceptionRecord->ExceptionAddress,
					data->ExceptionRecord->ExceptionFlags,
					data->ExceptionRecord->ExceptionRecord
					);
			for( DWORD i = 0; i < data->ExceptionRecord->NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
			{
				Len = appStrlen(GSEError);
				if (UNUSED_CHARS_COUNT(GSEError) > 0)
					appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), TEXT(" i[%i]=%x"), i, data->ExceptionRecord->ExceptionInformation[i]);
			}
		}
		CONTEXT context = {};
		if (!data->ContextRecord)
		{
			RtlCaptureContext(&context);
			data->ContextRecord = &context;
		}
		Len = appStrlen(GSEError);
		if (UNUSED_CHARS_COUNT(GSEError) > 0)
			appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), 
				TEXT("\neax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x eip=%08x")
				TEXT("\nebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n"), 
				data->ContextRecord->Eax, data->ContextRecord->Ebx, data->ContextRecord->Ecx,
				data->ContextRecord->Edx, data->ContextRecord->Esi, data->ContextRecord->Edi,
				data->ContextRecord->Eip, data->ContextRecord->Ebp, data->ContextRecord->Esp, 
				data->ContextRecord->SegCs, data->ContextRecord->SegDs,
				data->ContextRecord->SegEs, data->ContextRecord->SegFs, 
				data->ContextRecord->SegGs, data->ContextRecord->EFlags
				);

		MEMORY_BASIC_INFORMATION info;
		TCHAR dll_location[MAX_PATH] = {0};
		HMODULE hdll;

		// Get the base address of the module that holds address
		if (!VirtualQuery((LPCVOID)data->ContextRecord->Eip, &info, sizeof(MEMORY_BASIC_INFORMATION)))
		{
			INT Error = appGetSystemErrorCode();
			debugfSafe(NAME_Warning, TEXT("VirtualQuery failed to resolve %p: %ls (%i)"), (void*)data->ContextRecord->Eip, appGetSystemErrorMessage(Error), Error);
			FlushGLog();
		}

		// MEMORY_BASIC_INFORMATION::AllocationBase corresponds to HMODULE
		hdll = (HMODULE)info.AllocationBase;
 
		// Get the dll filename
		int ret = GetModuleFileNameW(hdll, dll_location, MAX_PATH);

		if (!ret)
		{
			INT Error = appGetSystemErrorCode();
			debugfSafe(NAME_Warning, TEXT("GetModuleFileNameW failed to resolve %p: %ls (%i)"), hdll, appGetSystemErrorMessage(Error), Error);
			FlushGLog();
			dll_location[0] = TEXT('\0');
		}

		Len = appStrlen(GSEError);
		if (UNUSED_CHARS_COUNT(GSEError) > 0)
			appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), 
				TEXT("In module '%ls' (%d bytes) at address %08x\n"),
				dll_location, FileSize(dll_location), data->ContextRecord->Eip - (DWORD)info.AllocationBase
				);
		Len = appStrlen(GSEError);
		if (UNUSED_CHARS_COUNT(GSEError) > 0)
			appSnprintf(&GSEError[Len], UNUSED_CHARS_COUNT(GSEError), 
				TEXT("Engine Version: %d%ls - %ls Preview\n"),
				ENGINE_VERSION, ENGINE_REVISION, appFromAnsi(__DATE__)
				);

#if defined(TEST_BUILD)
		if (data && data->ExceptionRecord)
		{
			Len = appStrlen(GErrorHist);
			if (UNUSED_CHARS_COUNT(GErrorHist) > 0)
				appSnprintf(&GErrorHist[Len], UNUSED_CHARS_COUNT(GErrorHist), TEXT("%ls"), GSEError);
		}
#endif
		
		if (GLog)
			GLog->Log(NAME_Critical, GSEError);
		FlushGLog();

		GIsBacktraceDumped = 1;

		GSEError[0] = TEXT('\0');
		DumpSimpleBackTrace(data, info.AllocationBase);
		if (GSEError[0] != TEXT('\0'))
		{
			if (GLog)
				GLog->Log(NAME_Critical, GSEError);
			FlushGLog();
		}

		GSEError[0] = TEXT('\0');
		DumpAdvancedBackTrace(data);
		if (GSEError[0] != TEXT('\0'))
		{
			if (GLog)
				GLog->Log(NAME_Critical, GSEError);
			FlushGLog();
		}
	}
	if (data && data->ExceptionRecord)
	{
		if (!PrevSETranslator)
			throw code;
		PrevSETranslator(code, data);
	}
}

void SetSETranslator()
{
	guard(SetSETranslator);
	PrevSETranslator = _set_se_translator(SETranslator);
	unguard;
}

inline void MaybeBackTrace()
{
	if (!GIsBacktraceDumped && !IsSETranslatorCalled())
	{
		EXCEPTION_POINTERS PtrData = {};
		EXCEPTION_POINTERS* data = &PtrData;
		CONTEXT context = {};
		RtlCaptureContext(&context);
		data->ContextRecord = &context;
		SETranslator(0, data);
	}
}
#endif

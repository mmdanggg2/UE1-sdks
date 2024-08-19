/*=============================================================================
	UnThreadingBaseWindows.h: Unreal Tournament Threading Support
	Copyright 2021-2022 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

#ifndef _UNTHREADINGWINDOWS_H
#define _UNTHREADINGWINDOWS_H

/*----------------------------------------------------------------------------
	Windows-specific classes
----------------------------------------------------------------------------*/

class FCriticalSectionWin : public FCriticalSection
{
	CRITICAL_SECTION CriticalSection;

public:
	FCriticalSectionWin();
	virtual ~FCriticalSectionWin();
	virtual void Lock();
	virtual void Unlock();
};

class FEventWin : public FEvent
{
	HANDLE Event;
	
public:
	FEventWin();
	virtual ~FEventWin();
	virtual void Lock();
	virtual void Unlock();
	virtual UBOOL Create(UBOOL bIsManualReset = FALSE);
	virtual void Trigger();
	virtual void Reset();
	virtual void Pulse();
	virtual UBOOL Wait(DWORD WaitTime = INFINITE);
};

class CORE_API FSynchronizationFactoryWin : public FSynchronizationFactory
{
public:
	FSynchronizationFactoryWin();
	virtual FCriticalSection* CreateCriticalSection();
	virtual FEvent* CreateSynchEvent(UBOOL bIsManualReset = FALSE);
	virtual void Destroy(FSynchronize* SyncObject);
};

class FRunnableThreadWin : public FRunnableThread
{
	HANDLE Thread{};
	FRunnable* Runnable{};
	UBOOL bShouldDeleteSelf{};
	UBOOL bShouldDeleteRunnable{};
	static DWORD STDCALL ThreadProc(LPVOID Params);
	DWORD Run();

public:
	FRunnableThreadWin();
	~FRunnableThreadWin();
	UBOOL Create(FRunnable* InRunnable, UBOOL bAutoDeleteSelf = FALSE, UBOOL bAutoDeleteRunnable = FALSE, DWORD StackSize = 0);
	virtual void Suspend(UBOOL bShouldPause = TRUE);
	virtual UBOOL Kill(UBOOL bShouldWait = FALSE, DWORD MaxWaitTime = 0);
	virtual void WaitForCompletion();
};

class FThreadFactoryWin : public FThreadFactory
{
public:
	virtual FRunnableThread* CreateThread(FRunnable* InRunnable, UBOOL bAutoDeleteSelf = FALSE, UBOOL bAutoDeleteRunnable = FALSE, DWORD StackSize = 0);
	virtual void Destroy(FRunnableThread* Thread);
};

#endif
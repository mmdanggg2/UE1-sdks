/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Header file for commandlet. Implements class, basic
 * functions and variables.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Raven
 */
#include "UEnginePPCCore.h"
#include "UEnginePPCVersion.h"
#include "FConfigCacheIni.h"

//structure which holds information about variables
struct SVars
{
	FString VType;		// variable type (not ust ATM)
	FString VName;		// variable name
	FString VValue;		// variable value
};

//structure which holds information about per-file commands
struct SInfo
{
	UBOOL bWasDebug;	// was originally debug mode on
	UBOOL bWasClean;	// was originally clean mode on
	UBOOL bWasChanged;	// was variable changed by previous file
};

//structure for import info
struct SImportInfo
{
	FString Directory;
	FString Extension;
	FString Type;
	FString Group;
	FString LodSet;
	FString Flags;
	FString Package;
};
//macros !!
struct sMacroParameters
{
	FString Key;
	FString Value;
};
struct sMacroInfo
{
	FString MacroLook;
	FString MacroKey;
	FString MacroValue;
	sMacroParameters Parameters[8];
	INT NumParams;
};


class UENGINEPPC_API UParse : public UCommandlet
{
	#if ENGINE_VERSION>226
		DECLARE_CLASS(UParse,UCommandlet,CLASS_Transient,UEnginePPC)
	#else
		DECLARE_CLASS(UParse,UCommandlet,CLASS_Transient)
	#endif
	// public functions (available to other classes)
public:
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Static Constructor
	 */
	void StaticConstructor();
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Initialize basic variables
	 */
	UParse();
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Displays header
	 */
	virtual void InitExecution();
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Initialize parser
	 */
	virtual INT Main( TCHAR* Parms );
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Prints help file
	 */
	void PrintHelpFile()
	{
		GWarn->Logf( TEXT("Usage:"));
		GWarn->Logf( TEXT("    ucc uengineppc.parse project=[<project_dir>/<project_file>] [-option...] [-globals...]"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("*Parameters:"));
		GWarn->Logf( TEXT("    -<project_dir> - relative project directory."));
		GWarn->Logf( TEXT("    -<project_file> - file (.upc extension) conaining all options. If file is detected, no fuhrer modifiers are checked"));
		GWarn->Logf( TEXT("**Options:"));
		GWarn->Logf( TEXT("    -clean - deletes preprocessor directives from .uc file"));
		GWarn->Logf( TEXT("    -debug - turns on debug mode (prints every operation on parsed .uc file)"));
		GWarn->Logf( TEXT("    -printglobals - prints all global variables"));
		GWarn->Logf( TEXT("    -normalizeeol - tries to find \r and \n and change them into \r\n"));
		GWarn->Logf( TEXT("    -bIsPackage - when defining <project_dir> or path in project file, you can type only name of package. Path will be detected automatically"));
		GWarn->Logf( TEXT("    -bIniVersion - macro __UENGINEVERSION__ will return uengine version saved in INI (FirstRun param), if false, it'll return version saved in engine."));
		GWarn->Logf( TEXT("    -deletelog - scans UScript source for log functions and deletes it"));
		GWarn->Logf( TEXT("**Globals:"));
		GWarn->Logf( TEXT("    Each other parameter will be concidered as global variable. If = is not detected, global variable is equal null. Example:"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("    val1=1 val val2=3"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("*Directives:"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("`process				- should be in the first line of .uc file. Tells preprocessor to parse file"));
		GWarn->Logf( TEXT("`include(file)				- embade file in the currently opened .uc (do not parses it)"));
		GWarn->Logf( TEXT("`include(file,false)			- embade file in the currently opened .uc (do not parses it)"));
		GWarn->Logf( TEXT("`include(file,true)			- embade file in the currently opened .uc and parses it"));
		GWarn->Logf( TEXT("`require(file)				- embade file in the currently opened .uc (do not parses it). If required file doesn't exists, it stops parsing current file and produce error."));
		GWarn->Logf( TEXT("`require(file,false)			- embade file in the currently opened .uc (do not parses it). If required file doesn't exists, it stops parsing current file and produce error."));
		GWarn->Logf( TEXT("`require(file,true)			- embade file in the currently opened .uc and parses it. If required file doesn't exists, it stops parsing current file and produce error."));
		GWarn->Logf( TEXT("`define(name)				- defines variable name (used in `ifdef and `ifndef directives)"));
		GWarn->Logf( TEXT("`define(name,value)			- defines variable name with specified value (used in `if and ternary operation)"));
		GWarn->Logf( TEXT("`undef(name)				- removes name from local definitions"));
		GWarn->Logf( TEXT("`error(name1,true)			- produces error message and exits commandlet"));
		GWarn->Logf( TEXT("`error(name1)				- produces error message and stops parsing current file"));
		GWarn->Logf( TEXT("`warn(name1)				- produces warning message"));
		GWarn->Logf( TEXT("`log(name1)				- produces message"));
		GWarn->Logf( TEXT("`ifdef(name)				- evaluates to true if variable name is defined"));
		GWarn->Logf( TEXT("`ifndef(name)				- evaluates to true if variable name is not defined"));
		GWarn->Logf( TEXT("`if([expression1] [operator] [expression2])			- checks to see if the first condition is true by comparing expression1 to expression2 using the operator."));
		GWarn->Logf( TEXT("`else if([expression1] [operator] [expression2])			- checks to see if the first condition is true by comparing expression1 to expression2 using the operator. Only if first condition is false") );
		GWarn->Logf( TEXT("`else					- part of conditional statement"));
		GWarn->Logf( TEXT("`endif					- ends conditional statement"));
		GWarn->Logf( TEXT("`write(name)				- writes defined variable name"));
		GWarn->Logf( TEXT("`write(name1==name2option1:option2)	- if statemente evaluate to true (variable name1 equals variable name2) writes option1 otherwise writes option 2"));
		GWarn->Logf( TEXT("`write(name1<>name2?option1:option2)	- if statemente evaluate to true (variable name1 does not match variable name2) writes option1 otherwise writes option 2"));
		GWarn->Logf( TEXT("`write(name1>name2?option1:option2)	- if statemente evaluate to true (variable name1 is greater then variable name2) writes option1 otherwise writes option 2"));
		GWarn->Logf( TEXT("`write(name1<name2?option1:option2)	- if statemente evaluate to true (variable name1 is less then variable name2) writes option1 otherwise writes option 2") );
		GWarn->Logf( TEXT("`write(name1?option1:option2)		- if statemente evaluate to true (variable name1 is defined) writes option1 otherwise writes option 2"));
		GWarn->Logf( TEXT("`import(directory,extension,type,group,lodset,flags,package) - can be used to import textures/sounds from chosen directory"));
		GWarn->Logf( TEXT("`namespace(name,value)			- defines namespace name with specified value. It's combination of `define and `write. If namespace will be detected, it'll be replaced by value, without need of using `write."));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("Notice that all variables used in directive `if and trenary operation are parsed in following order:"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("1. Returns value from global variables if correct name is found, otherwise..."));
		GWarn->Logf( TEXT("2. Returns value from local variables if correct name is found, otherwise..."));
		GWarn->Logf( TEXT("3. Assumes that name is value."));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("UnrealEngine 1 Commandlet preprocessor  macros:"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("__FILE__ - will write name of currently parsed file, usable in conditional statements"));
		GWarn->Logf( TEXT("__CLASS__ - will write name of currently parsed class, usable in conditional statements"));
		GWarn->Logf( TEXT("__DATE__ - will write time"));
		GWarn->Logf( TEXT("__SELF__ - will write package name, usable in conditional statements"));
		GWarn->Logf( TEXT("__UENGINEVERSION__ - will write current uengine version, usable in conditional statements"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("*Project file"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("[project]                 - project informations"));
		GWarn->Logf( TEXT("path=path                 - path to project"));
		GWarn->Logf( TEXT("debug=true                - turns on debug mode (prints every operation on parsed .uc)"));
		GWarn->Logf( TEXT("clean=true                - if true will delete preprocessor directives"));
		GWarn->Logf( TEXT("output=folder             - override default output folder where parsed .uc files are written"));
		GWarn->Logf( TEXT("input=folder              - override default input folder where parsed .uc files are stored"));
		GWarn->Logf( TEXT("bIniVersion=true          - if true, macro __UENGINEVERSION__ will return uengine version saved in INI (FirstRun param), if false, it'll return version saved in engine."));
		GWarn->Logf( TEXT("bDeleteLog=true           - scans UScript source for log functions and deletes it"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("[globals]                 - group contatin global variables for whole project"));
		GWarn->Logf( TEXT("someglobal=somevalue      - global variable (sample)"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("example:"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("[project]"));
		GWarn->Logf( TEXT("path=../MyProject/"));
		GWarn->Logf( TEXT("debug=true"));
		GWarn->Logf( TEXT("make=true"));
		GWarn->Logf( TEXT("make_ini=make.ini"));
		GWarn->Logf( TEXT("clean=true"));
		GWarn->Logf( TEXT("output=classes"));
		GWarn->Logf( TEXT("input=classes/preprocessor"));
		GWarn->Logf( TEXT(""));
		GWarn->Logf( TEXT("[globals]"));
		GWarn->Logf( TEXT("global_value1=test1"));
		GWarn->Logf( TEXT("global_value2=test2"));
		return;
	}
private:
	//private variables and functions (available for this class only)
	FTime StartupTime;				// startup time
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	INT num_files;					// number of uc files in directory
	INT num_parsed;					// number of parsed files
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FString Project;				// project (directory or .upc file)
	FString ProjectDir;				// project directory
	FString DateType;				// date format
	FString makeini;				// name of ini to use in ucc make
	FString output;					// output directory
	FString input;					// input directory
	FString CurClass;				// currently class (for __CLASS__ macro)
	FString CurFile;				// currently parsed file (for __FILE__ macro)
	FString CurPackage;				// package
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	UBOOL bForce;					// force preprocessor for all files
	UBOOL bUseProjectFile;			// is project file in use
	UBOOL debug;					// should print detailed stats
	UBOOL clean;					// should clean the code
	UBOOL printglobals;				// should global variables be printed
	UBOOL printmacros;
	UBOOL printnamespace;			// should global variables be printed
	UBOOL normalizeeol;				// should clean the code
	UBOOL make;						// should clean the code
	UBOOL bIsPackage;				// is project a packege
	UBOOL bDeleteLog;				// should we delete log
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	TArray<SVars> VGlobal;			// global variables (returns GPF)
	TArray<SVars> VLocal;			// local variables (returns GPF)
	sMacroInfo GlobalMacros[32];	// global macros
	INT GlobalMacros_num;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FString VLocalString;			// local variables
	FString VGlobalString;			// global variables
	FString VNameSpaceString;
	FString VNameSpaceGlobal;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FConfigCacheIni* ConfigCache;	// main cache for project config
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	UBOOL bCrashCommandlet;			// should we crash commandlet...
	UBOOL bError;					// ..or just trigger error
	UBOOL bIniVersion;				//
	FString UENGVER;
	INT native_offset;
	INT CUR_Line, REL_Line, TOT_Lines;
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Returns current engine version
	 */
	INT GetUEngineVersion()
	{
		#if ENGINE_VERSION==430
			return 436;
		#else
			return ENGINE_VERSION;
		#endif
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Check if current line contain a macro
	 */
	UBOOL MacroCheck(FString& Src)
	{
		if( Src == FString(TEXT("__LINE__")) )
		{
			Src = FStrReplace(Src, FString(TEXT("__LINE__")), FString::Printf(TEXT("%i"),CUR_Line));
			if(debug)
				GWarn->Logf( TEXT("...macro __LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),CUR_Line));
		}
		if( Src == FString(TEXT("__RELATIVE_LINE__")) )
		{
			Src = FStrReplace(Src, FString(TEXT("__RELATIVE_LINE__")), FString::Printf(TEXT("%i"),REL_Line));
			if(debug)
				GWarn->Logf( TEXT("...macro __RELATIVE_LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),REL_Line));
		}
		if(Src == FString(TEXT("__TOTAL_LINES__")) )
		{
			Src = FStrReplace(Src, FString(TEXT("__TOTAL_LINES__")), FString::Printf(TEXT("%i"),TOT_Lines));
			if(debug)
				GWarn->Logf( TEXT("...macro __TOTAL_LINES__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),TOT_Lines));
		}
		if( Src == FString(TEXT("__FILE__")) )
		{
			Src = CurFile;
			if(debug)
				GWarn->Logf( TEXT("......macro __FILE__ found. Value inserted: %ls"), *CurFile);
			return 1;
		}
		else if( Src == FString(TEXT("__CLASS__")) )
		{
			Src = CurClass;
			if(debug)
				GWarn->Logf( TEXT("......macro __CLASS__ found in directive `if. Value used: %ls"), *CurFile);
			return 1;
		}
		else if( Src == FString(TEXT("__SELF__")) )
		{
			Src = CurPackage;
			if(debug)
				GWarn->Logf( TEXT("......macro __SELF__ found in directive `if. Value used: %ls"), *CurFile);
			return 1;
		}
		else if( Src == FString(TEXT("__UENGINEVERSION__")) )
		{
			Src = UENGVER;
			if(debug)
				GWarn->Logf( TEXT("......macro __UENGINEVERSION__ found in directive `if. Value used: %ls"), *UENGVER);
			return 1;
		}
		return 0;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Check if current line contain a macro
	 */
	FString MacroReplace(FString& Src)
	{
		if( Src.InStr( FString(TEXT("__LINE__"))) != -1 )
		{
			Src = FStrReplace(Src, FString(TEXT("__LINE__")), FString::Printf(TEXT("%i"),CUR_Line) );
			if(debug)
				GWarn->Logf( TEXT("...macro __LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),CUR_Line) );
		}
		if( Src.InStr( FString(TEXT("__RELATIVE_LINE__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__RELATIVE_LINE__")), FString::Printf(TEXT("%i"),REL_Line) );
			if(debug)
				GWarn->Logf( TEXT("...macro __RELATIVE_LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),REL_Line) );
		}
		if( Src.InStr( FString(TEXT("__TOTAL_LINES__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__TOTAL_LINES__")), FString::Printf(TEXT("%i"),TOT_Lines) );
			if(debug)
				GWarn->Logf( TEXT("...macro __TOTAL_LINES__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),TOT_Lines) );
		}
		if( Src.InStr( FString(TEXT("__FILE__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__FILE__")), *CurFile);
			if(debug)
				GWarn->Logf( TEXT("...macro __FILE__ found. Value inserted: %ls"), *CurFile);
		}
		//macro __CLASS__
		if( Src.InStr( FString(TEXT("__CLASS__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__CLASS__")), CurClass);
			if(debug)
				GWarn->Logf( TEXT("...macro __CLASS__ found. Value inserted: %ls"), *CurClass);
		}
		//macro __DATE__
		if( Src.InStr( FString(TEXT("__DATE__"))) != -1)
		{
			FString CurTime = GetTime();
			Src = FStrReplace(Src, FString(TEXT("__DATE__")), CurTime);
			if(debug)
				GWarn->Logf( TEXT("...macro __DATE__ found. Value inserted: %ls"), *CurTime);
		}
		//macro __SELF__
		if( Src.InStr( FString(TEXT("__SELF__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__SELF__")), CurPackage);
			if(debug)
				GWarn->Logf( TEXT("...macro __SELF__ found. Value inserted: %ls"), *CurPackage);
		}
		//macro __UENGINEVERSION__
		if( Src.InStr( FString(TEXT("__UENGINEVERSION__"))) != -1)
		{
			Src = FStrReplace(Src, FString(TEXT("__UENGINEVERSION__")), *UENGVER);
			if(debug)
				GWarn->Logf( TEXT("...macro __UENGINEVERSION__ found. Value inserted: %ls"), *UENGVER);
		}
		return Src;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Checks if line is a directive
	 */
	UBOOL IsDirective(FString Src)
	{
		guard(UParse::IsDirective);
		int div_pos=Src.InStr(FString(TEXT("`")));

		if(div_pos != -1)
		{
			FString Div2=FString(TEXT("`write"));
			if(Src.InStr(Div2) != -1) return 1;
			else if(div_pos < 2) return 1;
			else return 0;
		}
		else
			return 0;

		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Split a FString by FString.
	 * It works like function explode from PHP.
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * delimiter	- The boundary string.
	 * string		- The input string.
	 * limit		- If limit is set, the returned array will contain a maximum of limit elements.
	 * bCutRest		- If limit is set and bCutRest is false, the returned array will contain a maximum of limit elements with the last element containing the rest of string.
	 */
	TArray<FString> Explode(FString delimiter, FString string, INT limit = -1, UBOOL bCutRest = 0)
	{
		guard(UParse::Explode);
		TArray<FString> tmpArr;
		int line_num=0;

		while(string.Len() > 0)
		{
			if( limit > 0 )
			{
				line_num++;
			}

			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());
			if( limit > 0 && line_num >= limit )
			{
				if(!bCutRest)
					tmpArr.AddItem(*string);
				else
					tmpArr.AddItem(*tmpStr);
			}
			tmpArr.AddItem(*tmpStr);
			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}
		return tmpArr;
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Split a string into two parts with Divider as the cut-off point.
	 * Returns true when the string was divided.
	 */
	UBOOL FStringDivide(FString Src, FString Divider, FString& LeftPart, FString& RightPart)
	{
		guard(UParse::FStringDivide);
		int div_pos=Src.InStr(Divider);

		if(div_pos != -1)
		{
			LeftPart = Src.Left(div_pos);
			RightPart = Src.Mid(LeftPart.Len()+Divider.Len(), Src.Len()-LeftPart.Len()-Divider.Len());
			return 1;
		}
		else
			return 0;

		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * replaces 'replace' with 'with' in text
	 */
	FString FStrReplace( FString Text, FString Replace, FString With )
	{
		guard(UParse::FStrReplace);
		int i;
		FString InputT;

		InputT = Text;
		Text = TEXT("");
		i = InputT.InStr(Replace);
		while(i != -1)
		{
			Text = Text + InputT.Left(i) + With;
			InputT = InputT.Mid(i + Replace.Len());
			i = InputT.InStr(Replace);
		}
		Text = Text+ InputT;
		return Text;
		unguard;
	}
	FString FTabRemove( FString Text )
	{
		return FStrReplace( Text, FString(TEXT("\t")), FString(TEXT("")) );
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * These functions remove spaces from the left, the right or both sides of a string.
	 */
	FString FStrLTrim( FString Text )
	{

		while(Text.Left(1) == FString(TEXT(" ")))
		{
			Text = Text.Right(Text.Len()-1);
		}
		return Text;
	}
	FString FStrRTrim( FString Text )
	{

		while(Text.Right(1) == FString(TEXT(" ")))
		{
			Text = Text.Left(Text.Len()-1);
		}
		return Text;
	}
	FString FStrTrim( FString Text )
	{
		return FStrLTrim(FStrRTrim(Text));
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Extract string from between two different dividers
	 * Returns true when the string was extracted.
	 */
	UBOOL FStringExtract(FString Src, FString LeftDivider, FString RightDivider, FString& MidString)
	{
		guard(UParse::FStringExtract);

		int inc_first=Src.InStr( LeftDivider );
		int inc_last=Src.InStr( RightDivider );
		if(inc_first != -1 && inc_last != -1)
		{
			MidString = Src.Mid(inc_first+LeftDivider.Len(), inc_last-LeftDivider.Len());
			return 1;
		}
		else
			return 0;

		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Extract string from between two different dividers
	 * Returns true when the string was extracted.
	 */
	UBOOL FStringAdvExtract(FString Src, FString LeftDivider, FString RightDivider, FString& MidString, FString LeftBracketSg = FString(TEXT("(")))
	{
		guard(UParse::FStringAdvExtract);

		int inc_first=Src.InStr( LeftDivider );
		int inc_last=Src.InStr( RightDivider );
		if(inc_first != -1)
		{
			FString tmp = Src.Left(inc_first+LeftDivider.Len());
			tmp = FStrReplace(Src,tmp,FString(TEXT("")));

			inc_last=tmp.InStr( RightDivider );

			UBOOL bBracketClosed;
			INT CurChk = tmp.InStr(LeftBracketSg);

			INT loop_check=0;
			UBOOL bEOS=0;

			if(CurChk < inc_last)
				bBracketClosed = 0;
			else
				goto Process;

			while(!bEOS && loop_check < 2000)
			{
				loop_check++;
				inc_last = InFStr(tmp, RightDivider, CurChk);
				if(inc_last != -1)
					bBracketClosed = 1;
				CurChk = InFStr(tmp, LeftBracketSg, inc_last);
				if(CurChk < inc_last)
					bBracketClosed = 0;
				if(CurChk == -1 || CurChk > inc_last)
					bEOS=1;
			}
			if(bBracketClosed)
				inc_last = InFStr(tmp, RightDivider, inc_last+RightDivider.Len());
			Process:
			if(inc_last != -1)
			{
				MidString = tmp.Mid(0, inc_last);
				return 1;
			}
			return 0;
		}
		else
			return 0;

		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Finds substring inside string after certain number of characters
	 */
	INT InFStr(FString Src,  FString SubStr, INT Start=0 )
	{
		guard(UParse::InFStr);
		if(Start <= 0 || Start >= Src.Len()) return Src.InStr(SubStr);

		FString tmp = Src.Left(Start);

		return FStrReplace(Src,tmp,FString(TEXT(""))).InStr(SubStr)+tmp.Len();

		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Add slashes
	 */
	FString AddSlashes( FString Text, FString Text2=FString(TEXT("")) )
	{
		guard(UParse::AddSlashes);
		if(Text2 != FString(TEXT("")))
			return FStrReplace(Text+Text2, FString(TEXT("/")), FString(TEXT("//")));
		else
			return FStrReplace(Text, FString(TEXT("/")), FString(TEXT("//")));
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Removes spaces
	 */
	FString RemoveSpaces( FString Text )
	{
		guard(UParse::RemoveSpaces);
		return FStrReplace(Text, FString(TEXT(" ")), FString(TEXT("")));
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * check if test_dir contain backslash at the end
	 */
	FString IsDir( FString test_dir )
	{
		guard(UParse::IsDir);
		if(test_dir.Right(1) == FString(TEXT("/")))
			return test_dir;
		else
			return test_dir+FString(TEXT("/"));
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Converts slash to backslash
	 */
	FString ConvertSlash( FString Text )
	{
		guard(UParse::ConvertSlash);
		return FStrReplace(Text, FString(TEXT("\\")), FString(TEXT("/")));
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Converts \r and \n to \r\n (optional)
	 */
	FString NormalizeEOL( FString Text )
	{
		guard(UParse::NormalizeEOL);

		if(Text.InStr(FString(TEXT("\r\n"))) == -1 && Text.InStr(FString(TEXT("\r"))) != -1)
			return FStrReplace(Text,FString(TEXT("\r")),FString(TEXT("\r\n")));
		else if(Text.InStr(FString(TEXT("\r\n"))) == -1 && Text.InStr(FString(TEXT("\n"))) != -1)
			return FStrReplace(Text,FString(TEXT("\n")),FString(TEXT("\r\n")));

		return Text;
		unguard;
	}
	FString NormalizeEOL2( FString Text )
	{
		guard(UParse::NormalizeEOL);

		Text = FStrReplace(Text,FString(TEXT("\r")),FString(TEXT("")));
		Text = FStrReplace(Text,FString(TEXT("\n")),FString(TEXT("")));
		return Text;
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Returns class name
	 */
	FString GetClass( FString File )
	{
		guard(UParse::GetClass);


		FString Text=FString(TEXT(""));
		FString CName=FString(TEXT("xx"));
		//GLog->Logf(TEXT("File = %ls"),File);
		if( appLoadFileToString(Text,*File) )
		{
			//GLog->Logf(TEXT("Text.Len() = %i"),Text.Len());
			while(Text.Len() > 0)
			{
				int linebreak = Text.InStr(FString(TEXT("\r\n")));
				//GLog->Logf(TEXT("linebreak = %i"),linebreak);
				FString tmpStr, keyword, tmpStr2;
				if(linebreak == -1) //we have reached end of text
					tmpStr = Text;
				else
					tmpStr = Text.Mid(0,linebreak+2);

				if(tmpStr.InStr(FString(TEXT("extends"))) != -1 )
					keyword = FString(TEXT("extends"));
				else if(tmpStr.InStr(FString(TEXT("expands"))) != -1 )
					keyword = FString(TEXT("expands"));

				//GLog->Logf(TEXT("keyword = %ls"),keyword);

				if(keyword != FString(TEXT("")))
				{
					int ClsPos = tmpStr.InStr(FString(TEXT("class")));
					int KwdPos = tmpStr.InStr(keyword);
					CName=RemoveSpaces( tmpStr.Mid(ClsPos+FString(TEXT("class")).Len(), KwdPos-keyword.Len()+1) );

					break;
				}

				if(linebreak == -1)
					Text = FString(TEXT(""));
				else
					Text = Text.Mid(linebreak+2);
			}

		}

		return CName;
		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Returns current time
	 */
	FString GetTime()
	{
		guard(UParse::GetTime);
		INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
		appSystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );
		//FString CurDate = CurDate.Printf(TEXT("%ls-%ls-%ls %ls:%ls"), Day, Month, Year, Hour, Min);
		//TCHAR DayBegin=(Day < (int)10)?TEXT("0"):TEXT("");
		//TCHAR MinBegin=(Min < (int)10)?TEXT("0"):TEXT("");
		FString DATE=FString::Printf(TEXT("%i-%i-%i "), Day, Month, Year);
		if(Month < (int)10) DATE=FString::Printf(TEXT("%i-0%i-%i "), Day, Month, Year);
		if(Day < (int)10) DATE=FString(TEXT("0"))+DATE;
		FString MINUTES=FString::Printf(TEXT("%i:%i"),Hour, Min);
		if(Min < (int)10) MINUTES=FStrReplace(MINUTES, FString::Printf(TEXT(":%i"), Min), FString::Printf(TEXT(":0%i"), Min));

		return DATE+MINUTES;
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * It should convert FString into array.
	 * Each line is new item in the array
	 * for some reason sometimes it couses GPF :)
	 * Thats why it's not used anymore.
	 * I left it, because it might be useful someday :).
	 */
	TArray<FString> LoadFStringToTArray( FString Text, UBOOL bDebugLines=false )
	{
		guard(UParse::LoadFStringToTArray);
		return Explode( Text, FString(TEXT("\r\n")) );
		unguard;
	}

	INT NumLines(FString string)
	{
		guard(UParse::Explode);
		int line_num=0;
		FString delimiter = FString(TEXT("\r\n"));

		while(string.Len() > 0)
		{
			line_num++;

			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());
			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}
		return line_num;
		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Get parameters from createexec directive
	 */
	SImportInfo GetExecParams(FString string)
	{
		guard(UParse::GetExecParams);

		int param=0;
		FString delimiter=FString(TEXT(","));

		SImportInfo ImpInfo;

		ImpInfo.Directory = FString(TEXT(""));
		ImpInfo.Extension = FString(TEXT(""));
		ImpInfo.Type = FString(TEXT(""));
		ImpInfo.Group = FString(TEXT(""));
		ImpInfo.LodSet = FString(TEXT(""));
		ImpInfo.Flags = FString(TEXT(""));
		ImpInfo.Package = FString(TEXT(""));

		while(string.Len() > 0)
		{
			if(param >= 7) break;

			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos);

			switch(param)
			{
				case 0:
					ImpInfo.Directory = tmpStr;
				break;
				case 1:
					ImpInfo.Extension = tmpStr;
				break;
				case 2:
					ImpInfo.Type = tmpStr;
				break;
				case 3:
					ImpInfo.Group = tmpStr;
				break;
				case 4:
					ImpInfo.LodSet = tmpStr;
				break;
				case 5:
					ImpInfo.Flags = tmpStr;
				break;
				case 6:
					ImpInfo.Package = tmpStr;
				break;
			}

			param++;

			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}
		return ImpInfo;
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Adds namespace
	 */
	UBOOL AddNamespace(FString name, FString val, FString& VLString)
	{
		guard(UParse::AddNamespace);

		if(VLString.InStr(name) != -1) return 0;
		if(VNameSpaceGlobal.InStr(name) != -1) return 0;

		val = FStrReplace(val, FString(TEXT("__FILE__")), CurFile);
		val = FStrReplace(val, FString(TEXT("__CLASS__")), CurClass);
		FString CurTime = GetTime();
		val = FStrReplace(val, FString(TEXT("__DATE__")), CurTime);
		val = FStrReplace(val, FString(TEXT("__SELF__")), CurPackage);

		VLString+=name+FString(TEXT("="))+val+FString(TEXT("||"));
		//VNameSpaceString+=name+FString(TEXT("="))+val+FString(TEXT("||"));VNameSpaceGlobal

		return 1;
		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Adds namespace
	 */
	UBOOL AddNamespaceGlobal(FString name, FString val, FString& VLString)
	{
		guard(UParse::AddNamespaceGlobal);

		if(VLString.InStr(name) != -1) return 0;

//		val = FStrReplace(val, FString(TEXT("__FILE__")), CurFile);
//		val = FStrReplace(val, FString(TEXT("__CLASS__")), CurClass);
		//FString CurTime = GetTime();
		//val = FStrReplace(val, FString(TEXT("__DATE__")), CurTime);
		//val = FStrReplace(val, FString(TEXT("__SELF__")), CurPackage);

		VLString+=name+FString(TEXT("="))+val+FString(TEXT("||"));
		//VNameSpaceString+=name+FString(TEXT("="))+val+FString(TEXT("||"));

		return 1;
		unguard;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Replaces Namespace
	 */
	FString ReplaceNamespace(FString str)
	{
		guard(UParse::ReplaceNamespace);

		FString delimiter = FString(TEXT("||"));
		FString string = VNameSpaceGlobal;

		while(string.Len() > 0)
		{
			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());

			FString gl_name=tmpStr;
			FString gl_val=FString(TEXT("null"));
			FStringDivide(tmpStr,FString(TEXT("=")), gl_name, gl_val);

			gl_val=FStrReplace(gl_val,FString(TEXT("||")),FString(TEXT("")));
			//macro check
			gl_val = FStrReplace(gl_val, FString(TEXT("__FILE__")), CurFile);
			gl_val = FStrReplace(gl_val, FString(TEXT("__CLASS__")), CurClass);
			FString CurTime = GetTime();
			gl_val = FStrReplace(gl_val, FString(TEXT("__DATE__")), CurTime);
			gl_val = FStrReplace(gl_val, FString(TEXT("__SELF__")), CurPackage);

			FString OldStr = str;
			str = FStrReplace(str, gl_name, gl_val);
			if(OldStr != str && debug)
				GWarn->Logf( TEXT("...global namespace detected. Value inserted: %ls"), *gl_val);

			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}

		string = VNameSpaceString;

		while(string.Len() > 0)
		{
			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());

			FString gl_name=tmpStr;
			FString gl_val=FString(TEXT("null"));
			FStringDivide(tmpStr,FString(TEXT("=")), gl_name, gl_val);

			gl_val=FStrReplace(gl_val,FString(TEXT("||")),FString(TEXT("")));

			FString OldStr = str;
			str = FStrReplace(str, gl_name, gl_val);
			if(OldStr != str && debug)
				GWarn->Logf( TEXT("...namespace detected. Value inserted: %ls"), *gl_val);

			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}
		return str;
		unguard;
	}


	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Add local variable
	 */
	UBOOL AddLocalVar(FString name, FString val, FString& VLString)
	{
		guard(UParse::AddLocalVar);

		if(VLString.InStr(name) != -1) return 0;

		VLString+=name+FString(TEXT("="))+val+FString(TEXT("||"));
		return 1;
		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Delete local variable
	 */
	UBOOL DelLocalVar(FString name)
	{
		guard(UParse::AddLocalVar);

		FString VSearch = name+FString(TEXT("="));
		FString val;

		INT ins = VLocalString.InStr(VSearch);

		if(ins == -1) return 0;

		FString tmp1, tmp2;

		FStringDivide(VLocalString, VSearch, tmp1, tmp2);

		if(tmp2.InStr(FString(TEXT("||"))) != -1)
		{
			FStringDivide(tmp2, FString(TEXT("||")), val, tmp1);
		}
		else
			val = tmp2;

		VSearch = name+FString(TEXT("="))+val;

		if( VLocalString.InStr(VSearch+FString(TEXT("||"))) != -1 )
			VLocalString = FStrReplace(VLocalString, VSearch+FString(TEXT("||")), FString(TEXT("")));
		else
			VLocalString = FStrReplace(VLocalString, VSearch, FString(TEXT("")) );
		return 1;

		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Finds local variable
	 */
	UBOOL FindLocalVar(FString name, FString& val, FString VLString = FString(TEXT("")))
	{
		guard(UParse::FindLocalVar);
		FString VSearch = name+FString(TEXT("="));

		if(VLString == FString(TEXT(""))) VLString = VLocalString;

		INT ins = VLString.InStr(VSearch);

		if(ins == -1) return 0;

		FString tmp1, tmp2;

		FStringDivide(VLString, VSearch, tmp1, tmp2);

		if(tmp2.InStr(FString(TEXT("||"))) != -1)
		{
			FStringDivide(tmp2, FString(TEXT("||")), val, tmp1);
		}
		else
			val = tmp2;

		return 1;
		unguard;
	}

	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Parses input string in search for global variables
	 */
	void ParseGlobalString(FString globalappstr)
	{
		guard(UParse::ParseGlobalString);
		FString delimiter = FString(TEXT(" "));

		while(globalappstr.Len() > 0)
		{
			int div_pos = globalappstr.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = globalappstr;
			else
				tmpStr = globalappstr.Mid(0,div_pos+delimiter.Len());

			if(tmpStr.InStr(FString(TEXT("project="))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-debug"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-clean"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-printglobals"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-make"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("makeini="))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-bIsPackage"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-bIniVersion"))) == -1 &&
			   tmpStr.InStr(FString(TEXT("-deletelog"))) == -1
			   )
			{

				FString gl_name=tmpStr;
				FString gl_val=FString(TEXT("null"));
				FStringDivide(tmpStr,FString(TEXT("=")), gl_name, gl_val);

				AddLocalVar(gl_name, gl_val, VGlobalString);
			}

			if(div_pos == -1)
				globalappstr = FString(TEXT(""));
			else
				globalappstr = globalappstr.Mid(div_pos+delimiter.Len());
		}


		unguard;
	}


	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Print global variables
	 */
	void PrintGlobals()
	{
		guard(UParse::ParseGlobalString);
		FString delimiter = FString(TEXT("||"));
		FString string = VGlobalString;

		while(string.Len() > 0)
		{
			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());

			FString gl_name=tmpStr;
			FString gl_val=FString(TEXT("null"));
			FStringDivide(tmpStr,FString(TEXT("=")), gl_name, gl_val);
			GWarn->Logf( TEXT("variable: %ls "), *gl_name);
			gl_val=FStrReplace(gl_val,FString(TEXT("||")),FString(TEXT("")));
			if(gl_val != FString(TEXT("null")))
				GWarn->Logf( TEXT("...value: %ls "), *gl_val);

			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}


		unguard;
	}

	void PrintMacros()
	{
		for(int i=0; i<GlobalMacros_num; i++)
		{

			GWarn->Logf( TEXT("macro: %ls"), *GlobalMacros[i].MacroLook);
			GWarn->Logf( TEXT("...key: %ls"), *GlobalMacros[i].MacroKey );
			GWarn->Logf( TEXT("...value: %ls"), *GlobalMacros[i].MacroValue );
			GWarn->Logf( TEXT("...parameters: %i"), GlobalMacros[i].NumParams );
			if( GlobalMacros[i].NumParams > 0 )
			{
				for(int j=0; j<GlobalMacros[i].NumParams; j++)
				{
					GWarn->Logf( TEXT("......parameter(%i): %ls"), j, *GlobalMacros[i].Parameters[j].Value );
				}
			}
		}
	}
	void PrintNamespace()
	{
		guard(UParse::PrintNamespace);
		FString delimiter = FString(TEXT("||"));
		FString string = VNameSpaceGlobal;

		while(string.Len() > 0)
		{
			int div_pos = string.InStr(delimiter);
			FString tmpStr;
			if(div_pos == -1) //we have reached end of text
				tmpStr = string;
			else
				tmpStr = string.Mid(0,div_pos+delimiter.Len());

			FString gl_name=tmpStr;
			FString gl_val=FString(TEXT("null"));
			FStringDivide(tmpStr,FString(TEXT("=")), gl_name, gl_val);
			GWarn->Logf( TEXT("namespace: %ls "), *gl_name);
			gl_val=FStrReplace(gl_val,FString(TEXT("||")),FString(TEXT("")));
			if(gl_val != FString(TEXT("null")))
				GWarn->Logf( TEXT("...value: %ls "), *gl_val);

			if(div_pos == -1)
				string = FString(TEXT(""));
			else
				string = string.Mid(div_pos+delimiter.Len());
		}


		unguard;
	}
	UBOOL HasMacros(FString Str, INT& Pos)
	{
		for(int i=0; i<GlobalMacros_num; i++)
		{
			if( Str.InStr(GlobalMacros[i].MacroKey) != -1 )
			{
				Pos=i;
				return 1;
			}
		}
		return 0;
	}
	INT MacroPos(FString Str)
	{
		for(int i=0; i<GlobalMacros_num; i++)
		{
			if( Str.InStr(GlobalMacros[i].MacroKey) != -1 )
				return i;
		}
		return -1;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Search for global or local variable.
	 */
	UBOOL GetVariable(FString name, SVars& var, UBOOL ReturnName=0)
	{
			SVars tmp_var;
			UBOOL found=0;

			if( name == FString(TEXT("")) ) return 0;
			FString xtmpval=FString(TEXT(""));
			if(FindLocalVar(name, xtmpval, VGlobalString))
			{
				tmp_var.VName = name;
				tmp_var.VValue = xtmpval;
				var = tmp_var;
				found=1;
			}
			if(!found)
			{
				FString xtmpval;
				if(FindLocalVar(name, xtmpval))
				{
					tmp_var.VName = name;
					tmp_var.VValue = xtmpval;
					var = tmp_var;
					found=1;
				}
			}
			if(!found)
			{
				if( name.InStr(FString(TEXT("__NUMERATE_CPP__"))) != -1)
				{
					tmp_var.VName = name;
					tmp_var.VValue = FStrReplace(name, FString(TEXT("__NUMERATE_CPP__")), FString::Printf(TEXT("%i"), native_offset));
					native_offset++;
					found=1;
				}
				else
				{
					tmp_var.VName = name;
					tmp_var.VValue = name;
				}
				var = tmp_var;
			}
			return found;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Check if given expression is true
	 */
	UBOOL IsTrue(FString Value1, FString Value2, FString Operator)
	{
		if(Operator == FString(TEXT("==")))
		{
			if(Value1 == Value2) return 1;
			else return 0;
		}
		else if(Operator == FString(TEXT("<>")))
		{
			if(Value1 != Value2) return 1;
			else return 0;
		}
		else if(Operator == FString(TEXT("<")))
		{
			if(appAtof(*Value1) < appAtof(*Value2)) return 1;
			else return 0;
		}
		else if(Operator == FString(TEXT(">")))
		{
			if(appAtof(*Value1) > appAtof(*Value2)) return 1;
			else return 0;
		}
		else if(Operator == FString(TEXT("<=")))
		{
			if(appAtof(*Value1) <= appAtof(*Value2)) return 1;
			else return 0;
		}
		else if(Operator == FString(TEXT(">=")))
		{
			if(appAtof(*Value1) >= appAtof(*Value2)) return 1;
			else return 0;
		}
		return 0;
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Parses condition
	 */
	void ParseCondition(FString tmpStr, int& nested, UBOOL& SkipLine)
	{
			nested++;
			FString if_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString if_cmd;
			FStringExtract(if_string, FString(TEXT("`if(")), FString(TEXT(")")), if_cmd);

			FString ifdiv=FString(TEXT("none"));

			if(if_cmd.InStr(FString(TEXT("=="))) != -1)
				ifdiv = FString(TEXT("=="));
			else if(if_cmd.InStr(FString(TEXT("<>"))) != -1)
				ifdiv = FString(TEXT("<>"));
			else if(if_cmd.InStr(FString(TEXT("<"))) != -1)
				ifdiv = FString(TEXT("<"));
			else if(if_cmd.InStr(FString(TEXT(">"))) != -1)
				ifdiv = FString(TEXT(">"));
			else if(if_cmd.InStr(FString(TEXT("<="))) != -1)
				ifdiv = FString(TEXT("<="));
			else if(if_cmd.InStr(FString(TEXT(">="))) != -1)
				ifdiv = FString(TEXT(">="));

			if(ifdiv != FString(TEXT("none")))
			{
				FString if_v1, if_v2;

				FStringDivide(if_cmd, ifdiv, if_v1, if_v2);

				SVars if_var1, if_var2;
				GetVariable(if_v1,if_var1);
				GetVariable(if_v2,if_var2);

				MacroCheck(if_var1.VValue);

				if(IsTrue(if_var1.VValue, if_var2.VValue, ifdiv))
				{
					SkipLine=0;
					if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to true"), *if_cmd);
				}
				else
				{
					SkipLine=1;
					if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to false"), *if_cmd);
				}
			}
			else
			{
				SVars if_var;
				UBOOL neg = false;
				if(if_cmd.InStr(FString(TEXT("!"))) != -1)
				{
					if_cmd=FStrReplace(if_cmd,FString(TEXT("!")),FString(TEXT("")));
					neg=true;
				}
				UBOOL Evl = GetVariable(if_cmd,if_var);
				if(neg) Evl = !Evl;

				if( Evl )
				{
					SkipLine=0;
					if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to true"), *if_cmd);
				}
				else
				{
					SkipLine=1;
					if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to false"), *if_cmd);
				}
			}
	}
	/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * Parses UnrealScript source in search for preprocessor directives.
	 * Uses same basecode as LoadFStringToTArray.
	 */
	virtual FString ParseSource( FString Text, UBOOL& bError, UBOOL& bCrash);
};

IMPLEMENT_CLASS(UParse);

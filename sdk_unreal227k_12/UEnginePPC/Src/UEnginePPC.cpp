/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Main commandlet code. Parses UScript source in search
 * for preprocessor commands.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Raven
 */
#include "UEnginePPC.h"
/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Static constructor
 */
void UParse::StaticConstructor()
{
	if(appStrfind(appCmdLine(),TEXT("HELP")))
	{
		InitExecution();
		PrintHelpFile();
	}
	return;
}
/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Initialize basic variables
 */
UParse::UParse()
{
	bUseProjectFile = false;
	debug = false;
	make = false;
	clean = false;
	printglobals = false;
	make = false;
	bIniVersion = false;
	bForce = false;
	makeini = FString(TEXT("none"));
	input = FString(TEXT("classes/preprocessor"));
	output = FString(TEXT("classes"));
	ConfigCache = new FConfigCacheIni();
	UENGVER = UENGVER.Printf(TEXT("%i"),GetUEngineVersion());
	bDeleteLog = false;
	printnamespace = false;
}
/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Displays header
 */
void UParse::InitExecution()
{
	guard(UParse::InitExecution);

	StartupTime = appSeconds();
	GWarn->Logf( TEXT("------------------------------------------") );
	GWarn->Logf( TEXT(" Unreal Engine Preprocessor Commandlet") );
	GWarn->Logf( TEXT("------------------------------------------") );
	GWarn->Logf( TEXT(" Author      : Raven"));
	GWarn->Logf( TEXT(" WWW         : http://turniej.unreal.pl/portfolio"));
	GWarn->Logf( TEXT(" Version     : %ls"), *uecpp_version );
	GWarn->Logf( TEXT(" Build date  : %ls"), *uecpp_date );
	GWarn->Logf( TEXT("------------------------------------------") );

	return;
	unguard;
}

/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Initialize parser
 */
INT UParse::Main( TCHAR* Parms )
{
	guard(UParse::Main);

	const TCHAR* MainProjectPath;
	SVars tmp_var;
	int DefNat=0;

	//if project is specified we can start to check everything
	if( Parse(Parms,TEXT("project="), Project) )
	{
		//parses project file
		if( Project.InStr(FString(TEXT(".upc"))) != -1 )
		{
			bUseProjectFile=true;//we're using project file
			GWarn->Logf( TEXT(" Using project definition: %ls"), *Project);
			MainProjectPath = ANSI_TO_TCHAR(TCHAR_TO_ANSI(*Project));
			if(!ConfigCache->Find(MainProjectPath,false))
			{
				GWarn->Logf( TEXT(" Project file not readable!"));
				return 0;
			}
			ConfigCache->GetBool(TEXT("project"), TEXT("bIsPackage"), bIsPackage, MainProjectPath );

			if( ConfigCache->GetString(TEXT("project"), TEXT("path"), ProjectDir, MainProjectPath ) )
			{
				if(bIsPackage) ProjectDir = FString(TEXT("../"))+ProjectDir;
				GWarn->Logf( TEXT(" Using project directory: %ls"), *ProjectDir);
			}
			else
			{
				GWarn->Logf( TEXT(" Project directory not specified!"));
				return 0;
			}

			//returns all important variables
			ConfigCache->GetBool(TEXT("project"), TEXT("debug"), debug, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("clean"), clean, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("printglobals"), printglobals, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("make"), make, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("bIniVersion"), bIniVersion, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("bDeleteLog"), bDeleteLog, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("bForce"), bForce, MainProjectPath );
			ConfigCache->GetInt(TEXT("project"), TEXT("native_offset"), native_offset, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("printnamespace"), printnamespace, MainProjectPath );
			ConfigCache->GetBool(TEXT("project"), TEXT("printfunctions"), printmacros, MainProjectPath );
			DefNat=native_offset;

			if(bIniVersion)
			{
					INT FirstRun = GetUEngineVersion();
					GConfig->GetInt(TEXT("FirstRun"), TEXT("FirstRun"), FirstRun);
					UENGVER = UENGVER.Printf(TEXT("%i"),FirstRun);
			}
			if( !ConfigCache->GetString(TEXT("project"), TEXT("dateformat"), DateType, MainProjectPath ) )
			{
				DateType = FString(TEXT("m-d-Y, H:i:s"));
			}
			if( !ConfigCache->GetString(TEXT("project"), TEXT("make_ini"), makeini, MainProjectPath ) )
			{
				makeini = FString(TEXT("none"));
			}
			if( !ConfigCache->GetString(TEXT("project"), TEXT("output"), output, MainProjectPath ) )
			{
				output = FString(TEXT("classes"));
			}
			if( !ConfigCache->GetString(TEXT("project"), TEXT("input"), input, MainProjectPath ) )
			{
				input = FString(TEXT("classes/preprocessor"));
			}

			UBOOL bClearOutput;
			ConfigCache->GetBool(TEXT("project"), TEXT("bClearOutput"), bClearOutput, MainProjectPath );
			if( bClearOutput )
			{
				FString output_del = AddSlashes(ProjectDir, output+FString(TEXT("/")) )+FString(TEXT("*.uc"));
				FString output_del2 = AddSlashes(ProjectDir, output+FString(TEXT("/")) );

				const TCHAR* search_del = ANSI_TO_TCHAR(TCHAR_TO_ANSI(*output_del));
				GWarn->Logf( TEXT("------------------------------------------"));
				GWarn->Logf( TEXT(" Clearing output: %ls"), *output_del);
				TArray<FString> DelFl = GFileManager->FindFiles(search_del , 1, 0 );
				num_files = DelFl.Num();
				for(int i=0; i<num_files; i++)
				{
					FString td_output = output_del2+DelFl.Last(num_files-(i+1));
					GFileManager->Delete(*td_output,0,1);
					GWarn->Logf( TEXT(" ...deleting file: %ls"), *td_output);
				}
				GWarn->Logf( TEXT("------------------------------------------"));
			}

			//returns global values
			TMultiMap<FString,FString>* tmp_globals = ConfigCache->GetSectionPrivate( TEXT("globals"), 0, 1, MainProjectPath );
			for( TMultiMap<FString,FString>::TIterator It(*tmp_globals); It; ++It )
			{
				AddLocalVar(It.Key(), It.Value(), VGlobalString);
			}

			TMultiMap<FString,FString>* tmp_nmspace = ConfigCache->GetSectionPrivate( TEXT("namespace"), 0, 1, MainProjectPath );
			for( TMultiMap<FString,FString>::TIterator Iti(*tmp_nmspace); Iti; ++Iti )
			{
				AddNamespaceGlobal(Iti.Key(), Iti.Value(), VNameSpaceGlobal);
			}
			//MACROS :D
			int max_macr=32;
			int num_macr=0;
			TMultiMap<FString,FString>* tmp_macros = ConfigCache->GetSectionPrivate( TEXT("functions"), 0, 1, MainProjectPath );
			for( TMultiMap<FString,FString>::TIterator Itii(*tmp_macros); Itii; ++Itii )
			{
				if( num_macr < max_macr )
				{
					//let's player around a little bit, shall we :)
					INT Num_Params=0;
					FString Macro_key, Macro_Rest;
					FStringDivide(Itii.Key(), FString(TEXT("(")), Macro_key, Macro_Rest);
					//below we have list of parameters :)
					Macro_Rest = FStrReplace(Macro_Rest, FString(TEXT(")")), FString(TEXT("")) );
					Macro_Rest = FStrReplace(Macro_Rest, FString(TEXT("(")), FString(TEXT("")) );
					GlobalMacros[num_macr].MacroLook=Itii.Key();
					GlobalMacros[num_macr].MacroKey=Macro_key;
					GlobalMacros[num_macr].MacroValue=Itii.Value();
					FString tmp = Macro_Rest;
					FString pt1;
					while( FStringDivide(tmp, FString(TEXT(",")), pt1, tmp) )
					{
						if( Num_Params >= 8 )
							break;
						GlobalMacros[num_macr].Parameters[Num_Params].Value=FStrTrim(NormalizeEOL2(pt1));
						Num_Params++;
						pt1=FString(TEXT(""));
					}
					if( tmp != FString(TEXT("")) )
					{
						GlobalMacros[num_macr].Parameters[Num_Params].Value=FStrTrim(NormalizeEOL2(tmp));
						Num_Params+=1;
					}
					GlobalMacros[num_macr].NumParams=Num_Params;
					num_macr++;
					//we have what we need - now let's take care of parameters
				}
			}
			GlobalMacros_num = num_macr;
		}
		else
		{
			if( appStrfind(Parms,TEXT("-bIsPackage")) ) bIsPackage = true;
			GWarn->Logf( TEXT(" Using project directory: %ls"), *Project);
			ProjectDir = Project;
			if(bIsPackage) ProjectDir = FString(TEXT("../"))+ProjectDir;
		}

		if( appStrfind(Parms,TEXT("-normalizeeol")) ) normalizeeol = true;

		if(!bUseProjectFile)
		{
			if( appStrfind(Parms,TEXT("-debug")) ) debug = true;
			if( appStrfind(Parms,TEXT("-clean")) ) clean = true;
			if( appStrfind(Parms,TEXT("-printglobals")) ) printglobals = true;
			if( appStrfind(Parms,TEXT("-make")) ) make = true;
			if( appStrfind(Parms,TEXT("-deletelog")) ) bDeleteLog = true;
			if( appStrfind(Parms,TEXT("-force")) ) bForce = true;
			if(!Parse(Parms,TEXT("makeini="), makeini)) makeini = FString(TEXT("none"));
			ParseGlobalString(Parms);
		}
		if( appStrfind(Parms,TEXT("-forcemake")) )
		{
			GWarn->Logf( TEXT(" Forced UCC make"));
			make = true;
		}

		if(debug) GWarn->Logf( TEXT(" Debug mode: on"));
		else GWarn->Logf( TEXT(" Debug mode: off"));

		if(clean) GWarn->Logf( TEXT(" Clean code: on"));
		else GWarn->Logf( TEXT(" Clean code: off"));

		if(printglobals) GWarn->Logf( TEXT(" Print globals: on"));
		else GWarn->Logf( TEXT(" Print globals: off"));
		GWarn->Logf( TEXT(" Input: %ls"), *input);
		GWarn->Logf( TEXT(" Output: %ls"), *output);

		//prints global variables
		if(printglobals)
		{
			GWarn->Logf( TEXT("------------------------------------------"));
			GWarn->Logf( TEXT(" Globals:"));
			GWarn->Logf( TEXT("") );
			PrintGlobals();
		}
		if(printnamespace)
		{
			GWarn->Logf( TEXT("------------------------------------------"));
			GWarn->Logf( TEXT(" Global namespaces:"));
			GWarn->Logf( TEXT("") );
			PrintNamespace();
		}
		if(printmacros && GlobalMacros_num > 0)
		{
			GWarn->Logf( TEXT("------------------------------------------"));
			GWarn->Logf( TEXT(" Global functions:"));
			GWarn->Logf( TEXT("") );
			PrintMacros();
		}
		// here everything takes place :)
		ProjectDir = IsDir(ConvertSlash(ProjectDir));
		input = IsDir(ConvertSlash(input));
		output = IsDir(ConvertSlash(output));
		FStringAdvExtract(ProjectDir, FString(TEXT("../")), FString(TEXT("/")), CurPackage, FString(TEXT("/")));

		FString real_input = AddSlashes(ProjectDir, input);
		FString real_output = AddSlashes(ProjectDir, output);

		const TCHAR* search_dir = ANSI_TO_TCHAR(TCHAR_TO_ANSI(*AddSlashes(ProjectDir, input+FString(TEXT("*.uc")))));

		TArray<FString> source = GFileManager->FindFiles(search_dir , 1, 0 );
		num_files = source.Num();
		GWarn->Logf( TEXT("------------------------------------------"));

		for(int i=0; i<num_files; i++)
		{
			FString t_source = real_input+source.Last(num_files-(i+1));
			FString t_output = real_output+source.Last(num_files-(i+1));
			FString S=FString(TEXT(""));
			CurFile = source.Last(num_files-(i+1));
			//GWarn->Logf(TEXT("xx:%ls"),GetClass(t_output));
			CurClass = GetClass(*t_source);
			UBOOL ParseThisOne=0;
			UBOOL bError=0;
			UBOOL bCrash=0;

			if(appLoadFileToString(S,*t_source))
			{
				FString FileToWrite = FString(TEXT("")); //empty content


				if( S.InStr( FString(TEXT("`process"))) != -1 || bForce)
				{
					GWarn->Logf( TEXT("Parsing file: %ls"), *source.Last(num_files-(i+1)));
					FileToWrite=ParseSource(S, bError, bCrash);
					num_parsed++;
					ParseThisOne=1;
					if(bCrash)
					{
						return 0;
					}
				}
				else
				{
					//we're assuming that file should not be parsed
					FileToWrite = S;
				}

				if(!bError)
				{
					// saves file
					if(appSaveStringToFile(FileToWrite,*t_output))
					{
						if(debug)
						{
							if(ParseThisOne)
								GWarn->Logf( TEXT("...saving parsed file: %ls"), *source.Last(num_files-(i+1)));
							else
								GWarn->Logf( TEXT("Saving unparsed file: %ls"), *source.Last(num_files-(i+1)));
						}
					}
					else
					{
						if(debug)
							GWarn->Logf( TEXT("...can not save file: %ls"), *source.Last(num_files-(i+1)));
					}
				}
			}
			else
			{
				GWarn->Logf( TEXT("Error loading file %ls"),*t_source);
			}
		}

		FLOAT Work = appSeconds() - StartupTime;
		GWarn->Logf( TEXT("------------------------------------------") );
		GWarn->Logf( TEXT("%i uc files found."), num_files);
		GWarn->Logf( TEXT("%i uc files parsed."), num_parsed);
		if( native_offset-DefNat > 0 )
			GWarn->Logf( TEXT("%i native functions."), native_offset-DefNat);
		GWarn->Logf( TEXT("Execution time: %f seconds"), Work);


		if(make)
		{

			FString* ErrorX=new FString(TEXT(""));
			FString App = TEXT("ucc.exe");
			FString Token = TEXT(" make -silent");
			if(makeini != FString(TEXT("none")))
					Token = Token+ TEXT(" ini=")+*ProjectDir+*makeini;
			GWarn->Logf( TEXT("------------------------------------------") );
			GWarn->Logf( TEXT("Executing: %ls"), *App+Token);

			appLaunchURL(*App, *Token, ErrorX);
		}
	}
	else
	{
		PrintHelpFile();
	}

	return 0;
	unguard;
}
/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Parses UnrealScript source in search for preprocessor directives.
 * Uses same basecode as LoadFStringToTArray.
 */
FString UParse::ParseSource(FString Text, UBOOL& bError, UBOOL& bCrash)
{
	guard(UParse::WrapText);
	FString result;
	int nested=0;				// is nested
	UBOOL SkipLine=0;			// should we skip code (used in all kind of conditional statements)
	UBOOL Compl=0;
	UBOOL ComplFnd=0;

	if(normalizeeol)
		Text=NormalizeEOL(Text);

	VLocal.Empty();
	VLocalString=FString(TEXT(""));
	VNameSpaceString=FString(TEXT(""));
	CUR_Line=1;
	REL_Line=1;
	TOT_Lines = NumLines(Text);

	while(Text.Len() > 0)
	{
		int linebreak = Text.InStr(FString(TEXT("\r\n")));
		FString tmpStr; //it'll hold current line
		FString InclusionString;
		UBOOL inc_parse = 0;
		UBOOL include_file = 0;
		UBOOL write_once = 0;
		//if we're nested only conditional statement can allow to write lines again
		if(nested <= 0 && SkipLine) SkipLine=0;

		if(linebreak == -1) //we have reached end of text
			tmpStr = Text;
		else
			tmpStr = Text.Mid(0,linebreak+2);
		// BEGIN: parses tmpStr
		if( tmpStr.InStr( FString(TEXT("`process"))) != -1 && clean) SkipLine=1;
		/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 * Predefined macros
		 */
		//macro __FILE__
		if( tmpStr.InStr( FString(TEXT("__FILE__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__FILE__")), CurFile);
			if(debug)
				GWarn->Logf( TEXT("...macro __FILE__ found. Value inserted: %ls"), *CurFile);
		}
		if( tmpStr.InStr( FString(TEXT("__LINE__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__LINE__")), FString::Printf(TEXT("%i"),CUR_Line));
			if(debug)
				GWarn->Logf( TEXT("...macro __LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),CUR_Line));
		}
		if( tmpStr.InStr( FString(TEXT("__RELATIVE_LINE__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__RELATIVE_LINE__")), FString::Printf(TEXT("%i"),REL_Line));
			if(debug)
				GWarn->Logf( TEXT("...macro __RELATIVE_LINE__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),REL_Line));
		}
		if( tmpStr.InStr( FString(TEXT("__TOTAL_LINES__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__TOTAL_LINES__")), FString::Printf(TEXT("%i"),TOT_Lines));
			if(debug)
				GWarn->Logf( TEXT("...macro __TOTAL_LINES__ found. Value inserted: %ls"), *FString::Printf(TEXT("%i"),TOT_Lines));
		}
		if( tmpStr.InStr( FString(TEXT("__FILE__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__FILE__")), CurFile);
			if(debug)
				GWarn->Logf( TEXT("...macro __FILE__ found. Value inserted: %ls"), *CurFile);
		}
		//macro __CLASS__
		if( tmpStr.InStr( FString(TEXT("__CLASS__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__CLASS__")), CurClass);
			if(debug)
				GWarn->Logf( TEXT("...macro __CLASS__ found. Value inserted: %ls"), *CurClass);
		}
		//macro __DATE__
		if( tmpStr.InStr( FString(TEXT("__DATE__"))) != -1 && !IsDirective(tmpStr))
		{
			FString CurTime = GetTime();
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__DATE__")), CurTime);
			if(debug)
				GWarn->Logf( TEXT("...macro __DATE__ found. Value inserted: %ls"), *CurTime);
		}
		//macro __SELF__
		if( tmpStr.InStr( FString(TEXT("__SELF__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__SELF__")), CurPackage);
			if(debug)
				GWarn->Logf( TEXT("...macro __SELF__ found. Value inserted: %ls"), *CurPackage);
		}
		//macro __UENGINEVERSION__
		if( tmpStr.InStr( FString(TEXT("__UENGINEVERSION__"))) != -1 && !IsDirective(tmpStr))
		{
			tmpStr = FStrReplace(tmpStr, FString(TEXT("__UENGINEVERSION__")), UENGVER);
			if(debug)
				GWarn->Logf( TEXT("...macro __UENGINEVERSION__ found. Value inserted: %ls"), *UENGVER);
		}
		//namespace
		if( ( VNameSpaceString != FString(TEXT("")) || VNameSpaceGlobal != FString(TEXT("")) ) && !SkipLine && !IsDirective(tmpStr) )
		{
			tmpStr = ReplaceNamespace(tmpStr);
		}
		//directive `remove.start
		if( tmpStr.InStr( FString(TEXT("`remove.start"))) != -1 )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `remove.start found - deleting code started"));
			nested++;
			SkipLine=1;
			write_once=!clean;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 * Conditional statements
		 */
		//directive `ifndef
		if( tmpStr.InStr( FString(TEXT("`ifndef("))) != -1 )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `ifndef found"));

			nested++;
			FString if_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString if_cmd;
			FStringExtract(if_string, FString(TEXT("`ifndef(")), FString(TEXT(")")), if_cmd);

			SVars if_var;
			if( GetVariable(if_cmd, if_var) )
			{
				SkipLine=1;
				if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to true"), *if_cmd);
			}
			else
			{
				SkipLine=0;
				if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to false"), *if_cmd);
			}

			write_once=!clean;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		//directive `ifdef
		if( tmpStr.InStr( FString(TEXT("`ifdef("))) != -1 )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `ifdef found"));

			nested++;
			FString if_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString if_cmd;
			FStringExtract(if_string, FString(TEXT("`ifdef(")), FString(TEXT(")")), if_cmd);

			SVars if_var;
			if( GetVariable(if_cmd,if_var) )
			{
				SkipLine=0;
				if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to true"), *if_cmd);
			}
			else
			{
				SkipLine=1;
				if(debug) GWarn->Logf( TEXT("......statement (%ls) evaluates to false"), *if_cmd);
			}

			write_once=!clean;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		//directive `if
		if( tmpStr.InStr( FString(TEXT("`if("))) != -1 )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `if found"));

			ParseCondition(tmpStr, nested, SkipLine);
			if(!SkipLine) ComplFnd=1;
			write_once=!clean;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		//directive `else
		if( tmpStr.InStr( FString(TEXT("`else"))) != -1 )
		{
			if(tmpStr.InStr(FString(TEXT("`else if"))) != -1)
			{
				if(SkipLine)
				{
					FString tstr = FStrReplace(tmpStr,FString(TEXT("else ")), FString(TEXT("")));
					int nested_null;
					ParseCondition(tstr, nested_null, SkipLine);
					if(!SkipLine) ComplFnd=1;
				}
				else
					SkipLine=1;
				Compl=1;
			}
			else
			{
				if(!Compl)
					SkipLine=!SkipLine;
				else
					SkipLine=ComplFnd;
			}
			write_once=!clean;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		//directive `endif
		if( tmpStr.InStr( FString(TEXT("`endif"))) != -1 )
		{
			if(nested > 0) nested--;
			SkipLine=0;
			Compl=0;
			write_once=!clean;
			ComplFnd=0;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		//directive `remove.end
		if( tmpStr.InStr( FString(TEXT("`remove.end"))) != -1 )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `remove.end found - deleting code ended"));
			if(nested > 0) nested--;
			SkipLine=0;
			Compl=0;
			write_once=!clean;
			ComplFnd=0;
			if(!write_once) tmpStr = FString(TEXT(""));
		}
		if(bDeleteLog && tmpStr.InStr(FString(TEXT("log("))) != -1)
		{
			FString delStr = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			INT lgBegin = delStr.InStr(FString(TEXT("log(")));
			INT lgEnd = delStr.InStr(FString(TEXT(");")));
			FString test = delStr.Mid(lgBegin,lgEnd+3);
			GWarn->Logf( TEXT("...UScript log function found and deleted."));
			tmpStr = FStrReplace(tmpStr,test,FString(TEXT("")));
			if(FStrTrim(delStr).Len() <= 0)
			{
				tmpStr = FString(TEXT(""));
			}
		}
		/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 * Directives
		 */
		//directive `include
		if( tmpStr.InStr( FString(TEXT("`include("))) != -1 && !SkipLine )
		{
			FString inc_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString inc_cmd;
			FStringExtract(inc_string, FString(TEXT("`include(")), FString(TEXT(")")), inc_cmd);

			FString inc_file = inc_cmd;
			FString inc_fparse = FString(TEXT("false"));
			if(FStringDivide(inc_cmd, FString(TEXT(",")), inc_file, inc_fparse))
			{
				if(inc_fparse == FString(TEXT("true")))
				{
					inc_parse = 1;
				}
			}
			if( ( VNameSpaceString != FString(TEXT("")) || VNameSpaceGlobal != FString(TEXT("")) ))
			{
				inc_file = ReplaceNamespace(inc_file);
			}
			if(debug)
			{
				GWarn->Logf( TEXT("...directive `include found"));
				GWarn->Logf( TEXT("......embedding file: %ls"), *inc_file);
				GWarn->Logf( TEXT(".........parsing file: %ls"), *inc_fparse);
			}
			if(appLoadFileToString(InclusionString,*AddSlashes(ProjectDir, inc_file)))
			{
				if(debug)
					GWarn->Logf( TEXT(".........successful"));
				include_file = 1;
			}
			else if(debug)
					GWarn->Logf( TEXT(".........failed"));
			if(clean) SkipLine=1;
		}
		//directive `require
		if( tmpStr.InStr( FString(TEXT("`require("))) != -1 && !SkipLine )
		{
			FString inc_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString inc_cmd;
			FStringExtract(inc_string, FString(TEXT("`require(")), FString(TEXT(")")), inc_cmd);

			FString inc_file = inc_cmd;
			FString inc_fparse = FString(TEXT("false"));
			if(FStringDivide(inc_cmd, FString(TEXT(",")), inc_file, inc_fparse))
			{
				if(inc_fparse == FString(TEXT("true")))
				{
					inc_parse = 1;
				}
			}
			if( ( VNameSpaceString != FString(TEXT("")) || VNameSpaceGlobal != FString(TEXT("")) ))
			{
				inc_file = ReplaceNamespace(inc_file);
			}
			if(debug)
			{
				GWarn->Logf( TEXT("...directive `require found"));
				GWarn->Logf( TEXT("......embedding file: %ls"), *inc_file);
				GWarn->Logf( TEXT(".........parsing file: %ls"), *inc_fparse);
			}
			if(appLoadFileToString(InclusionString,*AddSlashes(ProjectDir, inc_file)))
			{
				if(debug)
					GWarn->Logf( TEXT(".........successful"));
				include_file = 1;
			}
			else
			{
				if(debug)
				{
					GWarn->Logf( TEXT(".........failed."));
					GWarn->Logf( TEXT("............Can not parse class %ls."), *CurClass);
				}
				bError=1;
				break;
			}
			if(clean) SkipLine=1;
		}
		//directive `define
		if( tmpStr.InStr( FString(TEXT("`define("))) != -1 && !SkipLine )
		{
			FString def_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString def_cmd;
			FStringExtract(def_string, FString(TEXT("`define(")), FString(TEXT(")")), def_cmd);

			FString def_name = def_cmd;
			FString def_val = FString(TEXT("null"));

			if(FStringDivide(def_cmd, FString(TEXT(",")), def_name, def_val))
			{
				if(debug)
				{
					GWarn->Logf( TEXT("...directive `define found. Local variable %ls defined."), *def_name);
					GWarn->Logf( TEXT("......defined value equals %ls."), *def_val);
				}
			}
			else if(debug)
				GWarn->Logf( TEXT("...directive `define found. Local variable %ls defined."), *def_name);
			AddLocalVar(def_name, def_val, VLocalString);
			//SVars tmp_var;
			//tmp_var.VName = def_name;
			//tmp_var.VValue = def_val;
			//VLocal.AddItem(tmp_var);

			if(clean) SkipLine=1;
		}
		//directive `namespace
		if( tmpStr.InStr( FString(TEXT("`namespace("))) != -1 && !SkipLine )
		{
			FString nspc_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString nspc_cmd;
			FStringExtract(nspc_string, FString(TEXT("`namespace(")), FString(TEXT(")")), nspc_cmd);

			FString nspc_name = nspc_cmd;
			FString nspc_val = FString(TEXT("null"));

			if(FStringDivide(nspc_cmd, FString(TEXT(",")), nspc_name, nspc_val))
			{
				if(debug)
				{
					GWarn->Logf( TEXT("...directive `namespace found. Namespace \"%ls\" defined as %ls."), *nspc_val, *nspc_name );
				}
				AddNamespace(nspc_name, nspc_val, VNameSpaceString);
			}
			else if(debug)
				GWarn->Logf( TEXT("...NOTICE: directive `namespace ignored (namespace not defined)."), *nspc_name);

			if(clean) SkipLine=1;
		}
		//we have macros
		INT CurMacro=-1;
		if( HasMacros(tmpStr, CurMacro) && !SkipLine )
		{
			FString Macro_Rest = FStrReplace(tmpStr, GlobalMacros[CurMacro].MacroKey+FString(TEXT("(")), FString(TEXT("")) );
			Macro_Rest = FTabRemove( Macro_Rest );
			Macro_Rest = FStrTrim( Macro_Rest );
			//list of parameters :)
			Macro_Rest = FStrReplace(Macro_Rest, FString(TEXT(");")), FString(TEXT("")) );
			FString ValToWrite=GlobalMacros[CurMacro].MacroValue;
			if(debug)
			{
				GWarn->Logf( TEXT("...function detected: %ls."), *GlobalMacros[CurMacro].MacroLook );
				GWarn->Logf( TEXT("......key: %ls"), *GlobalMacros[CurMacro].MacroKey );
				GWarn->Logf( TEXT("......value: %ls"), *GlobalMacros[CurMacro].MacroValue );
				GWarn->Logf( TEXT("......parameters: %i"), GlobalMacros[CurMacro].NumParams );
			}

			FString pt1, pt2;
			INT Num_Params=0;
			while( FStringDivide(Macro_Rest, FString(TEXT(",")), pt1, pt2) )
			{
				if( Num_Params >= 8 || pt1 == FString(TEXT("")) )
					break;
				ValToWrite = FStrReplace(ValToWrite, GlobalMacros[CurMacro].Parameters[Num_Params].Value, NormalizeEOL2(pt1));
				Num_Params++;
				pt1 = FString(TEXT(""));
				Macro_Rest = pt2;
			}
			if( Macro_Rest != FString(TEXT("")))
			{
				ValToWrite = FStrReplace(ValToWrite, GlobalMacros[CurMacro].Parameters[Num_Params].Value , NormalizeEOL2(Macro_Rest));
			}
			FString tmpStr2 = NormalizeEOL2(ValToWrite)+FString(TEXT(";"))+FString(TEXT("\r\n"));
			tmpStr = FStrReplace(tmpStr, FTabRemove(FStrTrim(tmpStr)), tmpStr2);
			if( ( VNameSpaceString != FString(TEXT("")) || VNameSpaceGlobal != FString(TEXT("")) ))
			{
				tmpStr = ReplaceNamespace(tmpStr);
			}
			tmpStr = MacroReplace(tmpStr);
		}
		//directive `undef
		if( tmpStr.InStr( FString(TEXT("`undef("))) != -1 && !SkipLine)
		{
			FString undef_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString undef_cmd;
			FStringExtract(undef_string, FString(TEXT("`undef(")), FString(TEXT(")")), undef_cmd);
			if(debug)
				GWarn->Logf( TEXT("...directive `undef found. Local variable %ls undefined."), *undef_cmd);

			DelLocalVar(undef_cmd);

			if(clean) SkipLine=1;
		}
		//directive `error
		if( tmpStr.InStr( FString(TEXT("`error("))) != -1 && !SkipLine )
		{
			FString err_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString err_cmd;
			FStringExtract(err_string, FString(TEXT("`error(")), FString(TEXT(")")), err_cmd);

			FString err_msg = err_cmd;
			FString err_crash = FString(TEXT("false"));
			if(FStringDivide(err_cmd, FString(TEXT(",")), err_msg, err_crash))
			{
				if(err_crash == FString(TEXT("true")))
				{
					inc_parse = 1;
				}
			}
			if( err_crash == FString(TEXT("true")) )
			{
				GWarn->Logf( TEXT("...ERROR: %ls"), *err_msg);
				bError=1;
			}
			else
			{
				GWarn->Logf( TEXT("...CRITICAL ERROR: %ls"), *err_msg);
				bCrash=1;
			}
			break;
		}
		//directive `warn
		if( tmpStr.InStr( FString(TEXT("`warn("))) != -1 && !SkipLine )
		{
			FString warn_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString warn_msg;
			FStringExtract(warn_string, FString(TEXT("`warn(")), FString(TEXT(")")), warn_msg);
			GWarn->Logf( TEXT("...WARNING: %ls"), *warn_msg);
			if(clean) SkipLine=1;
		}
		//directive `log
		if( tmpStr.InStr( FString(TEXT("`log("))) != -1 && !SkipLine )
		{
			FString log_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString log_msg;
			FStringExtract(log_string, FString(TEXT("`log(")), FString(TEXT(")")), log_msg);
			GWarn->Logf( TEXT("...%ls"), *log_msg);
			if(clean) SkipLine=1;
		}
		//directive `createexec - for easy importing large number of textures/sounds
		if( tmpStr.InStr( FString(TEXT("`import("))) != -1  && !SkipLine )
		{
			if(debug)
				GWarn->Logf( TEXT("...directive `import found"));

			FString exc_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString exc_cmd;
			FStringExtract(exc_string, FString(TEXT("`import(")), FString(TEXT(")")), exc_cmd);
			SImportInfo ImpInfo = GetExecParams(exc_cmd);

			FString exc_input = IsDir(ImpInfo.Directory)+FString(TEXT("*."))+ImpInfo.Extension;

			const TCHAR* exc_dir = ANSI_TO_TCHAR(TCHAR_TO_ANSI(*AddSlashes(ProjectDir, exc_input)));

			TArray<FString> exc_files = GFileManager->FindFiles(exc_dir , 1, 0 );
			int exc_num_files = exc_files.Num();

			FString exc_out = FString(TEXT(""));
			for(int i=0; i<exc_num_files; i++)
			{
				FString exc_imp_file = exc_files.Last(exc_num_files-(i+1));
				FString exc_imm = IsDir(ImpInfo.Directory)+exc_imp_file;

				if(ImpInfo.Extension.Caps() == FString(TEXT("UTX")) || ImpInfo.Extension.Caps() == FString(TEXT("UAX")))
				{
					FString tmpOut = FString(TEXT("#exec "))+ImpInfo.Type+FString(TEXT(" obj load FILE=\""))+exc_imm+FString(TEXT("\""));

					if( ImpInfo.Package != FString(TEXT("")) )
						tmpOut += FString(TEXT(" PACKAGE="))+ImpInfo.Package;
					else
					{
						tmpOut += FString(TEXT(" PACKAGE="))+FStrReplace(exc_imp_file,FString(TEXT("."))+ImpInfo.Extension,FString(TEXT("")));
					}
					tmpOut += FString(TEXT("\r\n"));
					exc_out = exc_out + tmpOut;
				}
				else
				{
					FString tmpOut = FString(TEXT("#exec "))+ImpInfo.Type+FString(TEXT(" IMPORT NAME="))+FStrReplace(exc_imp_file,FString(TEXT("."))+ImpInfo.Extension,FString(TEXT("")))+FString(TEXT(" FILE=\""))+exc_imm+FString(TEXT("\""));
					//optional values
					if( ImpInfo.Group != FString(TEXT("")) )
						tmpOut += FString(TEXT(" GROUP="))+ImpInfo.Group;
					if( ImpInfo.LodSet != FString(TEXT("")) )
						tmpOut += FString(TEXT(" LODSET="))+ImpInfo.LodSet;
					if( ImpInfo.LodSet != FString(TEXT("")) )
						tmpOut += FString(TEXT(" FLAGS="))+ImpInfo.Flags;
					if( ImpInfo.Package != FString(TEXT("")) )
						tmpOut += FString(TEXT(" PACKAGE="))+ImpInfo.Package;
					tmpOut += FString(TEXT("\r\n"));
					exc_out = exc_out + tmpOut;

				}
				if(debug)
				{
					GWarn->Logf( TEXT("......creating #exec for file: %ls"), *exc_imp_file);
				}
			}
			if(clean) tmpStr = FString(TEXT(""));
			write_once = true;
			tmpStr = tmpStr + exc_out;
		}
		/**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		 * Inline directives
		 */
		//directive `write
		if( tmpStr.InStr( FString(TEXT("`write("))) != -1 && !SkipLine )
		{
			FString wrt_string = FStrReplace(tmpStr, FString(TEXT("\r\n")), FString(TEXT("")));
			FString wrt;
			FStringAdvExtract(wrt_string, FString(TEXT("`write(")), FString(TEXT(")")), wrt);

			FString rpl_str=FString(TEXT("`write("))+wrt+FString(TEXT(")"));

			if(debug)
				GWarn->Logf( TEXT("...directive `write found"));

			if(wrt.InStr(FString(TEXT("?"))) != -1)
			{
				FString wrt_left, wrt_right;
				FString wrt_true, wrt_false;

				//we're dividing ternary operation into two parts
				FStringDivide(wrt, FString(TEXT("?")), wrt_left, wrt_right);
				//we're splitting right part
				FStringDivide(wrt_right, FString(TEXT(":")), wrt_true, wrt_false);

				FString div=FString(TEXT("none"));
				if(wrt_left.InStr(FString(TEXT("=="))) != -1)
					div = FString(TEXT("=="));
				else if(wrt_left.InStr(FString(TEXT("<>"))) != -1)
					div = FString(TEXT("<>"));
				else if(wrt_left.InStr(FString(TEXT("<"))) != -1)
					div = FString(TEXT("<"));
				else if(wrt_left.InStr(FString(TEXT(">"))) != -1)
					div = FString(TEXT(">"));
				else if(wrt_left.InStr(FString(TEXT("<="))) != -1)
					div = FString(TEXT("<="));
				else if(wrt_left.InStr(FString(TEXT(">="))) != -1)
					div = FString(TEXT(">="));

				if(div != FString(TEXT("none")))
				{
					FString wrt_v1, wrt_v2;
					FStringDivide(wrt_left, div, wrt_v1, wrt_v2);

					SVars wrt_var1, wrt_var2;
					GetVariable(wrt_v1,wrt_var1);
					GetVariable(wrt_v2,wrt_var2);
					if(IsTrue(wrt_var1.VValue, wrt_var2.VValue, div))
					{
						SVars tmpi_var1;
						GetVariable(wrt_true,tmpi_var1);
						tmpStr = FStrReplace(tmpStr,rpl_str,tmpi_var1.VValue);
						if(debug)
						{
							GWarn->Logf( TEXT("......inline statement (%ls) evaluates to true"), *wrt_left);
							GWarn->Logf( TEXT("......value inserted: %ls"), *tmpi_var1.VValue);
						}
					}
					else
					{
						SVars tmpi_var1;
						GetVariable(wrt_false,tmpi_var1);
						tmpStr = FStrReplace(tmpStr,rpl_str,tmpi_var1.VValue);
						if(debug)
						{
							GWarn->Logf( TEXT("......inline statement (%ls) evaluates to false"), *wrt_left);
							GWarn->Logf( TEXT("......value inserted: %ls"), *tmpi_var1.VValue);
						}
					}
				}
				else
				{
					SVars wrt_var;
					if( GetVariable(wrt_left,wrt_var) )
					{
						SVars tmpi_var1;
						GetVariable(wrt_true,tmpi_var1);
						tmpStr = FStrReplace(tmpStr,rpl_str,tmpi_var1.VValue);
						if(debug)
						{
							GWarn->Logf( TEXT("......inline statement (%ls) evaluates to true"), *wrt_left);
							GWarn->Logf( TEXT("......value inserted: %ls"), *tmpi_var1.VValue);
						}
					}
					else
					{
						SVars tmpi_var1;
						GetVariable(wrt_false,tmpi_var1);
						tmpStr = FStrReplace(tmpStr,rpl_str,tmpi_var1.VValue);
						if(debug)
						{
							GWarn->Logf( TEXT("......inline statement (%ls) evaluates to false"), *wrt_left);
							GWarn->Logf( TEXT("......value inserted: %ls"), *tmpi_var1.VValue);
						}
					}
				}
			}
			else
			{
				SVars tmpi_var1;
				GetVariable(wrt,tmpi_var1);
				tmpStr = FStrReplace(tmpStr,rpl_str,tmpi_var1.VValue);
				if(debug)
					GWarn->Logf( TEXT("......value inserted: %ls"), *tmpi_var1.VValue);
			}
		}

		//if( SkipLine && nested > 0 )
			REL_Line++;

		if(!SkipLine || write_once)
		{
			if(tmpStr.InStr( FString(TEXT("`"))) != -1 && tmpStr.InStr( FString(TEXT("//`"))) == -1)
				tmpStr=FStrReplace(tmpStr,FString(TEXT("`")),FString(TEXT("//`")));
			result += tmpStr;
			CUR_Line++;
			//REL_Line++;
		}
		else if(SkipLine && nested > 0 && !write_once && !clean)
		{
			result += FString(TEXT("//")) + tmpStr;
			REL_Line++;
		}
		if(include_file && !inc_parse)
		{
			result += InclusionString;
			//REL_Line += NumLines(InclusionString);
			CUR_Line += NumLines(InclusionString);
		}
		// END: parses tmpStr
		if(linebreak == -1)
		{
			if(include_file && inc_parse)
			{
				Text = InclusionString + Text;
				REL_Line -= NumLines(InclusionString);
				//CUR_Line += NumLines(InclusionString);
			}
			Text = FString(TEXT(""));
		}
		else
		{
			Text = Text.Mid(linebreak+2);
			if(include_file && inc_parse)
			{
				Text = InclusionString + Text;
				REL_Line -= NumLines(InclusionString);
				//CUR_Line += NumLines(InclusionString);
			}
		}
	}

	return result;
	unguard;
}

/*=============================================================================
	Package Browser: Browse all packages.

	Revision history:
		* Created by Marco

=============================================================================*/
#pragma once

// Grab absolute path of the file for display.
static void GetFileInfo(const TCHAR* FileName, const TCHAR* FilePath, FString& InfoStr, FString& FullPath)
{
	guardSlow(GetFileInfo);

	const TCHAR* S = FilePath;
	if (*S && S[1] == ':') // Full drive path.
	{
		while (*S && *S != '*')
			++S;
		FullPath = FString(FilePath, S) + FileName;
	}
	else
	{
		while (*S && *S != '*')
			++S;
		FString LocalDir = FString(appBaseDir()) * FString(FilePath, S);

		// Strip out sub-directory directives.
		const TCHAR* Start = *LocalDir;
		S = Start;
		const TCHAR* LastPath = S;
		const TCHAR* PrevPath = S;
		while (*S)
		{
			if (S[0] == '.' && (S[1] == '/' || S[1] == '\\'))
			{
				LocalDir = FString(Start, PrevPath) + FString(S + 2);
				S = LastPath = PrevPath = Start = *LocalDir;
				continue;
			}
			else if (*S == '/' || *S == '\\')
			{
				PrevPath = LastPath;
				LastPath = S + 1;
			}
			++S;
		}
		FullPath = LocalDir + FileName;
	}
	
	InfoStr = FString::Printf(TEXT("%ls [%ls]"), FileName, *FullPath);
	unguardSlow;
}

struct FPackageListItem
{
	FString Info, CompStr, FullFilePath;
	UBOOL bIsMapFile;

	FPackageListItem(const TCHAR* FileName, const TCHAR* FilePath)
	{
		GetFileInfo(FileName, FilePath, Info, FullFilePath);
		CompStr = FString(FileName).Caps();
		bIsMapFile = ((CompStr.Len() >= 4) && (CompStr.Right(4) == TEXT(".UNR")));
	}
	inline UBOOL PassesFilter(const TCHAR* Filter) const
	{
		guardSlow(FPackageListItem::PassesFilter);
		return (CompStr.InStr(Filter) >= 0);
		unguardSlow;
	}
};

INT Compare(const FPackageListItem& A, const FPackageListItem& B)
{
	return appStricmp(*A.Info, *B.Info);
}

class DlgPackageBrowser : public WCustomWindowBase
{
	WListBox PackageList;
	WButton LoadButton;
	WEdit Filter;
	FString NameFilter;
	TArray<FPackageListItem> FileList;
	TArray<INT> RefList;

public:
	DlgPackageBrowser()
		: WCustomWindowBase(0)
		, PackageList(this)
		, LoadButton(this,0,FDelegate(this, (TDelegate)&DlgPackageBrowser::OnLoadPackages))
		, Filter(this)
	{
		PerformCreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Package Browser"), (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU), 150, 250, 650, 600, GhwndEditorFrame, NULL, NULL);
		Show(1);
		InvalidateRect(hWnd, 0, TRUE);
		PackageList.OpenWindow(TRUE, TRUE, TRUE, FALSE);
		PackageList.DoubleClickDelegate = FDelegate(this, (TDelegate)&DlgPackageBrowser::OnLoadPackages);

		FRect R = GetClientRect();
		PackageList.MoveWindow(FRect(10, 22, R.Width()-10, R.Height()-24), TRUE);
		Filter.OpenWindow(TRUE, FALSE, FALSE, 0, 0, 152, R.Height() - 24, R.Width() - 162, 20);
		Filter.ChangeDelegate = FDelegate(this, (TDelegate)&DlgPackageBrowser::OnFilterText);
		LoadButton.OpenWindow(TRUE, 16, R.Height() - 24, 128, 20, TEXT("Load packages"));

		BuildFileList();
		RefreshList();
	}
	~DlgPackageBrowser()
	{
		MaybeDestroy();
	}
	void OnClose()
	{
		guard(DlgPackageBrowser::OnClose);
		Show(0);
		throw TEXT("NoRoute");
		unguard;
	}
	void OnLoadPackages();

	void BuildFileList()
	{
		guard(DlgPackageBrowser::BuildFileList);
		INT j;
		for (INT i = 0; i < GSys->Paths.Num(); ++i)
		{
			TArray<FString> FileAr = GFileManager->FindFiles(*GSys->Paths(i), TRUE, FALSE);
			for (j = 0; j < FileAr.Num(); ++j)
				new (FileList) FPackageListItem(*FileAr(j), *GSys->Paths(i));
		}
		Sort(FileList);
		unguard;
	}
	void RefreshList()
	{
		guard(DlgPackageBrowser::RefreshList);
		PackageList.Empty();
		RefList.EmptyNoRealloc();
		if (NameFilter.Len())
		{
			FString CFilter = NameFilter.Caps();
			for (INT i = 0; i < FileList.Num(); ++i)
				if (FileList(i).PassesFilter(*CFilter))
				{
					RefList.AddItem(i);
					PackageList.AddString(*FileList(i).Info);
				}
		}
		else
		{
			for (INT i = 0; i < FileList.Num(); ++i)
			{
				RefList.AddItem(i);
				PackageList.AddString(*FileList(i).Info);
			}
		}
		unguard;
	}
	void OnFilterText()
	{
		guard(DlgPackageBrowser::OnFilterText);
		NameFilter = Filter.GetText();
		RefreshList();
		unguard;
	}
};

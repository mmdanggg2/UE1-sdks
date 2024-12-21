/*=============================================================================
	MRUList : Helper class for handling MRU lists
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

#define MRU_MAX_ITEMS 8

class MRUList
{
	MRUList()
	{
		check(0);	// wrong constructor
	}

public:
	MRUList( FString InINISection )
	{
		NumItems = 0;
		INISection = InINISection;
	}
	~MRUList()
	{}

	FString Items[MRU_MAX_ITEMS], INISection;
	int NumItems{};

	void ReadINI()
	{
		guard(MRUList::ReadINI);
		NumItems = 0;
		for( int mru = 0 ; mru < MRU_MAX_ITEMS ; mru++ )
		{
			FString Item;
			GConfig->GetString(*INISection, *(FString::Printf(TEXT("MRUItem%d"), mru)), Item, GUEdIni);

			// If we get a item, add it to the top of the MRU list.
			if( Item.Len() )
			{
				Items[mru] = Item;
				NumItems++;
			}
		}
		unguard;
	}
	void WriteINI()
	{
		guard(MRUList::WriteINI);
		for( int mru = 0 ; mru < NumItems ; mru++ )
			GConfig->SetString(*INISection, *(FString::Printf(TEXT("MRUItem%d"), mru)), *Items[mru], GUEdIni);
		unguard;
	}

	// Adds a item to the MRU list.  New itemss are added to the bottom of the list.
	void AddItem( FString Item )
	{
		guard(MRUList::AddItem);
		// See if the item already exists in the list...
		int mru;
		for( mru = 0 ; mru < NumItems ; mru++ )
			if( Items[mru] == Item )
				return;

		if( Item.Len() )
		{
			NumItems++;
			NumItems = min(NumItems, MRU_MAX_ITEMS);

			for( mru = MRU_MAX_ITEMS - 1 ; mru > 0 ; mru-- )
				Items[mru] = Items[mru - 1];
			Items[0] = Item;
		}

		unguard;
	}

	// Adds all currently known MRU items to the "File" menu.  This completely
	// replaces the windows menu with a new one.
	void AddToMenu( HWND hWnd, HMENU Menu, UBOOL bHasExit = 0 )
	{
		guard(MRUList::AddToMenu);

		::LockWindowUpdate( hWnd );

		// Get the file menu - this is assumed to be the first submenu.
		HMENU FileMenu = GetSubMenu( Menu, 0 );
		if( !FileMenu )
			return;

		// Destroy all MRU items on the menu, as well as the seperate and "Exit".
		DeleteMenu( FileMenu, ID_FileExit, MF_BYCOMMAND );
		DeleteMenu( FileMenu, IDMN_MRU_SEP, MF_BYCOMMAND );
		int x;
		for( x = 0 ; x < MRU_MAX_ITEMS ; x++ )
			DeleteMenu( FileMenu, IDMN_MRU1 + x, MF_BYCOMMAND );

		// Add any MRU items we have to the menu, and add an "Exit" option if
		// requested.
		MENUITEMINFOA mif;
		TCHAR tchItem[256] = TEXT("\0");

		mif.cbSize = sizeof(MENUITEMINFO);
		mif.fMask = MIIM_TYPE | MIIM_ID;
		mif.fType = MFT_STRING;

		for( x = 0 ; x < NumItems ; x++ )
		{
			mif.dwTypeData = const_cast<ANSICHAR*>(appToAnsi(*FString::Printf(TEXT("&%d %ls"), x + 1, *Items[x])));
			mif.wID = IDMN_MRU1 + x;

			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		// Only add this seperator if we actually have MRU items... looks weird otherwise.
		if( NumItems && bHasExit )
		{
			mif.fType = MFT_SEPARATOR;
			mif.wID = IDMN_MRU_SEP;
			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		if( bHasExit )
		{
			mif.fType = MFT_STRING;
			mif.dwTypeData = "Exit";
			mif.wID = ID_FileExit;
			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		::LockWindowUpdate( NULL );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

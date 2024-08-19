/*=============================================================================
	ViewportWindowContainer.h: Encapsulates a UViewport that can be embedded in
	a WWindow. This class provides support for viewport construction, renderer
	switching and WWindow renderer menu updating.
	
	Copyright 2020 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

class WViewportWindowContainer
{
public:
	UViewport* pViewport;
	FString ViewportName, ConfigName;
	HWND ViewportOwnerWindow;

	static TArray<FString> GViewportRenderers;

	WViewportWindowContainer(FString InViewportName, FString InConfigName)
	:	pViewport			(NULL)
	,	ViewportName		(InViewportName)
	,	ConfigName			(InConfigName)
	,	ViewportOwnerWindow	(NULL)
	{}

	static void StaticInit(ULevel* Level, HWND TopLevelWindow)
	{
		if (GViewportRenderers.Num() == 0)
		{
			debugf(TEXT("Testing supported renderers for level Viewports"));			
			GViewportRenderers.AddItem(TEXT("SoftDrv.SoftwareRenderDevice"));
			GViewportRenderers.AddItem(TEXT("OpenGLDrv.OpenGLRenderDevice"));
			GViewportRenderers.AddItem(TEXT("D3DDrv.D3DRenderDevice"));
			GViewportRenderers.AddItem(TEXT("D3D9Drv.D3D9RenderDevice"));
			GViewportRenderers.AddItem(TEXT("XOpenGLDrv.XOpenGLRenderDevice"));

			AActor* TestViewActor = nullptr;

			TArray<FRegistryObjectInfo> Classes;
			UObject::GetRegistryObjects(Classes, UClass::StaticClass(), URenderDevice::StaticClass(), 0);
			for (TArray<FRegistryObjectInfo>::TIterator It(Classes); It && GViewportRenderers.Num() < 10; ++It)
			{
				// check if it already exists
				INT Index;
				if (!GViewportRenderers.FindItem(It->Object, Index))
				{
					// Check if this one is blacklisted
					UBOOL UEDCompatible;
					if (GConfig->GetBool(*It->Object, TEXT("UEDCompatible"), UEDCompatible, GUEDIni) )
					{
						if (UEDCompatible)
							GViewportRenderers.AddItem(It->Object);
						continue;
					}

					// doesn't exist yet. Test if the viewport supports this rendev
					FString TestDevice = It->Object;
					debugf(TEXT("Testing renderer %ls"), *TestDevice);

					// flag it as incompatible while we test it. That way, if the renderer crashes during initialization, it will not be tested again in the future
					GConfig->SetBool(*It->Object, TEXT("UEDCompatible"), FALSE, GUEDIni);
					GConfig->Flush(0);
					
					UViewport* Viewport = GEditor->Client->NewViewport(TEXT("TestViewport"));
					if ( !Viewport )
						appErrorf( TEXT("Unable to create Viewport for testing render device %s"), *TestDevice);
					Level->SpawnViewActor(Viewport);
					TestViewActor = Viewport->Actor;
					Viewport->Actor->ShowFlags = SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes;
					Viewport->Actor->RendMap = REN_DynLight;
					Viewport->Input->Init(Viewport);

					// v469b: some legacy render devices will crash the editor, blacklist them manually here
					// This method allows the user to set UEDCompatible and try the render device anyways.
					if (appStricmp(*TestDevice,TEXT("GlideDrv.GlideRenderDevice")) &&
						appStricmp(*TestDevice, TEXT("MetalDrv.MetalRenderDevice")) &&
						appStricmp(*TestDevice, TEXT("SglDrv.SglRenderDevice")))
					{
						Viewport->OpenWindow(TopLevelWindow, 0, 10, 10, 0, 0, *TestDevice);
					}

					if ( Viewport->RenDev && FObjectPathName(Viewport->RenDev->GetClass()) == TestDevice)
					{					
						debugf(TEXT("This renderer appears to work"));
						GViewportRenderers.AddItem(TestDevice);
						GConfig->SetBool(*It->Object, TEXT("UEDCompatible"), TRUE, GUEDIni);
						GConfig->Flush(0);
					}
					else
					{
						debugf(TEXT("This renderer does not work in UEd"));												
					}

					// stijn: reset Player so this viewactor can be reused for the next viewport
					Viewport->Actor->Player = NULL;
					Viewport->CloseWindow();
					delete Viewport;
				}
			}

			if (TestViewActor)
				Level->DestroyActor(TestViewActor);
		}
	}

	void UpdateRendererMenu(HMENU l_menu, BOOL IncludeCustomRenderers)
	{
		if (!pViewport || !pViewport->RenDev)
			return;

		CheckMenuItem(l_menu, IDMN_RD_SOFTWARE, (!appStrcmp(TEXT("Class SoftDrv.SoftwareRenderDevice"), *FObjectFullName(pViewport->RenDev->GetClass())) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(l_menu, IDMN_RD_OPENGL, (!appStrcmp(TEXT("Class OpenGLDrv.OpenGLRenderDevice"), *FObjectFullName(pViewport->RenDev->GetClass())) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(l_menu, IDMN_RD_XOPENGL, (!appStrcmp(TEXT("Class XOpenGLDrv.XOpenGLRenderDevice"), *FObjectFullName(pViewport->RenDev->GetClass())) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(l_menu, IDMN_RD_D3D, (!appStrcmp(TEXT("Class D3DDrv.D3DRenderDevice"), *FObjectFullName(pViewport->RenDev->GetClass())) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(l_menu, IDMN_RD_D3D9, (!appStrcmp(TEXT("Class D3D9Drv.D3D9RenderDevice"), *FObjectFullName(pViewport->RenDev->GetClass())) ? MF_CHECKED : MF_UNCHECKED));

		for (INT i = 0; i < (IncludeCustomRenderers ? GViewportRenderers.Num() : IDMN_RD_CUSTOM0 - IDMN_RD_SOFTWARE); ++i)
		{
			UINT CheckedFlag = pViewport->RenDev && !appStrcmp(*FObjectPathName(pViewport->RenDev->GetClass()), *GViewportRenderers(i)) ? MF_CHECKED : MF_UNCHECKED;
			if (i >= IDMN_RD_CUSTOM0 - IDMN_RD_SOFTWARE)
				AppendMenu(l_menu, MF_ENABLED | MF_STRING | CheckedFlag, IDMN_RD_CUSTOM0 + i - 5, *GViewportRenderers(i).Left(GViewportRenderers(i).InStr(".")));
			else
				CheckMenuItem(l_menu, IDMN_RD_SOFTWARE + i, CheckedFlag);
		}
	}

	void SwitchRenderer(INT Command, BOOL ReplaceViewportWindow)
	{
		switch(Command)
		{
			case IDMN_RD_SOFTWARE:
			case IDMN_RD_OPENGL:
			case IDMN_RD_D3D:
			case IDMN_RD_D3D9:
			case IDMN_RD_XOPENGL:
			case IDMN_RD_CUSTOM0:
			case IDMN_RD_CUSTOM1:
			case IDMN_RD_CUSTOM2:
			case IDMN_RD_CUSTOM3:
			case IDMN_RD_CUSTOM4:
			{
				if (GViewportRenderers.Num() < Command - IDMN_RD_SOFTWARE)
				{
					debugf(TEXT("Selected an invalid renderer"));
					break;
				}

				if (ReplaceViewportWindow)
				{
					FName	 OldGroup	= pViewport->Group;
					UObject* OldMiscRes = pViewport->MiscRes;
					INT		 OldSizeX	= pViewport->SizeX;
					INT		 OldSizeY	= pViewport->SizeY;
					UCanvas* OldCanvas	= pViewport->Canvas;

					// stijn: switching rendevs within an existing viewport often leads to crashes.
					// Instead, we recreate the viewport with the new rendev here.
					FString NewDevice	= *GViewportRenderers(Command - IDMN_RD_SOFTWARE);
					GConfig->SetString(*ConfigName, TEXT("Device"), *NewDevice, GUEDIni);

					// Detach the existing ViewActor from the old viewport
					ACamera* OldViewActor = Cast<ACamera>(pViewport->Actor);

					// The ViewActor might have the old viewport set as its player.
					// We should update that after recreating the viewport.
					bool UpdatePlayer	= (OldViewActor && OldViewActor->Player == pViewport);

					// Null out these refs so the GC doesn't freak out
					pViewport->Actor	= NULL;
					pViewport->Canvas	= NULL;

					pViewport->CloseWindow();
					delete pViewport;

					// Recreate the viewport
					pViewport			= GEditor->Client->NewViewport(*ViewportName);
					pViewport->Actor	= OldViewActor;
					if (UpdatePlayer)
						pViewport->Actor->Player = pViewport;
					pViewport->Input->Init(pViewport);
					pViewport->Group	= OldGroup;
					pViewport->MiscRes	= OldMiscRes;
					pViewport->Canvas	= OldCanvas;
					if (pViewport->Canvas)
						pViewport->Canvas->Viewport = pViewport;

					// Recreate the viewport window.
					// We don't set an initial position here. Instead, we rely on the
					// caller to reposition the window after switching renderers.
					pViewport->OpenWindow(ViewportOwnerWindow, 0, OldSizeX, OldSizeY, -1, -1, *NewDevice);
				}
				else
				{
					// Switching renderers within an existing window. This is a bad idea...
					FString NewDevice = *GViewportRenderers(Command - IDMN_RD_SOFTWARE);
					GConfig->SetString(*ConfigName, TEXT("Device"), *NewDevice, GUEDIni);
					pViewport->TryRenderDevice(*GViewportRenderers(Command - IDMN_RD_SOFTWARE), pViewport->SizeX, pViewport->SizeY, INDEX_NONE, 0);
					if (!pViewport->RenDev)
					{
						GConfig->SetString(*ConfigName, TEXT("Device"), DEFAULT_UED_RENDERER_PATHNAME, GUEDIni);
						debugf(TEXT("Could not set render device ... reverting to default renderer."));
						pViewport->TryRenderDevice(DEFAULT_UED_RENDERER_PATHNAME, pViewport->SizeX, pViewport->SizeY, INDEX_NONE, 0);
					}
				}
				break;
			}
			default:
				break;
		}
	}

	void FlushRepaintViewport() const
	{
		if (pViewport->Lock(FPlane(0, 0, 0, 0), FPlane(0, 0, 0, 0), FPlane(0, 0, 0, 0), 0))
		{
			pViewport->RenDev->Flush(1);
			pViewport->Unlock(0);
		}
		pViewport->Repaint(1);
	}

	void CreateViewport(DWORD ShowFlags, DWORD RendMap, INT SizeX=320, INT SizeY=200, INT PosX=-1, INT PosY=-1, const TCHAR* Device=nullptr)
	{
		check(!pViewport);
		check(ViewportOwnerWindow);
		
		pViewport = GEditor->Client->NewViewport(*ViewportName);
		check(pViewport);
		GEditor->Level->SpawnViewActor(pViewport);
		pViewport->Input->Init(pViewport);
		check(pViewport->Actor);
		pViewport->Actor->ShowFlags = ShowFlags;
		pViewport->Actor->RendMap	= RendMap;
		pViewport->Group			= NAME_None;
		pViewport->MiscRes			= nullptr;
		pViewport->Actor->Misc1		= 0;
		pViewport->Actor->Misc2		= 0;
		if (!Device)
		{
			Device = DEFAULT_UED_RENDERER_PATHNAME;
			GConfig->SetString(*ConfigName, TEXT("Device"), Device, GUEDIni);
		}
		pViewport->OpenWindow(ViewportOwnerWindow, 0, SizeX, SizeY, PosX, PosY, Device);
	}
};

TArray<FString> WViewportWindowContainer::GViewportRenderers;
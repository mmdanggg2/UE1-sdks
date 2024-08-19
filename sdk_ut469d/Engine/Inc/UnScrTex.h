/*=============================================================================
	UnScrTex.h: Unreal scripted texture class
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Jack Porter
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif


class ENGINE_API UScriptedTexture : public UTexture
{
	DECLARE_CLASS(UScriptedTexture,UTexture,0,Engine)

	// Constructors. Executed on both instantation and reloading of an object.
	UScriptedTexture();

	// UObject interface.	
	void PostLoad();
	void PostEditChange() {PostLoad();}
	void Destroy();

	// UTexture interface.
	void Init( INT InUSize, INT InVSize );
	void Tick(FLOAT DeltaSeconds);

	// Variables.
	AActor*			NotifyActor;
    UTexture*		SourceTexture;

	// UScriptedTexture Interface.
	void DrawTile( FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, UTexture *Tex, FTextureInfo *Info, UBOOL bMasked, UBOOL bUseColor=0, BYTE ColorIndex=0 );

	// Natives.
	DECLARE_FUNCTION(execDrawText)
	DECLARE_FUNCTION(execDrawTile)
	DECLARE_FUNCTION(execDrawColoredText)
	DECLARE_FUNCTION(execReplaceTexture)
	DECLARE_FUNCTION(execTextSize)

private:
	enum EDrawCall_None            { DCT_None            = 0 };
	enum EDrawCall_SourceTexture   { DCT_SourceTexture   = 1 }; // Not really a draw call
	enum EDrawCall_DrawTile        { DCT_DrawTile        = 2 };
	enum EDrawCall_DrawText        { DCT_DrawText        = 3 };
	enum EDrawCall_DrawColoredText { DCT_DrawColoredText = 4 };
	enum EDrawCall_MAX             { DCT_MAX             = 5 };

	struct FDrawCall
	{
		DWORD DrawCallType;
		FORCE_64BIT_PADDING_DWORD;
		FLOAT X, Y, XL, YL;
		FLOAT U, V, UL, VL;
		union{ UBOOL bMasked;     DWORD Color;	};
		union{ UTexture* Texture; UFont* Font;	};
		FString Text;

		FDrawCall()	{}
		FDrawCall( EDrawCall_SourceTexture, UTexture* InTexture);
		FDrawCall( EDrawCall_DrawTile, FLOAT InX, FLOAT InY, FLOAT InXL, FLOAT InYL, FLOAT InU, FLOAT InV, FLOAT InUL, FLOAT InVL, UTexture* InTex, UBOOL InbMasked);
		FDrawCall( EDrawCall_DrawText, FLOAT InX, FLOAT InY /*, FString&& InText*/, UFont* InFont);
		FDrawCall( EDrawCall_DrawColoredText, FLOAT InX, FLOAT InY /*, FString&& InText*/, UFont* InFont, DWORD InColor);

		bool operator==( const FDrawCall& Other) const;
		const FPlane& XYPlane() const { return *(FPlane*)&X; } 
		const FPlane& UVPlane() const { return *(FPlane*)&U; } 
	};


	UTexture*		OldSourceTex;
	BYTE*			LocalSourceBitmap;
	TMap< UTexture*, TArray< BYTE > >*	PaletteMap;
	FLOAT			LocalTime;

	TArray<FDrawCall> OldDrawCalls;
	TArray<FDrawCall> CurDrawCalls;
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

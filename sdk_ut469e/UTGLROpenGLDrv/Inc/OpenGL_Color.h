/*=============================================================================
	OpenGL_Color.h: FPlane to color conversions

	Revision history:
	* Moved from UTGLROpenGL.h
	* SSE version by Fernando Velazquez
=============================================================================*/



#ifdef RGBA_MAKE
#undef RGBA_MAKE
#endif

#if __INTEL_BYTE_ORDER__
static constexpr DWORD COLOR_NO_ALPHA = (255 << 24);
#else
static constexpr DWORD COLOR_NO_ALPHA = (255 << 0);
#endif

static inline DWORD RGBA_MAKE( BYTE r, BYTE g, BYTE b, BYTE a)
{
#if __INTEL_BYTE_ORDER__
	// vogel: I hate macros...
	return (a << 24) | (b << 16) | (g << 8) | r; 
	// vogel: ... and macros hate me
#else
	return (r << 24) | (g <<16) | (b<< 8) | a;
#endif
}							

static inline DWORD RGBA_to_BGRA( DWORD c)
{
	return	(c & 0xFF00FF00)
		|	(c & 0x00FF0000) >> 16
		|	(c & 0x000000FF) << 16;
}


#if USES_SSE_INTRINSICS

static FORCEINLINE DWORD RGBA_MAKE_SSE2( __m128 v)
{
	__m128i iv = _mm_cvtps_epi32(v);
	iv = _mm_packs_epi32(iv, iv); //Down to 16
	iv = _mm_packus_epi16(iv, iv); //Down to 8 (unsigned)
	return (DWORD)_mm_cvtsi128_si32( iv); //x86 CPU's are little endian
}

static FORCEINLINE DWORD FPlaneTo_RGB_A255( const FPlane *pPlane)
{
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps( &pPlane->X), v_255);
	return RGBA_MAKE_SSE2( v_plane ) | 0xFF000000;
}

static FORCEINLINE DWORD FPlaneTo_RGBClamped_A255( const FPlane *pPlane)
{
	__m128 v_0 = _mm_setzero_ps();
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps(&pPlane->X), v_255);
	return RGBA_MAKE_SSE2( _mm_min_ps( _mm_max_ps( v_plane, v_0), v_255) ) | 0xFF000000;
}

static FORCEINLINE DWORD FPlaneTo_RGB_A0( const FPlane *pPlane)
{
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps( &pPlane->X), v_255);
	return RGBA_MAKE_SSE2( v_plane ) & 0x00FFFFFF;
}

static FORCEINLINE DWORD FPlaneTo_RGB_Aub( const FPlane *pPlane, BYTE alpha)
{
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps( &pPlane->X), v_255);
	return RGBA_MAKE_SSE2( v_plane ) | RGBA_MAKE(0,0,0,alpha);
}

static FORCEINLINE DWORD FPlaneTo_RGBA( const FPlane *pPlane)
{
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps( &pPlane->X), v_255);
	return RGBA_MAKE_SSE2( v_plane );
}

static FORCEINLINE DWORD FPlaneTo_RGBAClamped( const FPlane *pPlane)
{
	__m128 v_0 = _mm_setzero_ps();
	__m128 v_255 = _mm_set_ps(255,255,255,255);
	__m128 v_plane = _mm_mul_ps( _mm_loadu_ps( &pPlane->X), v_255);
	return RGBA_MAKE_SSE2( _mm_min_ps( _mm_max_ps( v_plane, v_0), v_255) );
}

static FORCEINLINE DWORD FPlaneTo_RGBScaled_A255( const FPlane *pPlane, FLOAT rgbScale)
{
	__m128 v_plane = _mm_loadu_ps( &pPlane->X);
	__m128 v_scale = _mm_load1_ps( &rgbScale);
	return RGBA_MAKE_SSE2( _mm_mul_ps(v_plane,v_scale) ) | 0xFF000000;
}

#else

static inline DWORD FASTCALL FPlaneTo_RGB_A255(const FPlane *pPlane)
{
	return RGBA_MAKE(
		appRound(pPlane->X * 255.0f),
		appRound(pPlane->Y * 255.0f),
		appRound(pPlane->Z * 255.0f),
		255);
}

static inline DWORD FASTCALL FPlaneTo_RGBClamped_A255(const FPlane *pPlane)
{
	return RGBA_MAKE(
		Clamp(appRound(pPlane->X * 255.0f), 0, 255),
		Clamp(appRound(pPlane->Y * 255.0f), 0, 255),
		Clamp(appRound(pPlane->Z * 255.0f), 0, 255),
		255);
}

static inline DWORD FASTCALL FPlaneTo_RGB_A0(const FPlane *pPlane)
{
	return RGBA_MAKE(
		appRound(pPlane->X * 255.0f),
		appRound(pPlane->Y * 255.0f),
		appRound(pPlane->Z * 255.0f),
		0);
}

static inline DWORD FASTCALL FPlaneTo_RGB_Aub(const FPlane *pPlane, BYTE alpha)
{
	return RGBA_MAKE(
		appRound(pPlane->X * 255.0f),
		appRound(pPlane->Y * 255.0f),
		appRound(pPlane->Z * 255.0f),
		alpha);
}

static inline DWORD FASTCALL FPlaneTo_RGBA(const FPlane *pPlane)
{
	return RGBA_MAKE(
		appRound(pPlane->X * 255.0f),
		appRound(pPlane->Y * 255.0f),
		appRound(pPlane->Z * 255.0f),
		appRound(pPlane->W * 255.0f));
}

static inline DWORD FASTCALL FPlaneTo_RGBAClamped(const FPlane *pPlane)
{
	return RGBA_MAKE(
		Clamp(appRound(pPlane->X * 255.0f), 0, 255),
		Clamp(appRound(pPlane->Y * 255.0f), 0, 255),
		Clamp(appRound(pPlane->Z * 255.0f), 0, 255),
		Clamp(appRound(pPlane->W * 255.0f), 0, 255));
}

static inline DWORD FASTCALL FPlaneTo_RGBScaled_A255(const FPlane *pPlane, FLOAT rgbScale)
{
	return RGBA_MAKE(
		appRound(pPlane->X * rgbScale),
		appRound(pPlane->Y * rgbScale),
		appRound(pPlane->Z * rgbScale),
		255);
}

#endif
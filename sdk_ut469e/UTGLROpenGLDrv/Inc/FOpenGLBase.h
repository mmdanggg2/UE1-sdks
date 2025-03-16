/*=============================================================================
	FOpenGLBase.h: OpenGL context abstraction.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

// Move this
#define GL_GENERATE_MIPMAP_HINT 0x8192
#define GL_TEXTURE_BUFFER       0x8C2A

#if _WIN32
#include "wgl.h"
#include "wglext.h"

#define GL_ENTRYPOINTS_DLL_WGL(GLEntry) \
	GLEntry(PFNWGLCOPYCONTEXTPROC,wglCopyContext) \
	GLEntry(PFNWGLCREATECONTEXTPROC,wglCreateContext) \
	GLEntry(PFNWGLCREATELAYERCONTEXTPROC,wglCreateLayerContext) \
	GLEntry(PFNWGLDELETECONTEXTPROC,wglDeleteContext) \
	GLEntry(PFNWGLGETCURRENTCONTEXTPROC,wglGetCurrentContext) \
	GLEntry(PFNWGLGETCURRENTDCPROC,wglGetCurrentDC) \
	GLEntry(PFNWGLGETPROCADDRESSPROC,wglGetProcAddress) \
	GLEntry(PFNWGLMAKECURRENTPROC,wglMakeCurrent) \
	GLEntry(PFNWGLSHARELISTSPROC,wglShareLists)

#define GL_ENTRYPOINTS_WGL(GLEntry) \
	GLEntry(PFNWGLCHOOSEPIXELFORMATARBPROC,wglChoosePixelFormatARB) \
	GLEntry(PFNWGLCREATECONTEXTATTRIBSARBPROC,wglCreateContextAttribsARB) \
	GLEntry(PFNWGLGETEXTENSIONSSTRINGARBPROC,wglGetExtensionsStringARB) \
	GLEntry(PFNWGLSWAPINTERVALEXTPROC,wglSwapIntervalEXT) \
	GLEntry(PFNWGLGETPIXELFORMATATTRIBIVARBPROC,wglGetPixelFormatAttribivARB)\
	GLEntry(PFNWGLDXOPENDEVICENVPROC,wglDXOpenDeviceNV) \
	GLEntry(PFNWGLDXCLOSEDEVICENVPROC,wglDXCloseDeviceNV) \
	GLEntry(PFNWGLDXREGISTEROBJECTNVPROC,wglDXRegisterObjectNV) \
	GLEntry(PFNWGLDXUNREGISTEROBJECTNVPROC,wglDXUnregisterObjectNV) \
	GLEntry(PFNWGLDXLOCKOBJECTSNVPROC,wglDXLockObjectsNV) \
	GLEntry(PFNWGLDXUNLOCKOBJECTSNVPROC,wglDXUnlockObjectsNV)

#define GL_EXTENSIONS_WGL(GLExt) \
	GLExt(_WGL_ARB_multisample) \
	GLExt(_WGL_EXT_swap_control) \
	GLExt(_WGL_EXT_swap_control_tear) \
	GLExt(_WGL_NV_DX_interop) \
	GLExt(_WGL_NV_DX_interop2)

#endif


// The common GL interface requires at least GL 2.0
// May be populated by extensions and workarounds if missing from API
#define GL_ENTRYPOINTS_COMMON(GLEntry) \
	GLEntry(PFNGLACTIVETEXTUREPROC,glActiveTexture) \
	GLEntry(PFNGLBINDTEXTUREPROC,glBindTexture) \
	GLEntry(PFNGLBLENDFUNCPROC,glBlendFunc) \
	GLEntry(PFNGLCLEARPROC,glClear) \
	GLEntry(PFNGLCLEARCOLORPROC,glClearColor) \
	GLEntry(PFNGLCLEARDEPTHFPROC,glClearDepthf) \
	GLEntry(PFNGLCOLORMASKPROC,glColorMask) \
	GLEntry(PFNGLCOMPRESSEDTEXIMAGE2DPROC,glCompressedTexImage2D) \
	GLEntry(PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC,glCompressedTexSubImage2D) \
	GLEntry(PFNGLCOMPRESSEDTEXIMAGE3DPROC,glCompressedTexImage3D) \
	GLEntry(PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC,glCompressedTexSubImage3D) \
	GLEntry(PFNGLCOPYTEXSUBIMAGE2DPROC,glCopyTexSubImage2D) \
	GLEntry(PFNGLCOPYTEXSUBIMAGE3DPROC,glCopyTexSubImage3D) \
	GLEntry(PFNGLDELETETEXTURESPROC,glDeleteTextures) \
	GLEntry(PFNGLDEPTHFUNCPROC,glDepthFunc) \
	GLEntry(PFNGLDEPTHMASKPROC,glDepthMask) \
	GLEntry(PFNGLDEPTHRANGEFPROC,glDepthRangef) \
	GLEntry(PFNGLDISABLEPROC,glDisable) \
	GLEntry(PFNGLDRAWARRAYSPROC,glDrawArrays) \
	GLEntry(PFNGLDRAWELEMENTSPROC,glDrawElements) \
	GLEntry(PFNGLENABLEPROC,glEnable) \
	GLEntry(PFNGLFINISHPROC,glFinish) \
	GLEntry(PFNGLFLUSHPROC,glFlush) \
	GLEntry(PFNGLGENTEXTURESPROC,glGenTextures) \
	GLEntry(PFNGLGETCOMPRESSEDTEXIMAGEPROC,glGetCompressedTexImage) \
	GLEntry(PFNGLGETERRORPROC,glGetError) \
	GLEntry(PFNGLGETINTEGERVPROC,glGetIntegerv) \
	GLEntry(PFNGLGETSTRINGPROC,glGetString) \
	GLEntry(PFNGLGETTEXIMAGEPROC,glGetTexImage) \
	GLEntry(PFNGLHINTPROC,glHint) \
	GLEntry(PFNGLMULTIDRAWARRAYSPROC,glMultiDrawArrays) \
	GLEntry(PFNGLMULTIDRAWELEMENTSPROC,glMultiDrawElements) \
	GLEntry(PFNGLPOLYGONOFFSETPROC,glPolygonOffset) \
	GLEntry(PFNGLREADPIXELSPROC,glReadPixels) \
	GLEntry(PFNGLTEXIMAGE2DPROC,glTexImage2D) \
	GLEntry(PFNGLTEXIMAGE3DPROC,glTexImage3D) \
	GLEntry(PFNGLTEXPARAMETERFPROC,glTexParameterf) \
	GLEntry(PFNGLTEXPARAMETERFVPROC,glTexParameterfv) /*REMOVE IF NOT USED*/ \
	GLEntry(PFNGLTEXPARAMETERIPROC,glTexParameteri) \
	GLEntry(PFNGLTEXPARAMETERIVPROC,glTexParameteriv) /*REMOVE IF NOT USED*/ \
	GLEntry(PFNGLTEXSUBIMAGE2DPROC,glTexSubImage2D) \
	GLEntry(PFNGLTEXSUBIMAGE3DPROC,glTexSubImage3D) \
	GLEntry(PFNGLVIEWPORTPROC,glViewport)

#define GL_ENTRYPOINTS_COMMON_OPTIONAL(GLEntry) \
/* Compatibility */ \
	GLEntry(PFNGLACTIVETEXTUREPROC,glActiveTextureARB) \
	GLEntry(PFNGLCLEARDEPTHPROC,glClearDepth) \
	GLEntry(PFNGLDEPTHRANGEPROC,glDepthRange) \
	GLEntry(PFNGLMULTIDRAWARRAYSPROC,glMultiDrawArraysEXT) \
	GLEntry(PFNGLMULTIDRAWELEMENTSPROC,glMultiDrawElementsEXT) \
/* Framebuffer */ \
	GLEntry(PFNGLBINDFRAMEBUFFERPROC,glBindFramebuffer) \
	GLEntry(PFNGLBINDRENDERBUFFERPROC,glBindRenderbuffer) \
	GLEntry(PFNGLCHECKFRAMEBUFFERSTATUSPROC,glCheckFramebufferStatus) \
	GLEntry(PFNGLDELETEFRAMEBUFFERSPROC,glDeleteFramebuffers) \
	GLEntry(PFNGLDELETERENDERBUFFERSPROC,glDeleteRenderbuffers) \
	GLEntry(PFNGLFRAMEBUFFERRENDERBUFFERPROC,glFramebufferRenderbuffer) \
	GLEntry(PFNGLFRAMEBUFFERTEXTURE2DPROC,glFramebufferTexture2D) \
	GLEntry(PFNGLFRAMEBUFFERTEXTURELAYERPROC,glFramebufferTextureLayer) \
	GLEntry(PFNGLGENERATEMIPMAPPROC,glGenerateMipmap) \
	GLEntry(PFNGLGENFRAMEBUFFERSPROC,glGenFramebuffers) \
	GLEntry(PFNGLGENRENDERBUFFERSPROC,glGenRenderbuffers) \
	GLEntry(PFNGLRENDERBUFFERSTORAGEPROC,glRenderbufferStorage) \
/* Framebuffer Blit */ \
	GLEntry(PFNGLBLITFRAMEBUFFERPROC,glBlitFramebuffer) \
/* Framebuffer Multisampling */ \
	GLEntry(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC,glRenderbufferStorageMultisample) \
/* Sampler Objects */ \
	GLEntry(PFNGLGENSAMPLERSPROC,glGenSamplers) \
	GLEntry(PFNGLDELETESAMPLERSPROC,glDeleteSamplers) \
	GLEntry(PFNGLBINDSAMPLERPROC,glBindSampler) \
	GLEntry(PFNGLSAMPLERPARAMETERIPROC,glSamplerParameteri) \
	GLEntry(PFNGLSAMPLERPARAMETERFPROC,glSamplerParameterf) \
	GLEntry(PFNGLGETSAMPLERPARAMETERIVPROC,glGetSamplerParameteriv) \
	GLEntry(PFNGLGETSAMPLERPARAMETERFVPROC,glGetSamplerParameterfv) \
/* Buffer Object */ \
	GLEntry(PFNGLGENBUFFERSPROC,glGenBuffers) \
	GLEntry(PFNGLDELETEBUFFERSPROC,glDeleteBuffers) \
	GLEntry(PFNGLBINDBUFFERPROC,glBindBuffer) \
	GLEntry(PFNGLBUFFERDATAPROC,glBufferData) \
	GLEntry(PFNGLBUFFERSUBDATAPROC,glBufferSubData) \
	GLEntry(PFNGLMAPBUFFERPROC,glMapBuffer) \
	GLEntry(PFNGLUNMAPBUFFERPROC,glUnmapBuffer) \
/* GLSL */ \
	GLEntry(PFNGLCOMPILESHADERPROC,glCompileShader) \
	GLEntry(PFNGLCREATESHADERPROC,glCreateShader) \
	GLEntry(PFNGLDELETESHADERPROC,glDeleteShader) \
	GLEntry(PFNGLISSHADERPROC,glIsShader) \
	GLEntry(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog) \
	GLEntry(PFNGLGETSHADERIVPROC,glGetShaderiv) \
	GLEntry(PFNGLSHADERSOURCEPROC,glShaderSource) \
	GLEntry(PFNGLCREATEPROGRAMPROC,glCreateProgram) \
	GLEntry(PFNGLDELETEPROGRAMPROC,glDeleteProgram) \
	GLEntry(PFNGLISPROGRAMPROC,glIsProgram) \
	GLEntry(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog) \
	GLEntry(PFNGLGETPROGRAMIVPROC,glGetProgramiv) \
	GLEntry(PFNGLLINKPROGRAMPROC,glLinkProgram) \
	GLEntry(PFNGLATTACHSHADERPROC,glAttachShader) \
	GLEntry(PFNGLDETACHSHADERPROC,glDetachShader) \
	GLEntry(PFNGLUSEPROGRAMPROC,glUseProgram) \
	GLEntry(PFNGLBINDATTRIBLOCATIONPROC,glBindAttribLocation) \
	GLEntry(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation) \
	GLEntry(PFNGLUNIFORM1IPROC,glUniform1i) \
	GLEntry(PFNGLUNIFORM1FPROC,glUniform1f) \
	GLEntry(PFNGLUNIFORM1UIPROC,glUniform1ui) \
	GLEntry(PFNGLUNIFORM4FPROC,glUniform4f) \
	GLEntry(PFNGLUNIFORM4FVPROC,glUniform4fv) \
	GLEntry(PFNGLBINDBUFFERBASEPROC,glBindBufferBase) \
	GLEntry(PFNGLBINDBUFFERRANGEPROC,glBindBufferRange) \
	GLEntry(PFNGLGETUNIFORMBLOCKINDEXPROC,glGetUniformBlockIndex) \
	GLEntry(PFNGLUNIFORMBLOCKBINDINGPROC,glUniformBlockBinding) \
	GLEntry(PFNGLENABLEVERTEXATTRIBARRAYPROC,glEnableVertexAttribArray) \
	GLEntry(PFNGLDISABLEVERTEXATTRIBARRAYPROC,glDisableVertexAttribArray) \
	GLEntry(PFNGLVERTEXATTRIBPOINTERPROC,glVertexAttribPointer) \
	GLEntry(PFNGLVERTEXATTRIBIPOINTERPROC,glVertexAttribIPointer) \
/* Buffer Range (GL 3.0) */ \
	GLEntry(PFNGLFLUSHMAPPEDBUFFERRANGEPROC,glFlushMappedBufferRange) \
	GLEntry(PFNGLMAPBUFFERRANGEPROC,glMapBufferRange) \
/* Buffer Textures (GL 3.1 - GLES 3.2) */ \
	GLEntry(PFNGLTEXBUFFERPROC,glTexBuffer) \
/* Vertex Array Object */ \
	GLEntry(PFNGLISVERTEXARRAYPROC,glIsVertexArray) \
	GLEntry(PFNGLGENVERTEXARRAYSPROC,glGenVertexArrays) \
	GLEntry(PFNGLDELETEVERTEXARRAYSPROC,glDeleteVertexArrays) \
	GLEntry(PFNGLBINDVERTEXARRAYPROC,glBindVertexArray) \
/* Fence Sync */ \
	GLEntry(PFNGLCLIENTWAITSYNCPROC,glClientWaitSync) \
	GLEntry(PFNGLDELETESYNCPROC,glDeleteSync) \
	GLEntry(PFNGLFENCESYNCPROC,glFenceSync) \
/* Clip Control */ \
	GLEntry(PFNGLCLIPCONTROLPROC,glClipControl) \
/* Compute Shaders */ \
	GLEntry(PFNGLDISPATCHCOMPUTEPROC,glDispatchCompute) \
	GLEntry(PFNGLDISPATCHCOMPUTEINDIRECTPROC,glDispatchComputeIndirect) \
/* Inmutable texture storage (GL 4.2 - GLES 3.0) */ \
	GLEntry(PFNGLTEXSTORAGE2DPROC,glTexStorage2D) \
	GLEntry(PFNGLTEXSTORAGE3DPROC,glTexStorage3D) \
/* Inmutable multisampled texture storage (GL 4.2 - GLES 3.0) */ \
	GLEntry(PFNGLTEXSTORAGE2DMULTISAMPLEPROC,glTexStorage2DMultisample) \
	GLEntry(PFNGLTEXSTORAGE3DMULTISAMPLEPROC,glTexStorage3DMultisample) \
/* Image load store (GL 4.2 - GLES 3.1) */ \
	GLEntry(PFNGLBINDIMAGETEXTUREPROC,glBindImageTexture) \
	GLEntry(PFNGLMEMORYBARRIERPROC,glMemoryBarrier) \
/* Debug callback (GL 4.3) */ \
	GLEntry(PFNGLDEBUGMESSAGECALLBACKPROC,glDebugMessageCallback) \
/* Image copy (GL 4.3 - GLES 3.2) */ \
	GLEntry(PFNGLCOPYIMAGESUBDATANVPROC,glCopyImageSubData) \
/* Data invalidation (GL 4.3 - GLES 3.0) */ \
	GLEntry(PFNGLINVALIDATEFRAMEBUFFERPROC,glInvalidateFramebuffer) \
	GLEntry(PFNGLINVALIDATETEXIMAGEPROC,glInvalidateTexImage) \
	GLEntry(PFNGLINVALIDATETEXSUBIMAGEPROC,glInvalidateTexSubImage) \
/* Inmutable Buffer Storage (GL 4.4) */ \
	GLEntry(PFNGLBUFFERSTORAGEPROC,glBufferStorage) \
/* Direct State Access (GL 4.4) */ \
	GLEntry(PFNGLTEXTUREPARAMETERFPROC,glTextureParameterf) \
	GLEntry(PFNGLTEXTUREPARAMETERIPROC,glTextureParameteri) \
	GLEntry(PFNGLTEXTURESTORAGE2DPROC,glTextureStorage2D) \
	GLEntry(PFNGLTEXTURESTORAGE3DPROC,glTextureStorage3D) \
	GLEntry(PFNGLTEXTURESUBIMAGE2DPROC,glTextureSubImage2D) \
	GLEntry(PFNGLTEXTURESUBIMAGE3DPROC,glTextureSubImage3D) \
	GLEntry(PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC,glCompressedTextureSubImage2D) \
	GLEntry(PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC,glCompressedTextureSubImage3D) \
	GLEntry(PFNGLCREATEBUFFERSPROC,glCreateBuffers) \
	GLEntry(PFNGLNAMEDBUFFERSTORAGEPROC,glNamedBufferStorage) \
	GLEntry(PFNGLMAPNAMEDBUFFERRANGEPROC,glMapNamedBufferRange) \
	GLEntry(PFNGLUNMAPNAMEDBUFFERPROC,glUnmapNamedBuffer) 


#define GL_EXTENSIONS_COMMON(GLExt) \
	GLExt(Framebuffer,TEXT("GL_ARB_framebuffer_object")) \
	GLExt(FramebufferBlit,TEXT("GL_EXT_framebuffer_blit")) \
	GLExt(FramebufferMultisampling,TEXT("GL_EXT_framebuffer_multisample")) \
	GLExt(SamplerObjects,TEXT("GL_ARB_sampler_objects")) \
	GLExt(RGB9_E5,TEXT("GL_EXT_texture_shared_exponent")) \
	GLExt(S3TC,TEXT("GL_EXT_texture_compression_s3tc")) \
	GLExt(RGTC,TEXT("GL_ARB_texture_compression_rgtc")) \
	GLExt(BPTC,TEXT("GL_ARB_texture_compression_bptc")) \
	GLExt(TBO,TEXT("GL_ARB_texture_buffer_object")) \
	GLExt(VAO,TEXT("GL_ARB_vertex_array_object")) \
	GLExt(VBO,TEXT("GL_ARB_vertex_buffer_object")) \
	GLExt(UBO,TEXT("GL_ARB_uniform_buffer_object")) \
	GLExt(Anisotropy,TEXT("GL_EXT_texture_filter_anisotropic")) \
	GLExt(LODBias,TEXT("GL_EXT_texture_lod_bias")) \
	GLExt(ClipControl,TEXT("GL_ARB_clip_control")) \
	GLExt(TextureStorage,TEXT("GL_ARB_texture_storage")) \
	GLExt(TextureStorageMultisample,TEXT("GL_ARB_texture_storage_multisample")) \
	GLExt(TextureSwizzle,TEXT("GL_ARB_texture_swizzle")) \
	GLExt(PersistentMapping,TEXT("GL_ARB_buffer_storage")) \
	GLExt(DataInvalidation,TEXT("GL_ARB_invalidate_subdata")) \
	GLExt(ImageCopy,TEXT("GL_ARB_copy_image")) \
	GLExt(SyncObjects,TEXT("GL_ARB_sync")) \
	GLExt(ComputeShader,TEXT("GL_ARB_compute_shader")) \
	GLExt(ImageLoadStore,TEXT("GL_ARB_shader_image_load_store")) \
	GLExt(ShaderGroupVote,TEXT("GL_ARB_shader_group_vote")) \
	GLExt(ClipCullDistance,TEXT("GL_EXT_clip_cull_distance")) \
	GLExt(DirectStateAccess,TEXT("GL_ARB_direct_state_access"))

namespace FGL
{
	enum EContextAttribute
	{
		PROFILE,
		MAJOR_VERSION,
		MINOR_VERSION,
		FLAGS,
	};

	enum EContextProfile
	{
		PROFILE_CORE          = 1 << 0,
		PROFILE_COMPATIBILITY = 1 << 1,
		PROFILE_ES            = 1 << 2,

		PROFILE_UNDEFINED     = -1
	};

	enum EContextFlags
	{
		FLAGS_DEBUG              = 1 << 0,
		FLAGS_FORWARD_COMPATIBLE = 1 << 1,
	};
};

#define GL_MAX_TEXTURE_UNITS 16

class FOpenGLSwizzle;
struct FTextureFormatInfo;

class FOpenGLBase
{
public:
	friend class FOpenGLSwizzle;
	friend class FOpenGLTexture;

	FOpenGLBase( void* InWindow);
	virtual ~FOpenGLBase();

	static bool Init();
	static void* GetGLProc( const char* ProcName);

	// Declare common procs and extensions
	#define GLEntry(type, name) static type name;
	#define GLExt(name,ext) static UBOOL Supports##name;
	GL_ENTRYPOINTS_COMMON(GLEntry)
	GL_ENTRYPOINTS_COMMON_OPTIONAL(GLEntry)
	GL_EXTENSIONS_COMMON(GLExt)
	#undef GLExt
	#undef GLEntry

	#if __WIN32__
	static HMODULE hOpenGL32;
	static ATOM DummyWndClass;

	static HWND CreateDummyWindow();

	// Declare WGL procs and extensions
	#define GLEntry(type, name) static type name;
	#define GLExt(name) static bool SUPPORTS##name;
	GL_ENTRYPOINTS_DLL_WGL(GLEntry)
	GL_ENTRYPOINTS_WGL(GLEntry)
	GL_EXTENSIONS_WGL(GLExt)
	#undef GLExt
	#undef GLEntry

	#elif __UNIX__
	static bool GLLoaded;

	#endif

	static FOpenGLBase* ActiveInstance;
	static TArray<FOpenGLBase*> Instances;
	static GLint MajorVersion;
	static GLint MinorVersion;
	static GLint MaxTextureSize;
	static GLint MaxArrayTextureLayers;
	static GLint MaxTextureImageUnits;
	static GLint MaxAnisotropy;
	static GLint MaxFramebufferSamples;
	static GLint MaxUniformBlockSize;
	static GLint MaxVertexUniformBlocks;
	static GLint MaxFragmentUniformBlocks;
	static GLint UniformBufferOffsetAlignment;
	static GLint MaxTextureBufferSize;
	static FString Vendor;
	static FString Renderer;
	static FString Version;
	static TArray<FString> Extensions;

	static bool InitProcs( bool Force=false);
	static void InitCapabilities( bool Force=false);
	static void InitInterfaces();
	static bool SupportsExtension( const TCHAR* ExtensionName);
	static void* CreateContext( void* Window);
	static void  DeleteContext( void* Context);
	static void SetContextAttribute( FGL::EContextAttribute Attribute, DWORD Value);
	static void InstallDebugCallback();

	static UBOOL GetTrilinear(); // Remove later
	static GLint GetAnisotropy(); // Remove later
	static FLOAT GetShaderBrightness();
	static void SetTrilinear( UBOOL Enable);
	static void SetAnisotropy( INT NewAnisotropy);
	static void SetDervMapping( UBOOL Enable);
	static void DisableColorCorrection();
	static void SetColorCorrection( FLOAT Brightness);
	static void SetColorCorrection( FLOAT Brightness, FLOAT GammaOffsetRed, FLOAT GammaOffsetBlue, FLOAT GammaOffsetGreen);
	static void SetLODBias( FLOAT NewLODBias);

	virtual void Reset();
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual void SetProgram( FProgramID NewProgramID, void** ProgramData=nullptr) = 0;
	virtual void FlushPrograms() = 0;

protected:
	static UBOOL UsingTrilinear;
	static GLint UsingAnisotropy;
	static UBOOL UsingDervMapping;
	static UBOOL UsingColorCorrection;
	static FPlane ColorCorrection;
	static GLfloat LODBias;
	static GLfloat CurrentLODBias;

public:
	// Generic abstraction layer

	// Note: affects buffer bindings
	static void CreateBuffer( GLuint& Buffer, GLenum Target, GLsizeiptr Size, const void* Data, GLenum Usage);
	static void CreateInmutableBuffer( GLuint& Buffer, GLenum Target, GLsizeiptr Size, const void* Data, GLbitfield StorageFlags);
	static void DeleteBuffer( GLuint& Buffer);
	// StorageFlags:
	// GL_DYNAMIC_STORAGE_BIT, GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT, GL_CLIENT_STORAGE_BIT.

	// Note: does not do texture mapping unit binding checks.
	static void SetTextureStorage( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLsizei Width, GLsizei Height, GLsizei Depth, GLsizei Levels);
	static void SetTextureData( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData, GLsizei TextureBytes);
	static void SetTextureFilters( FOpenGLTexture& Texture, bool Nearest);
	static bool GenerateTextureMipmaps( FOpenGLTexture& Texture);
	static const TCHAR* GetTextureTargetString(GLenum Target);

	static GLuint CompileShader(GLenum Target, const ANSICHAR* Source);

	static const TCHAR* GetGLError();
	static INT FlushGLErrors();

public:
	// Context handling
	UOpenGLRenderDevice* RenDev;
	static void* CurrentWindow;
	static void* CurrentContext;
	void* Window;
	void* Context;
#if __WIN32__
	static HDC CurrentDC;
	static TMapExt<void*,HDC> WindowDC;
	static HDC WindowDC_Get(void* Window);
	static void WindowDC_Release(void* Window);

	// implemented in OpenGL_DXGI_Interop.cpp
	void CreateDXGISwapchain();
	void SetDXGIFullscreenState(UBOOL Fullscreen);
	void DestroyDXGISwapchain();
	bool UpdateDXGISwapchain();
	void LockDXGIFramebuffer();
	bool UnlockDXGIFramebuffer();
#endif
	bool MakeCurrent( void* OnWindow);

public:
	// Texture unit handling
	struct TextureUnitState
	{
		GLuint          Active;
		FOpenGLTexture* Bound[GL_MAX_TEXTURE_UNITS];

		// Hard reset
		void Reset();

		void SetActive( GLuint Unit);
		void SetActiveNoCheck( GLuint Unit);

		void Bind( const FOpenGLTexture& Texture);
		void Unbind();

		// Note: sets the active texture to 0
		void UnbindAll();

		bool IsBound( FOpenGLTexture& Texture, GLuint Unit) const;
		bool IsUnbound( GLuint Unit) const;

		// Active bound texture modifiers
		void SetSwizzle( FOpenGLSwizzle InSwizzle);
	} TextureUnits{};

	// Viewport handling
	struct FViewportInfo
	{
		GLint X, Y;
		GLsizei XL, YL;
		UBOOL ForceSet;
	} ViewportInfo{};
	void SetViewport( GLint NewX, GLint NewY, GLsizei NewXL, GLsizei NewYL);

	// Indexed buffers
	GLuint IndexedBufferTargets[8]{};
	void BindBufferBase( GLenum Target, GLuint Index, GLuint Buffer);

public:
	// Compute program handling
	static struct FCompute
	{
		struct FShaders
		{
			GLuint QuadExpansion;
			GLuint PaletteTextureUpdate;
		} Shaders;
		struct FPrograms
		{
			GLuint QuadExpansion;
			GLuint PaletteTextureUpdate;
		} Programs;
	} Compute;

	void SetComputeQuadExpansion( GLuint InBuffer, GLuint OutBuffer);
	void SetComputePaletteTextureUpdate( GLuint Image, GLuint Layer, GLuint TexBuffer, GLuint PalBuffer, GLsizei PalOffset, bool Masked);
	void ExitCompute();

};


/*-----------------------------------------------------------------------------
	FOpenGLBase dependents.
-----------------------------------------------------------------------------*/

#include "FOpenGLTexture.h"


/*-----------------------------------------------------------------------------
	Initialization.
-----------------------------------------------------------------------------*/

inline FOpenGLBase::FOpenGLBase( void* InWindow)
{
	Window =  InWindow;
	Context = CreateContext(InWindow);
	if ( !Context )
		appFailAssert("Unable to create new OpenGL context", __FILE__, __LINE__);

	Instances.AddItem(this);
}

FORCEINLINE void* FOpenGLBase::GetGLProc( const char* ProcName)
{
	void* Result = nullptr;
#if _WIN32
	if ( wglGetProcAddress )
		Result = wglGetProcAddress(ProcName);
	if ( !Result )
		Result = GetProcAddress(hOpenGL32,ProcName);
#elif __UNIX__
	Result = (void *)SDL_GL_GetProcAddress(ProcName);
#endif
	return Result;
}

inline bool FOpenGLBase::SupportsExtension( const TCHAR* ExtensionName)
{
	if ( Extensions.FindItemIndex(ExtensionName) != INDEX_NONE )
		return true;
	return false;
}


/*-----------------------------------------------------------------------------
	General abstractions.
-----------------------------------------------------------------------------*/

inline void FOpenGLBase::CreateBuffer( GLuint& Buffer, GLenum Target, GLsizeiptr Size, const void* Data, GLenum Usage)
{
	if ( Buffer == 0 )
		glGenBuffers(1, &Buffer);
	glBindBuffer(Target, Buffer);
	glBufferData(Target, Size, Data, Usage);
	glBindBuffer(Target, 0);
}

inline void FOpenGLBase::DeleteBuffer( GLuint& Buffer)
{
	if ( Buffer )
	{
		glDeleteBuffers(1, &Buffer);
		Buffer = 0;
	}
}


/*-----------------------------------------------------------------------------
	General interface switches.
-----------------------------------------------------------------------------*/

inline UBOOL FOpenGLBase::GetTrilinear()
{
	return UsingTrilinear;
}

inline GLint FOpenGLBase::GetAnisotropy()
{
	return UsingAnisotropy;
}

inline FLOAT FOpenGLBase::GetShaderBrightness()
{
	return ColorCorrection.W;
}

inline void FOpenGLBase::SetTrilinear( UBOOL Enable)
{
	UsingTrilinear = (UBOOL)Enable;
}

inline void FOpenGLBase::SetAnisotropy( INT NewAnisotropy)
{
	UsingAnisotropy = SupportsAnisotropy ? Min( MaxAnisotropy, NewAnisotropy) : 0;
}

inline void FOpenGLBase::SetDervMapping( UBOOL Enable)
{
	UsingDervMapping = Enable;
}

inline void FOpenGLBase::DisableColorCorrection()
{
	UsingColorCorrection = 0;
}

inline void FOpenGLBase::SetLODBias( FLOAT NewLODBias)
{
	if ( SupportsLODBias )
		LODBias = NewLODBias;
}



/*=============================================================================
	OpenGLFuncs.h: OpenGL function-declaration macros.

	Copyright 1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Daniel Vogel (based on XMesaGLDrv)
		* GLX function declarations removed by  Daniel Vogel
		* WGL function re-introduced Fredrik Gustafsson
		* Unified by Daniel Vogel
=============================================================================*/

/*-----------------------------------------------------------------------------
	Standard OpenGL functions.
-----------------------------------------------------------------------------*/

#ifndef WIN32
#define DYNAMIC_BIND 1
#endif

#if DYNAMIC_BIND

GL_EXT(_GL)

// OpenGL.
#ifndef __EMSCRIPTEN__
GL_PROC(_GL,void,glAlphaFunc,(GLenum,GLclampf))
GL_PROC(_GL,void,glBegin,(GLenum))
GL_PROC(_GL,void,glBindTexture,(GLenum,GLuint))
GL_PROC(_GL,void,glBlendFunc,(GLenum,GLenum))
GL_PROC(_GL,void,glClear,(GLbitfield))
GL_PROC(_GL,void,glClearColor,(GLclampf,GLclampf,GLclampf,GLclampf))
GL_PROC(_GL,void,glClearDepth,(GLclampd))
GL_PROC(_GL,void,glColor3fv,(const GLfloat*))
GL_PROC(_GL,void,glColor4f,(GLfloat,GLfloat,GLfloat,GLfloat))
GL_PROC(_GL,void,glColor4fv,(const GLfloat*))
GL_PROC(_GL,void,glColorMask,(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
GL_PROC(_GL,void,glColorPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
GL_PROC(_GL,void,glDeleteTextures,(GLsizei n, const GLuint *textures))
GL_PROC(_GL,void,glDepthFunc,(GLenum func))
GL_PROC(_GL,void,glDepthMask,(GLboolean flag))
GL_PROC(_GL,void,glDepthRange,(GLclampd zNear, GLclampd zFar))
GL_PROC(_GL,void,glDisable,(GLenum cap))
GL_PROC(_GL,void,glDisableClientState,(GLenum array))
GL_PROC(_GL,void,glDrawArrays,(GLenum mode, GLint first, GLsizei count))
GL_PROC(_GL,void,glDrawElements,(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices))
GL_PROC(_GL,void,glEnable,(GLenum cap))
GL_PROC(_GL,void,glEnableClientState,(GLenum array))
GL_PROC(_GL,void,glEnd,(void))
GL_PROC(_GL,void,glFrustum,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
GL_PROC(_GL,void,glGenTextures,(GLsizei n, GLuint *textures))
GL_PROC(_GL,GLenum,glGetError,(void))
GL_PROC(_GL,void,glGetIntegerv,(GLenum pname, GLint *params))
GL_PROC(_GL,const GLubyte *,glGetString,(GLenum name))
GL_PROC(_GL,void,glLoadIdentity,(void))
GL_PROC(_GL,void,glMatrixMode,(GLenum mode))
GL_PROC(_GL,void,glMultMatrixf,(const GLfloat *m))
GL_PROC(_GL,void,glOrtho,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
GL_PROC(_GL,void,glPolygonOffset,(GLfloat factor, GLfloat units))
GL_PROC(_GL,void,glReadPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels))
GL_PROC(_GL,void,glShadeModel,(GLenum mode))
GL_PROC(_GL,void,glTexCoord2f,(GLfloat s, GLfloat t))
GL_PROC(_GL,void,glTexCoordPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
GL_PROC(_GL,void,glTexEnvf,(GLenum target, GLenum pname, GLfloat param))
GL_PROC(_GL,void,glTexEnvi,(GLenum target, GLenum pname, GLint param))
GL_PROC(_GL,void,glTexImage2D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels))
GL_PROC(_GL,void,glTexParameterf,(GLenum target, GLenum pname, GLfloat param))
GL_PROC(_GL,void,glTexParameteri,(GLenum target, GLenum pname, GLint param))
GL_PROC(_GL,void,glTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))
GL_PROC(_GL,void,glVertex3f,(GLfloat x, GLfloat y, GLfloat z))
GL_PROC(_GL,void,glVertex3fv,(const GLfloat *v))
GL_PROC(_GL,void,glVertexPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
GL_PROC(_GL,void,glViewport,(GLint x, GLint y, GLsizei width, GLsizei height))
GL_PROC(_GL,void,glClipPlane,(GLenum,const GLdouble*))
GL_PROC(_GL,void,glSelectBuffer,(GLsizei size, GLuint *buffer))
GL_PROC(_GL,void,glInitNames,(void))
GL_PROC(_GL,void,glPopName,(void))
GL_PROC(_GL,void,glPushName,(GLuint name))
GL_PROC(_GL,GLint,glRenderMode,(GLenum mode))
GL_PROC(_GL,void,glColor4ub,(GLubyte r, GLubyte g, GLubyte b, GLubyte a))
#ifdef IMMEDIATE_MODE_DRAWARRAYS
GL_PROC(_GL,void,glSecondaryColor3ub,(GLubyte r, GLubyte g, GLubyte b))
#endif
#endif

#ifdef WIN32
// WGL functions.
GL_PROC(_GL,BOOL,wglCopyContext,(HGLRC,HGLRC,UINT))
GL_PROC(_GL,HGLRC,wglCreateContext,(HDC))
GL_PROC(_GL,HGLRC,wglCreateLayerContext,(HGLRC))
GL_PROC(_GL,BOOL,wglDeleteContext,(HGLRC))
GL_PROC(_GL,HGLRC,wglGetCurrentContext,(VOID))
GL_PROC(_GL,HDC,wglGetCurrentDC,(VOID))
GL_PROC(_GL,PROC,wglGetProcAddress,(LPCSTR))
GL_PROC(_GL,BOOL,wglMakeCurrent,(HDC, HGLRC))
GL_PROC(_GL,BOOL,wglShareLists,(HGLRC,HGLRC))

// GDI functions.
GL_PROC(_GL,INT,ChoosePixelFormat,(HDC hDC,CONST PIXELFORMATDESCRIPTOR*))
GL_PROC(_GL,INT,DescribePixelFormat,(HDC,INT,UINT,PIXELFORMATDESCRIPTOR*))
GL_PROC(_GL,BOOL,GetPixelFormat,(HDC))
GL_PROC(_GL,BOOL,SetPixelFormat,(HDC,INT,CONST PIXELFORMATDESCRIPTOR*))
GL_PROC(_GL,BOOL,SwapBuffers,(HDC hDC))
#endif

#endif

/*-----------------------------------------------------------------------------
	OpenGL extensions.
-----------------------------------------------------------------------------*/

// BGRA textures.
GL_EXT(_GL_EXT_bgra)

// Paletted textures.
GL_EXT(_GL_EXT_paletted_texture)
GL_PROC(_GL_EXT_paletted_texture,void,glColorTableEXT,(GLenum target,GLenum internalFormat,GLsizei width,GLenum format,GLenum type,const void *data))
GL_PROC(_GL_EXT_paletted_texture,void,glColorSubTableEXT,(GLenum target,GLsizei start,GLsizei count,GLenum format,GLenum type,const void *data))
GL_PROC(_GL_EXT_paletted_texture,void,glGetColorTableEXT,(GLenum target,GLenum format,GLenum type,void *data))
GL_PROC(_GL_EXT_paletted_texture,void,glGetColorTableParameterivEXT,(GLenum target,GLenum pname,int *params))
GL_PROC(_GL_EXT_paletted_texture,void,glGetColorTableParameterfvEXT,(GLenum target,GLenum pname,float *params))

// Generic texture compression.
GL_EXT(_GL_ARB_texture_compression)
GL_PROC(_GL_ARB_texture_compression,void,glCompressedTexSubImage2DARB,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *))
GL_PROC(_GL_ARB_texture_compression,void,glCompressedTexImage2DARB,(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *))

// S3TC texture compression.
GL_EXT(_GL_EXT_texture_compression_s3tc)

// Used for detail textures
GL_EXT(_GL_EXT_texture_env_combine)
GL_EXT(_GL_ARB_texture_env_combine)

// Anisotropic filtering
GL_EXT(_GL_EXT_texture_filter_anisotropic)

// SGIS texture lod
GL_EXT(_GL_SGIS_texture_lod)

// Used for better detail texture approach
GL_EXT(_GL_NV_texture_env_combine4)

// Texture LOD bias
GL_EXT(_GL_EXT_texture_lod_bias)

// Compiled vertex arrays.
GL_EXT(_GL_EXT_compiled_vertex_array)
GL_PROC(_GL_EXT_compiled_vertex_array,void,glLockArraysEXT,(GLint first, GLsizei count))
GL_PROC(_GL_EXT_compiled_vertex_array,void,glUnlockArraysEXT,(void))

// Fog Coord
//GL_EXT(_GL_EXT_fog_coord)
//GL_PROC(_GL_EXT_fog_coord,void,glFogCoordPointerEXT,(GLenum type, GLsizei stride, GLvoid *pointer))

// Secondary Color
GL_EXT(_GL_EXT_secondary_color)
GL_PROC(_GL_EXT_secondary_color,void,glSecondaryColorPointerEXT,(GLint size, GLenum type, GLsizei stride, GLvoid *pointer))

// ARB multitexture.
GL_EXT(_GL_ARB_multitexture)
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord1fARB,(GLenum target,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord2fARB,(GLenum target,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord3fARB,(GLenum target,GLfloat,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord4fARB,(GLenum target,GLfloat,GLfloat,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord1fvARB,(GLenum target,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord2fvARB,(GLenum target,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord3fvARB,(GLenum target,GLfloat,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glMultiTexCoord4fvARB,(GLenum target,GLfloat,GLfloat,GLfloat,GLfloat))
GL_PROC(_GL_ARB_multitexture,void,glActiveTextureARB,(GLenum target))
GL_PROC(_GL_ARB_multitexture,void,glClientActiveTextureARB,(GLenum target))

GL_EXT(_GL_NV_vertex_array_range)
GL_PROC(_GL_NV_vertex_array_range,void,glFlushVertexArrayRangeNV,(void))
GL_PROC(_GL_NV_vertex_array_range,void,glVertexArrayRangeNV,(GLsizei, const GLvoid *))
#ifdef WIN32
GL_PROC(_GL_NV_vertex_array_range,void*,wglAllocateMemoryNV,(GLsizei, GLfloat, GLfloat, GLfloat))
GL_PROC(_GL_NV_vertex_array_range,void,wglFreeMemoryNV,(void *))
#else
GL_PROC(_GL_NV_vertex_array_range,void*,glXAllocateMemoryNV,(GLsizei, GLfloat, GLfloat, GLfloat))
GL_PROC(_GL_NV_vertex_array_range,void,glXFreeMemoryNV,(void *))
#endif

GL_EXT(_GL_APPLE_vertex_array_range)
GL_PROC(_GL_APPLE_vertex_array_range,void,glVertexArrayRangeAPPLE,(GLsizei, const GLvoid *))
GL_PROC(_GL_APPLE_vertex_array_range,void,glFlushVertexArrayRangeAPPLE,(GLsizei, const GLvoid *))
GL_PROC(_GL_APPLE_vertex_array_range,void,glVertexArrayParameteriAPPLE,(GLenum, GLint))

#if 0  // not used.
GL_EXT(_GL_NV_fence)
GL_PROC(_GL_NV_fence,void,glGenFencesNV,(GLsizei n, GLuint *fences))
GL_PROC(_GL_NV_fence,void,glDeleteFencesNV,(GLsizei n, const GLuint *fences))
GL_PROC(_GL_NV_fence,void,glSetFenceNV,(GLuint fence, GLenum condition))
GL_PROC(_GL_NV_fence,GLboolean,glTestFenceNV,(GLuint fence))
GL_PROC(_GL_NV_fence,void,glFinishFenceNV,(GLuint fence))
GL_PROC(_GL_NV_fence,GLboolean,glIsFenceNV,(GLuint fence))
GL_PROC(_GL_NV_fence,void,glGetFenceivNV,(GLuint fence, GLenum pname, GLint *params))

GL_EXT(_GL_APPLE_fence)
GL_PROC(_GL_APPLE_fence,void,glGenFencesAPPLE,(GLsizei n, GLuint *fences))
GL_PROC(_GL_APPLE_fence,void,glDeleteFencesAPPLE,(GLsizei n, const GLuint *fences))
GL_PROC(_GL_APPLE_fence,void,glSetFenceAPPLE,(GLuint fence))
GL_PROC(_GL_APPLE_fence,GLboolean,glTestFenceAPPLE,(GLuint fence))
GL_PROC(_GL_APPLE_fence,void,glFinishFenceAPPLE,(GLuint fence))
GL_PROC(_GL_APPLE_fence,GLboolean,glIsFenceAPPLE,(GLuint fence))
GL_PROC(_GL_APPLE_fence,GLboolean,glTestObjectAPPLE,(GLenum object, GLuint name))
GL_PROC(_GL_APPLE_fence,void,glFinishObjectAPPLE(GLenum object, GLuint name))
#endif

GL_EXT(_GL_ATI_array_rev_comps_in_4_bytes)

// FIXME: remove this?
#ifndef GL_ARRAY_REV_COMPS_IN_4_BYTES_ATI
#define GL_ARRAY_REV_COMPS_IN_4_BYTES_ATI 0x897C
#endif

#ifndef GL_VERTEX_ARRAY_STORAGE_HINT_APPLE
#define GL_VERTEX_ARRAY_STORAGE_HINT_APPLE	0x851F
#endif

#ifndef GL_STORAGE_CACHED_APPLE
#define GL_STORAGE_CACHED_APPLE				0x85BE
#endif

#ifndef GL_STORAGE_SHARED_APPLE
#define GL_STORAGE_SHARED_APPLE				0x85BF
#endif

#ifndef GL_VERTEX_ARRAY_RANGE_APPLE
#define GL_VERTEX_ARRAY_RANGE_APPLE			0x851D
#endif

#ifndef GL_FILTER4_SGIS
#define GL_FILTER4_SGIS                   0x8146
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/


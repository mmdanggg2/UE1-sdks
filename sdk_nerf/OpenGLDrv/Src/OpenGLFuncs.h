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

GL_EXT(_GL)
GL_PROC(_GL,void,glBindBuffer,(GLenum target, GLuint buffer))
GL_PROC(_GL,void,glBindVertexArray,(GLuint array))
GL_PROC(_GL,void,glBufferData,(GLenum target, GLsizeiptr size, const void *data, GLenum usage))
GL_PROC(_GL,void,glDeleteBuffers,(GLsizei n, const GLuint *buffers))
GL_PROC(_GL,void,glDeleteVertexArrays,(GLsizei n, const GLuint *arrays))
GL_PROC(_GL,void,glDisableVertexAttribArray,(GLuint index))
//GL_PROC(_GL,void,glDrawArrays,(GLenum mode, GLint first, GLsizei count))
GL_PROC(_GL,void,glEnableVertexAttribArray,(GLuint index))
GL_PROC(_GL,void,glGenBuffers,(GLsizei n, GLuint *buffers))
GL_PROC(_GL,void,glGenVertexArrays,(GLsizei n, GLuint *arrays))
GL_PROC(_GL,GLboolean,glIsBuffer,(GLuint buffer))
GL_PROC(_GL,void,glVertexAttribIPointer,(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
GL_PROC(_GL,void,glVertexAttribPointer,(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer))
GL_PROC(_GL,void,glActiveTexture,(GLenum texture))

// Uniforms.
GL_PROC(_GL,GLint,glGetUniformLocation,(GLuint program, const GLchar *name))
GL_PROC(_GL,void,glGetUniformfv,(GLuint program, GLint location, GLfloat *params))
GL_PROC(_GL,void,glGetUniformiv,(GLuint program, GLint location, GLint *params))
GL_PROC(_GL,void,glUniform1f,(GLint location, GLfloat v0))
GL_PROC(_GL,void,glUniform2f,(GLint location, GLfloat v0, GLfloat v1))
GL_PROC(_GL,void,glUniform3f,(GLint location, GLfloat v0, GLfloat v1, GLfloat v2))
GL_PROC(_GL,void,glUniform4f,(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))
GL_PROC(_GL,void,glUniform1i,(GLint location, GLint v0))
GL_PROC(_GL,void,glUniform2i,(GLint location, GLint v0, GLint v1))
GL_PROC(_GL,void,glUniform3i,(GLint location, GLint v0, GLint v1, GLint v2))
GL_PROC(_GL,void,glUniform4i,(GLint location, GLint v0, GLint v1, GLint v2, GLint v3))
GL_PROC(_GL,void,glUniform1fv,(GLint location, GLsizei count, const GLfloat *value))
GL_PROC(_GL,void,glUniform2fv,(GLint location, GLsizei count, const GLfloat *value))
GL_PROC(_GL,void,glUniform3fv,(GLint location, GLsizei count, const GLfloat *value))
GL_PROC(_GL,void,glUniform4fv,(GLint location, GLsizei count, const GLfloat *value))
GL_PROC(_GL,void,glUniform1iv,(GLint location, GLsizei count, const GLint *value))
GL_PROC(_GL,void,glUniform2iv,(GLint location, GLsizei count, const GLint *value))
GL_PROC(_GL,void,glUniform3iv,(GLint location, GLsizei count, const GLint *value))
GL_PROC(_GL,void,glUniform4iv,(GLint location, GLsizei count, const GLint *value))
GL_PROC(_GL,void,glUniformMatrix2fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
GL_PROC(_GL,void,glUniformMatrix3fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
GL_PROC(_GL,void,glUniformMatrix4fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))

// Programs and Shaders.
GL_PROC(_GL,void,glAttachShader,(GLuint program, GLuint shader))
GL_PROC(_GL,void,glCompileShader,(GLuint shader))
GL_PROC(_GL,GLuint,glCreateProgram,(void))
GL_PROC(_GL,GLuint,glCreateShader,(GLenum type))
GL_PROC(_GL,void,glDeleteProgram,(GLuint program))
GL_PROC(_GL,void,glDeleteShader,(GLuint shader))
GL_PROC(_GL,void,glDetachShader,(GLuint program, GLuint shader))
GL_PROC(_GL,void,glGetAttachedShaders,(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders))
GL_PROC(_GL,void,glGetProgramiv,(GLuint program, GLenum pname, GLint *params))
GL_PROC(_GL,void,glGetProgramInfoLog,(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog))
GL_PROC(_GL,void,glGetShaderiv,(GLuint shader, GLenum pname, GLint *params))
GL_PROC(_GL,void,glGetShaderInfoLog,(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog))
GL_PROC(_GL,void,glGetShaderSource,(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source))
GL_PROC(_GL,GLboolean,glIsProgram,(GLuint program))
GL_PROC(_GL,GLboolean,glIsShader,(GLuint shader))
GL_PROC(_GL,void,glLinkProgram,(GLuint program))
GL_PROC(_GL,void,glShaderSource,(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length))
GL_PROC(_GL,void,glUseProgram,(GLuint program))
GL_PROC(_GL,void,glValidateProgram,(GLuint program))

// Texture compression.
GL_PROC(_GL,void,glCompressedTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data))
GL_PROC(_GL,void,glCompressedTexImage2D,(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data))

/*-----------------------------------------------------------------------------
	OpenGL extensions.
-----------------------------------------------------------------------------*/

// BGRA textures.
GL_EXT(_GL_EXT_bgra)

// S3TC texture compression.
GL_EXT(_GL_EXT_texture_compression_s3tc)

// Anisotropic filtering
GL_EXT(_GL_EXT_texture_filter_anisotropic)

// Used for detail textures
GL_EXT(_GL_EXT_texture_env_combine)
GL_EXT(_GL_ARB_texture_env_combine)

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

/*-----------------------------------------------------------------------------
	WGL extensions bootstraping.
-----------------------------------------------------------------------------*/

GL_PROC(_GL,const char *,wglGetExtensionsStringARB,(HDC hdc))

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

/*=============================================================================
	WGLFuncs.h: WGL function-declaration macros.

	Revision history:
		* Created by Sebastian Kaufel
=============================================================================*/

// VSync.
GL_EXT(_WGL_EXT_swap_control)
GL_PROC(_WGL_EXT_swap_control,BOOL,wglSwapIntervalEXT,(int interval))

// Adaptive VSync.
GL_EXT(_WGL_EXT_swap_control_tear)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

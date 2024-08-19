/*=============================================================================
	OpenGL_TransformationMatrix.cpp: OpenGL view matrix handling (ARB path).

	Revision history:
	* Moved projection states from OpenGL.cpp
	* Single controller refactor by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "FOpenGL12.h"


void UOpenGLRenderDevice::SetTransformationModeNoCheck( DWORD Mode)
{
	// SetSceneNode has not been called yet.
	if ( m_RProjZ == 0 )
		return;

	switch ( Mode )
	{
		case MATRIX_OrthoProjection:
		{
			SceneParams.Mode = MATRIX_OrthoProjection;

			//Select projection matrix and reset to identity
			FOpenGL12::glMatrixMode(GL_PROJECTION);
			FOpenGL12::glLoadIdentity();
			FOpenGL12::glOrtho(-m_RProjZ, +m_RProjZ, -m_Aspect*m_RProjZ, +m_Aspect*m_RProjZ, 1.0 * 0.5, 32768.0);
			break;
		}
		case MATRIX_Projection:
		{
			SceneParams.Mode = MATRIX_Projection;

			//Select projection matrix and reset to identity
			FOpenGL12::glMatrixMode(GL_PROJECTION);
			FOpenGL12::glLoadIdentity();
			FOpenGL12::glFrustum(-m_RProjZ * zNear, +m_RProjZ * zNear, -m_Aspect*m_RProjZ * zNear, +m_Aspect*m_RProjZ * zNear, 1.0 * zNear, zFar);
			break;
		}
	}
}

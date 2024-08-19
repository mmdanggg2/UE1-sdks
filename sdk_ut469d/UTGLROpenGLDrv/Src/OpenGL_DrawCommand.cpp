/*=============================================================================
	OpenGL_DrawCommand.cpp

	Draw command pool and tracking implementation

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"


FMemStack FGL::Draw::CmdMem;
FMemMark  FGL::Draw::CmdMark;

static bool CmdMemInit = false;

void FGL::Draw::InitCmdMem()
{
	using namespace FGL::Draw;
	if ( !CmdMemInit )
	{
		CmdMemInit = true;
		CmdMem.Init(65536);
		CmdMark = FMemMark(CmdMem);
	}
}

void FGL::Draw::ExitCmdMem()
{
	using namespace FGL::Draw;
	CmdMem.Exit();
	CmdMark.Pop();
}

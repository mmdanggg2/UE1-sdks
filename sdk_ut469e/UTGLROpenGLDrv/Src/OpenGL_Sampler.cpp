/*=============================================================================
	OpenGL_Sampler.cpp: Sampler handling code for UTGLR OpenGLDrv.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"


void UOpenGLRenderDevice::UpdateSamplers( UBOOL& FlushTextures)
{
	// Do not use Samplers for now, will instead be forced for GLES and Core
	if ( !FOpenGLBase::SupportsSamplerObjects || true )
		return;

	if ( !m_SamplerInit )
	{
		appMemzero( SamplerList, sizeof(SamplerList));
		appMemzero( SamplerBindings, sizeof(SamplerBindings));

		// Create samplers in their default states.
		// Samplers can be shared between contexts, but here they heavily depend on
		// each OpenGLRenderDevice's settings so we won't be sharing those.
		FOpenGLBase::glGenSamplers( FGL::SamplerModel::SAMPLER_COMBINATIONS, SamplerList);

		for ( INT i=0; i<FGL::SamplerModel::SAMPLER_COMBINATIONS; i++)
		{
			const INT SamplerID = SamplerList[i];
			const bool MipMaps  = (i & FGL::SamplerModel::SAMPLER_BIT_MIPMAPS)   != 0;
			const bool NoSmooth = (i & FGL::SamplerModel::SAMPLER_BIT_NO_SMOOTH) != 0;

			FOpenGLBase::glSamplerParameteri( SamplerID, GL_TEXTURE_MIN_FILTER, FGL::GetFilter(MipMaps,NoSmooth));
			FOpenGLBase::glSamplerParameteri( SamplerID, GL_TEXTURE_MAG_FILTER, FGL::GetMagnificationFilter(NoSmooth));
		}

		// Initialize state locks required by the Sampler system.
		PL_MaxAnisotropy = 0;
		PL_LODBias = 0;
		PL_UseTrilinear = 0;
		PL_ForceNoSmooth = 0;

		m_SamplerInit = true;
	}

	if ( MaxAnisotropy != PL_MaxAnisotropy )
	{
		if ( FOpenGLBase::SupportsAnisotropy )
		{
			SetDefaultSamplerState();
			GLfloat Param = MaxAnisotropy ? (GLfloat)MaxAnisotropy : 1.0f;
			for ( INT i=0; i<FGL::SamplerModel::SAMPLER_COMBINATIONS; i++)
				if ( FGL::SamplerModel::UsesMaxAnisotropy(i) )
					FOpenGLBase::glSamplerParameterf( SamplerList[i], GL_TEXTURE_MAX_ANISOTROPY_EXT, Param);
		}
	}

	if ( LODBias != PL_LODBias )
	{
		SetDefaultSamplerState();
		for ( INT i=0; i<FGL::SamplerModel::SAMPLER_COMBINATIONS; i++)
			if ( FGL::SamplerModel::UsesLODBias(i) )
				FOpenGLBase::glSamplerParameterf( SamplerList[i], GL_TEXTURE_LOD_BIAS, LODBias);
	}

	if ( UseTrilinear != PL_UseTrilinear )
	{
		SetDefaultSamplerState();
		const bool UseTrilinear = (this->UseTrilinear != 0);
		for ( INT i=0; i<FGL::SamplerModel::SAMPLER_COMBINATIONS; i++)
			if ( FGL::SamplerModel::UsesLODBias(i) )
			{
				const bool NoSmooth = (i & FGL::SamplerModel::SAMPLER_BIT_NO_SMOOTH) != 0;
				FOpenGLBase::glSamplerParameterf( SamplerList[i], GL_TEXTURE_MIN_FILTER, FGL::GetMinificationFilter(NoSmooth,UseTrilinear));
			}
	}

}


/*-----------------------------------------------------------------------------
	Sampler object handling.
-----------------------------------------------------------------------------*/

static inline void InternalBindSampler( UOpenGLRenderDevice* GL, GLuint TMUnit, GLuint Sampler)
{
	if ( GL->SamplerBindings[TMUnit] != Sampler )
	{
		FOpenGLBase::glBindSampler( TMUnit, Sampler);
		GL->SamplerBindings[TMUnit] = Sampler;
	}
}

void UOpenGLRenderDevice::DestroySamplers()
{
	if ( m_SamplerInit )
	{
		SetDefaultSamplerState();
		FOpenGLBase::glDeleteSamplers( FGL::SamplerModel::SAMPLER_COMBINATIONS, SamplerList);
		appMemzero( SamplerList, sizeof(SamplerList));
		m_SamplerInit = false;
	}
}

void FASTCALL UOpenGLRenderDevice::SetSampler( INT Multi, UBOOL Mips, UBOOL AlwaysSmooth, DWORD PolyFlags)
{
	guardSlow(UOpenGLRenderDevice::SetSampler);
	if ( false && m_SamplerInit )
	{
		GLuint SamplerID = 0;
		if ( Mips )
			SamplerID |= FGL::SamplerModel::SAMPLER_BIT_MIPMAPS;
		if ( ((PolyFlags & PF_NoSmooth) || ForceNoSmooth) && !AlwaysSmooth )
			SamplerID |= FGL::SamplerModel::SAMPLER_BIT_NO_SMOOTH;
		// TODO: implement 227 clamp mode
		InternalBindSampler( this, Multi, SamplerList[SamplerID]);
	}
	unguardSlow;
}

void FASTCALL UOpenGLRenderDevice::SetNoSampler( INT Multi)
{
	// Note: InternalBindSampler only changes state if new Sampler is != old Sampler
	// Because 0 is a default value, a system that doesn't use Sampler Objects will
	// will never attempt to change the sampler object state (even without the check)
//	if ( UseSamplerObjects )
		InternalBindSampler( this, Multi, 0);
}

void UOpenGLRenderDevice::SetDefaultSamplerState()
{
	if ( m_SamplerInit && FOpenGLBase::SupportsSamplerObjects )
	{
//		for ( INT i=0; i<MAX_TMUNITS; i++)
//			InternalBindSampler( this, i, 0);
	}
}

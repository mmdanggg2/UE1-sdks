/*=============================================================================
	FOpenGLBase_Inlines.h
	
	FOpenGLBase functions that directly interact with other abstractions.
	These need to be defined last.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#ifndef _INC_GL_FOPENGLBASE_INLINES
#define _INC_GL_FOPENGLBASE_INLINES


/*-----------------------------------------------------------------------------
	TextureUnitState.
-----------------------------------------------------------------------------*/

inline void FOpenGLBase::TextureUnitState::Reset()
{
	SetActiveNoCheck(0);
	for ( INT i=0; i<ARRAY_COUNT(Bound); i++)
		Bound[i] = nullptr;
}

inline void FOpenGLBase::TextureUnitState::SetActive( GLuint Unit)
{
	GL_DEV_CHECK(Unit < GL_MAX_TEXTURE_UNITS);
	if ( Active != Unit )
		SetActiveNoCheck(Unit);
}

inline void FOpenGLBase::TextureUnitState::SetActiveNoCheck( GLuint Unit)
{
	Active = Unit;
	FOpenGLBase::glActiveTexture(GL_TEXTURE0_ARB + Unit);
}

inline void FOpenGLBase::TextureUnitState::Bind( const FOpenGLTexture& Texture)
{
	GL_DEV_CHECK(Texture.Target != 0);

	FOpenGLTexture** Current = &Bound[Active];
	if ( *Current )
	{
		GL_DEV_CHECK((*Current)->Target != 0);

		if ( *Current == &Texture )
			return;

		// If target is different, explicitly unbind current
		GLenum CurrentTarget = (*Current)->Target;
		if ( CurrentTarget != Texture.Target )
			FOpenGLBase::glBindTexture(CurrentTarget, 0);
	}
	*Current = (FOpenGLTexture*)&Texture;
	FOpenGLBase::glBindTexture(Texture.Target, Texture.Texture);
}

inline void FOpenGLBase::TextureUnitState::Unbind()
{
	FOpenGLTexture** Current = &Bound[Active];
	if ( *Current )
	{
		FOpenGLBase::glBindTexture((*Current)->Target, 0);
		*Current = nullptr;
	}
}

inline void FOpenGLBase::TextureUnitState::UnbindAll()
{
	for ( GLuint i=0; i<GL_MAX_TEXTURE_UNITS; i++)
		if ( Bound[i] )
		{
			SetActiveNoCheck(i);
			FOpenGLBase::glBindTexture(Bound[i]->Target, 0);
			Bound[i] = nullptr;
		}
	SetActive(0);
}

inline bool FOpenGLBase::TextureUnitState::IsBound( FOpenGLTexture& Texture, GLuint Unit) const
{
	GL_DEV_CHECK(Unit < GL_MAX_TEXTURE_UNITS);
	return Bound[Unit] == &Texture;
}

inline bool FOpenGLBase::TextureUnitState::IsUnbound( GLuint Unit) const
{
	GL_DEV_CHECK(Unit < GL_MAX_TEXTURE_UNITS);
	return Bound[Unit] == nullptr;
}


inline void FOpenGLBase::TextureUnitState::SetSwizzle( FOpenGLSwizzle InSwizzle)
{
	GL_DEV_CHECK(Bound[Active]);
	GLenum Target = Bound[Active]->Target;

	for ( GLint i=0; i<4; i++)
		if ( InSwizzle.Channel[i] != SWZ_UNDEFINED )
		{
			FOpenGLBase::glTexParameteri(Target, SwizzleParameters[i], FOpenGLSwizzle::ToGL((ESwizzle)InSwizzle.Channel[i]) );
			Bound[Active]->Swizzle.Channel[i] = InSwizzle.Channel[i];
		}
}

#endif

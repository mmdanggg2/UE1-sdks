/*=============================================================================
	OpenGL_SamplerModel.h

	Filter parameter helpers.
	Sampler identification standard.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


/*
Add new Table 6.23, "Textures (state per sampler object)", renumber subsequent tables.

+---------------------------+-----------+----------------------+------------------------+------------------------------+--------+------------+
| Get Value                 | Type      | Get Command          | Initial Value          | Description                  | Sec    | Attribute  |
+---------------------------+-----------+----------------------+------------------------+------------------------------+--------+------------+
| TEXTURE_BORDER_COLOR      | n x C     | GetSamplerParameter  | 0,0,0,0                | Border color                 | 3.9    | -          |
| TEXTURE_MIN_FILTER        | n x Z6    | GetSamplerParameter  | NEAREST_MIPMAP_LINEAR  | Minification function        | 3.9.9  | -          |
| TEXTURE_MAG_FILTER        | n x Z2    | GetSamplerParameter  | LINEAR                 | Magnification function       | 3.9.10 | -          |
| TEXTURE_WRAP_S            | n x Z5    | GetSamplerParameter  | REPEAT                 | Texcoord s wrap mode         | 3.9.9  | -          |
| TEXTURE_WRAP_T            | n x Z5    | GetSamplerParameter  | REPEAT                 | Texcoord t wrap mode         | 3.9.9  | -          |
| TEXTURE_WRAP_R            | n x Z5    | GetSamplerParameter  | REPEAT                 | Texcoord r wrap mode         | 3.9.9  | -          |
| TEXTURE_MIN_LOD           | n x R     | GetSamplerParameter  | -1000                  | Minimum level of detail      | 3.9    | -          |
| TEXTURE_MAX_LOD           | n x R     | GetSamplerParameter  | 1000                   | Maximum level of detail      | 3.9    | -          |
| TEXTURE_LOD_BIAS          | n x R     | GetSamplerParameter  | 0.0                    | Texture level of detail      | 3.9.9  | -          |
|                           |           |                      |                        | bias (biastexobj)            |        |            |
| TEXTURE_COMPARE_MODE      | n x Z2    | GetSamplerParameter  | NONE                   | Comparison mode              | 3.9.16 | -          |
| TEXTURE_COMPARE_FUNC      | n x Z8    | GetSamplerParameter  | LEQUAL                 | Comparison function          | 3.9.16 | -          |
| TEXTURE_MAX_ANISOTROPY_EXT| n x R     | GetSamplerParameter  | 1.0                    | Maximum degree of anisotropy | 3.9    | -          |
+---------------------------+-----------+----------------------+------------------------+------------------------------+--------+------------+
*/

/**
	Global states that modify samplers:
	- [MaxAnisotropy]     - GL_TEXTURE_MAX_ANISOTROPY_EXT
	- [UseTrilinear]      - GL_TEXTURE_MIN_FILTER
	- [LodBias]           - GL_TEXTURE_LOD_BIAS_EXT
*/



/*-----------------------------------------------------------------------------
	Sampler descriptors.
-----------------------------------------------------------------------------*/

namespace FGL
{
	// Nearest filtering choice
	static inline GLenum GetNearestMinificationFilter( bool UseTrilinear)
	{
		return UseTrilinear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
	}

	// Linear filtering choice
	static inline GLenum GetLinearMinificationFilter( bool UseTrilinear)
	{
		return UseTrilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;
	};

	// Magnification filtering choice
	static inline GLenum GetMagnificationFilter( bool NoSmooth)
	{
		return NoSmooth ? GL_NEAREST : GL_LINEAR;
	}

	// Minification filtering choice
	static inline GLenum GetMinificationFilter( bool NoSmooth, bool UseTrilinear=false)
	{
		return NoSmooth ? GetNearestMinificationFilter(UseTrilinear) : GetLinearMinificationFilter(UseTrilinear);
	}

	// Filtering choice
	static inline GLenum GetFilter( bool MipMaps, bool NoSmooth, bool UseTrilinear=false)
	{
		return MipMaps ? GetMagnificationFilter(NoSmooth) : GetMinificationFilter(NoSmooth, UseTrilinear);
	}


	namespace SamplerModel
	{
		// Sampler identification
		enum
		{
			SAMPLER_BIT_NO_SMOOTH     = 1 << 0,
			SAMPLER_BIT_MIPMAPS       = 1 << 1,

			SAMPLER_BIT_ALL           = SAMPLER_BIT_NO_SMOOTH | SAMPLER_BIT_MIPMAPS,
			SAMPLER_COMBINATIONS      = SAMPLER_BIT_ALL + 1,
		};

		// Deduce if an identified sampler uses anisotropic filter
		static inline bool UsesMaxAnisotropy( INT SamplerID)
		{
			return (SamplerID & SAMPLER_BIT_MIPMAPS) && !(SamplerID & SAMPLER_BIT_NO_SMOOTH);
		}

		// Deduce if an identified sampler uses LOD bias
		static inline bool UsesLODBias( INT SamplerID)
		{
			return (SamplerID & SAMPLER_BIT_MIPMAPS) != 0;
		}

	}; /* namespace SamplerModel */
}; /* namespace FGL */

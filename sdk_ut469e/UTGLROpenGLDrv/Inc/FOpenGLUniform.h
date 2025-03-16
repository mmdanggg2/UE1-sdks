/*=============================================================================
	FOpenGLUniform.h

	Simple Uniform descriptor, implementation is left to FOpenGL(version)

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

template < typename T > class FOpenGLUniform
{
protected:
	friend class FOpenGLBase;
	friend class FOpenGL3;

	GLint Location;
	T     Value;

public:
	FOpenGLUniform();
};


/*-----------------------------------------------------------------------------
	FOpenGLUniform.
-----------------------------------------------------------------------------*/

template<typename T>
inline FOpenGLUniform<T>::FOpenGLUniform()
{
}

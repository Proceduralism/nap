#include "uniforms.h"
#include "nglutils.h"
#include "imagefromfile.h"


RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::Uniform)
	RTTI_PROPERTY("Name", &nap::Uniform::mName, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformValueArray)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformTexture)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformTextureArray)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformInt)
	RTTI_PROPERTY("Value", &nap::UniformInt::mValue, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setValue", &nap::UniformInt::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformFloat)
	RTTI_PROPERTY("Value", &nap::UniformFloat::mValue, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setValue", &nap::UniformFloat::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec3)
	RTTI_PROPERTY("Value", &nap::UniformVec3::mValue, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setValue", &nap::UniformVec3::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec4)
	RTTI_PROPERTY("Value", &nap::UniformVec4::mValue, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setValue", &nap::UniformVec4::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformMat4)
	RTTI_PROPERTY("Value", &nap::UniformMat4::mValue, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setValue", &nap::UniformMat4::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformTexture2D)
	RTTI_PROPERTY("Texture", &nap::UniformTexture2D::mTexture, nap::rtti::EPropertyMetaData::Required)
	RTTI_FUNCTION("setTexture", &nap::UniformTexture2D::setTexture)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

RTTI_BEGIN_CLASS(nap::UniformIntArray)
	RTTI_PROPERTY("Values", &nap::UniformIntArray::mValues, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformFloatArray)
	RTTI_PROPERTY("Values", &nap::UniformFloatArray::mValues, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec3Array)
	RTTI_PROPERTY("Values", &nap::UniformVec3Array::mValues, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec4Array)
	RTTI_PROPERTY("Values", &nap::UniformVec4Array::mValues, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformMat4Array)
	RTTI_PROPERTY("Values", &nap::UniformMat4Array::mValues, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformTexture2DArray)
	RTTI_PROPERTY("Textures", &nap::UniformTexture2DArray::mTextures, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
	void UniformInt::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform1iv(declaration.mLocation, declaration.mSize, static_cast<const GLint*>(&mValue));
		glAssert();
	}


	void UniformFloat::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform1fv(declaration.mLocation, declaration.mSize, static_cast<const GLfloat*>(&mValue));
		glAssert();
	}


	void UniformVec3::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform3fv(declaration.mLocation, declaration.mSize, static_cast<const GLfloat*>(&mValue.x));
		glAssert();
	}


	void UniformVec4::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform4fv(declaration.mLocation, declaration.mSize, static_cast<const GLfloat*>(&mValue.x));
		glAssert();
	}


	void UniformMat4::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniformMatrix4fv(declaration.mLocation, declaration.mSize, GL_FALSE, static_cast<const GLfloat*>(&mValue[0].x));
		glAssert();
	}


	int UniformTexture2D::push(const opengl::UniformDeclaration& declaration, int textureUnit) const
	{
		if (mTexture == nullptr)
			return 0;

		glActiveTexture(GL_TEXTURE0 + textureUnit);
		mTexture->bind();
		glUniform1iv(declaration.mLocation, declaration.mSize, static_cast<const GLint*>(&textureUnit));

		return 1;
	}

	//////////////////////////////////////////////////////////////////////////

	void UniformIntArray::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform1iv(declaration.mLocation, declaration.mSize, static_cast<const GLint*>(mValues.data()));
		glAssert();
	}


	void UniformFloatArray::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform1fv(declaration.mLocation, declaration.mSize, static_cast<const GLfloat*>(mValues.data()));
		glAssert();
	}


	void UniformVec3Array::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform3fv(declaration.mLocation, declaration.mSize, (const GLfloat*)(mValues.data()));
		glAssert();
	}


	void UniformVec4Array::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniform4fv(declaration.mLocation, declaration.mSize, (const GLfloat*)(mValues.data()));
		glAssert();
	}


	void UniformMat4Array::push(const opengl::UniformDeclaration& declaration) const
	{
		glUniformMatrix4fv(declaration.mLocation, declaration.mSize, GL_FALSE, (const GLfloat*)(mValues.data()));
		glAssert();
	}
	
	int UniformTexture2DArray::push(const opengl::UniformDeclaration& declaration, int textureUnit) const
	{
		for (int index = 0; index < mTextures.size(); ++index)
		{
			int unit = textureUnit + index;
			glActiveTexture(GL_TEXTURE0 + unit);
			mTextures[index]->bind();
			glUniform1iv(declaration.mLocation, declaration.mSize, static_cast<const GLint*>(&unit));
		}

		return mTextures.size();
	}

} // End Namespace NAP

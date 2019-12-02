#include "uniforms.h"
#include "nglutils.h"


RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::Uniform)
	RTTI_PROPERTY("Name", &nap::Uniform::mName, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformValueArray)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformStruct)
	RTTI_PROPERTY("Uniforms", &nap::UniformStruct::mUniforms, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformStructArray)
	RTTI_PROPERTY("Structs", &nap::UniformStructArray::mStructs, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformInt)
	RTTI_PROPERTY("Value", &nap::UniformInt::mValue, nap::rtti::EPropertyMetaData::Required)
	//RTTI_FUNCTION("setValue", &nap::UniformInt::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformFloat)
	RTTI_PROPERTY("Value", &nap::UniformFloat::mValue, nap::rtti::EPropertyMetaData::Required)
	//RTTI_FUNCTION("setValue", &nap::UniformFloat::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec3)
	RTTI_PROPERTY("Value", &nap::UniformVec3::mValue, nap::rtti::EPropertyMetaData::Required)
	//RTTI_FUNCTION("setValue", &nap::UniformVec3::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformVec4)
	RTTI_PROPERTY("Value", &nap::UniformVec4::mValue, nap::rtti::EPropertyMetaData::Required)
	//RTTI_FUNCTION("setValue", &nap::UniformVec4::setValue)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniformMat4)
	RTTI_PROPERTY("Value", &nap::UniformMat4::mValue, nap::rtti::EPropertyMetaData::Required)
	//RTTI_FUNCTION("setValue", &nap::UniformMat4::setValue)
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

namespace nap
{
	std::unique_ptr<UniformInstance> createUniformFromDeclaration(const opengl::UniformDeclaration& declaration, const UniformCreatedCallback& uniformCreatedCallback)
	{
		rtti::TypeInfo declaration_type = declaration.get_type();

		if (declaration_type == RTTI_OF(opengl::UniformStructArrayDeclaration))
		{
			const opengl::UniformStructArrayDeclaration* struct_array_declaration = rtti_cast<const opengl::UniformStructArrayDeclaration>(&declaration);
			std::unique_ptr<UniformStructArrayInstance> struct_array_instance = std::make_unique<UniformStructArrayInstance>(*struct_array_declaration);
			for (auto& struct_declaration : struct_array_declaration->mElements)
			{
				std::unique_ptr<UniformStructInstance> struct_instance = std::make_unique<UniformStructInstance>(*struct_declaration, uniformCreatedCallback);
				struct_array_instance->addElement(std::move(struct_instance));
			}
			return std::move(struct_array_instance);
		}
		else if (declaration_type == RTTI_OF(opengl::UniformValueArrayDeclaration))
		{
			const opengl::UniformValueArrayDeclaration* value_array_declaration = rtti_cast<const opengl::UniformValueArrayDeclaration>(&declaration);

			if (value_array_declaration->mElementType == opengl::EGLSLType::Int)
			{
				std::unique_ptr<UniformIntArrayInstance> array_instance = std::make_unique<UniformIntArrayInstance>(*value_array_declaration);
				array_instance->getValues().resize(value_array_declaration->mNumElements);
				return std::move(array_instance);
			}
			else if (value_array_declaration->mElementType == opengl::EGLSLType::Float)
			{
				std::unique_ptr<UniformFloatArrayInstance> array_instance = std::make_unique<UniformFloatArrayInstance>(*value_array_declaration);
				array_instance->getValues().resize(value_array_declaration->mNumElements);
				return std::move(array_instance);
			}
			else if (value_array_declaration->mElementType == opengl::EGLSLType::Vec3)
			{
				std::unique_ptr<UniformVec3ArrayInstance> array_instance = std::make_unique<UniformVec3ArrayInstance>(*value_array_declaration);
				array_instance->getValues().resize(value_array_declaration->mNumElements);
				return std::move(array_instance);
			}
			else if (value_array_declaration->mElementType == opengl::EGLSLType::Vec4)
			{
				std::unique_ptr<UniformVec4ArrayInstance> array_instance = std::make_unique<UniformVec4ArrayInstance>(*value_array_declaration);
				array_instance->getValues().resize(value_array_declaration->mNumElements);
				return std::move(array_instance);
			}
			else if (value_array_declaration->mElementType == opengl::EGLSLType::Mat4)
			{
				std::unique_ptr<UniformMat4ArrayInstance> array_instance = std::make_unique<UniformMat4ArrayInstance>(*value_array_declaration);
				array_instance->getValues().resize(value_array_declaration->mNumElements);
				return std::move(array_instance);
			}
		}
		else if (declaration_type == RTTI_OF(opengl::UniformStructDeclaration))
		{
			const opengl::UniformStructDeclaration* struct_declaration = rtti_cast<const opengl::UniformStructDeclaration>(&declaration);
			return std::make_unique<UniformStructInstance>(*struct_declaration, uniformCreatedCallback);
		}
		else
		{
			const opengl::UniformValueDeclaration* value_declaration = rtti_cast<const opengl::UniformValueDeclaration>(&declaration);

			if (value_declaration->mType == opengl::EGLSLType::Int)
			{
				return std::make_unique<UniformIntInstance>(*value_declaration);
			}
			else if (value_declaration->mType == opengl::EGLSLType::Vec3)
			{
				return std::make_unique<UniformVec3Instance>(*value_declaration);
			}
			else if (value_declaration->mType == opengl::EGLSLType::Vec4)
			{
				return std::make_unique<UniformVec4Instance>(*value_declaration);
			}
			else if (value_declaration->mType == opengl::EGLSLType::Mat4)
			{
				return std::make_unique<UniformMat4Instance>(*value_declaration);
			}
		}

		return nullptr;
	}

	void UniformStructInstance::addUniform(std::unique_ptr<UniformInstance> uniform)
	{
		mUniforms.emplace_back(std::move(uniform));
	}

	void UniformStructArrayInstance::addElement(std::unique_ptr<UniformStructInstance> element)
	{
		mElements.emplace_back(std::move(element));
	}

	void UniformStruct::addUniform(Uniform& uniform)
	{
		mUniforms.push_back(&uniform);
	}

	Uniform* UniformStruct::findUniform(const std::string& name)
	{
		auto pos = std::find_if(mUniforms.begin(), mUniforms.end(), [name](auto& uniform)
		{
			return uniform->mName == name;
		});

		if (pos == mUniforms.end())
			return nullptr;

		return (*pos).get();
	}

	void UniformStructArray::insertStruct(int index, UniformStruct& uniformStruct)
	{
		if (mStructs.size() <= index)
			mStructs.resize(index + 1);
		
		mStructs[index] = &uniformStruct;
	}

} // End Namespace NAP

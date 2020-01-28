// Local Includes
#include "shader.h"
#include "material.h"

// External Includes
#include <utility/fileutils.h>
#include <nap/logger.h>
#include "renderservice.h"

RTTI_BEGIN_CLASS(nap::Shader)
	RTTI_PROPERTY_FILELINK("mVertShader", &nap::Shader::mVertPath, nap::rtti::EPropertyMetaData::Required, nap::rtti::EPropertyFileType::VertShader)
	RTTI_PROPERTY_FILELINK("mFragShader", &nap::Shader::mFragPath, nap::rtti::EPropertyMetaData::Required, nap::rtti::EPropertyFileType::FragShader)
	RTTI_PROPERTY("OutputFormat", &nap::Shader::mOutputFormat, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
	Shader::Shader() :
		mRenderer(nullptr)
	{

	}

	Shader::Shader(RenderService& renderService) :
		mRenderer(&renderService.getRenderer())
	{

	}

	// Store path and create display names
	bool Shader::init(utility::ErrorState& errorState)
	{
		if (!errorState.check(!mVertPath.empty(), "Vertex shader path not set"))
			return false;

		if (!errorState.check(!mFragPath.empty(), "Fragment shader path not set"))
			return false;

		// Set display name
		mDisplayName = utility::getFileNameWithoutExtension(mVertPath);

		mShader = std::make_unique<opengl::Shader>();

		// Initialize the shader
		if (!mShader->init(mRenderer->getDevice(), mVertPath, mFragPath, errorState))
			return false;

		return true;
	}


	// Returns the associated opengl shader
	const opengl::Shader& Shader::getShader() const
	{
		assert(mShader != nullptr);
		return *mShader;
	}

}

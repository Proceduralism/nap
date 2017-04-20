// Local Includes
#include "imageresource.h"

// External Includes
#include <nap/logger.h>
#include <nap/fileutils.h>

RTTI_DEFINE_BASE(nap::TextureResource)

RTTI_BEGIN_CLASS(nap::ImageResource)
	RTTI_PROPERTY("mImagePath", &nap::ImageResource::mImagePath)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(opengl::Texture2DSettings)
	RTTI_PROPERTY("mLevel",				&opengl::Texture2DSettings::level)
	RTTI_PROPERTY("mInternalFormat",	&opengl::Texture2DSettings::internalFormat)
	RTTI_PROPERTY("mWidth",				&opengl::Texture2DSettings::width)
	RTTI_PROPERTY("mHeight",			&opengl::Texture2DSettings::height)
	RTTI_PROPERTY("mFormat",			&opengl::Texture2DSettings::format)
	RTTI_PROPERTY("mType",				&opengl::Texture2DSettings::type)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::MemoryTextureResource2D)
	RTTI_PROPERTY("mSettings",			&nap::MemoryTextureResource2D::mSettings)
RTTI_END_CLASS

RTTI_DEFINE(nap::ImageResource)

namespace nap
{
	// Initializes 2D texture. Additionally a custom display name can be provided.
	bool MemoryTextureResource2D::init(InitResult& initResult)
	{
		mTexture.init();

		mTexture.allocate(mSettings);

		return true;
	}

	// Returns 2D texture object
	const opengl::BaseTexture& MemoryTextureResource2D::getTexture() const
	{
		return mTexture;
	}

	// Constructor
	ImageResource::ImageResource(const std::string& imgPath)
	{
		//mImagePath = imgPath;
		mDisplayName = getFileNameWithoutExtension(imgPath);
		assert(mDisplayName != "");
	}


	// Load image if required and extract texture
	const opengl::BaseTexture& ImageResource::getTexture() const
	{
		return getImage().getTexture();
	}


	const std::string ImageResource::getDisplayName() const
	{
		return mDisplayName;
	}

	bool ImageResource::init(InitResult& initResult)
	{
		if (!initResult.check(!mImagePath.empty(), "Imagepath not set"))
			return false;

		if (!initResult.check(mImage.load(mImagePath), "Unable to load image from file"))
			return false;

		return true;
	}


	const opengl::Image& ImageResource::getImage() const
	{
		return mImage;
	}
	
	// Non const getter, following:
	opengl::BaseTexture& TextureResource::getTexture()
	{
		return const_cast<opengl::BaseTexture&>(static_cast<const TextureResource&>(*this).getTexture());
	}
	
}

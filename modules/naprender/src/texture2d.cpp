#include "texture2d.h"
#include "bitmap.h"
#include "renderservice.h"
#include "nap/core.h"
#include "copyimagedata.h"

RTTI_BEGIN_ENUM(nap::EFilterMode)
	RTTI_ENUM_VALUE(nap::EFilterMode::Nearest, "Nearest"),
	RTTI_ENUM_VALUE(nap::EFilterMode::Linear, "Linear"),
	RTTI_ENUM_VALUE(nap::EFilterMode::NearestMipmapNearest, "NearestMipmapNearest"),
	RTTI_ENUM_VALUE(nap::EFilterMode::LinearMipmapNearest, "LinearMipmapNearest"),
	RTTI_ENUM_VALUE(nap::EFilterMode::NearestMipmapLinear, "NearestMipmapLinear"),
	RTTI_ENUM_VALUE(nap::EFilterMode::LinearMipmapLinear, "LinearMipmapLinear")
RTTI_END_ENUM

RTTI_BEGIN_ENUM(nap::EWrapMode)
	RTTI_ENUM_VALUE(nap::EWrapMode::Repeat,			"Repeat"),
	RTTI_ENUM_VALUE(nap::EWrapMode::MirroredRepeat, "MirroredRepeat"),
	RTTI_ENUM_VALUE(nap::EWrapMode::ClampToEdge,	"ClampToEdge"),
	RTTI_ENUM_VALUE(nap::EWrapMode::ClampToBorder,	"ClampToBorder")
RTTI_END_ENUM

RTTI_BEGIN_CLASS(nap::TextureParameters)
	RTTI_PROPERTY("MinFilter",			&nap::TextureParameters::mMinFilter,		nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("MaxFilter",			&nap::TextureParameters::mMaxFilter,		nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("WrapVertical",		&nap::TextureParameters::mWrapVertical,		nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("WrapHorizontal",		&nap::TextureParameters::mWrapHorizontal,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("MaxLodLevel",		&nap::TextureParameters::mMaxLodLevel,		nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::Texture2D)
	RTTI_CONSTRUCTOR(nap::Core&)
	RTTI_PROPERTY("Parameters", 	&nap::Texture2D::mParameters,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Usage", 			&nap::Texture2D::mUsage,		nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

/**
* openglFilterMap
*
* Maps Filter modes to supported GL formats
*/
using OpenglFilterMap = std::unordered_map<nap::EFilterMode, GLint>;
static const OpenglFilterMap openglFilterMap =
{
	{ nap::EFilterMode::Nearest,					GL_NEAREST},
	{ nap::EFilterMode::Linear,						GL_LINEAR },
	{ nap::EFilterMode::NearestMipmapNearest,		GL_NEAREST_MIPMAP_NEAREST },
	{ nap::EFilterMode::LinearMipmapNearest,		GL_LINEAR_MIPMAP_NEAREST },
	{ nap::EFilterMode::NearestMipmapLinear,		GL_NEAREST_MIPMAP_LINEAR },
	{ nap::EFilterMode::LinearMipmapLinear,			GL_LINEAR_MIPMAP_LINEAR}
};


/**
 *	openglWrapMap
 *
 * Maps Wrap modes to supported GL formats
 */
using OpenglWrapMap = std::unordered_map<nap::EWrapMode, GLint>;
static const OpenglWrapMap openglWrapMap =
{	 
	{ nap::EWrapMode::Repeat,						GL_REPEAT },
	{ nap::EWrapMode::MirroredRepeat,				GL_MIRRORED_REPEAT },
	{ nap::EWrapMode::ClampToEdge,					GL_CLAMP_TO_EDGE },
	{ nap::EWrapMode::ClampToBorder,				GL_CLAMP_TO_BORDER }
};


/**
 *	@return the opengl filter based on @filter
 */
static GLint getGLFilterMode(nap::EFilterMode filter)
{
	auto it = openglFilterMap.find(filter);
	assert(it != openglFilterMap.end());
	return it->second;
}


/**
 *	@return the opengl wrap mode based on @wrapmode
 */
static GLint getGLWrapMode(nap::EWrapMode wrapmode)
{
	auto it = openglWrapMap.find(wrapmode);
	assert(it != openglWrapMap.end());
	return it->second;
}


static void convertTextureParameters(const nap::TextureParameters& input, opengl::TextureParameters& output)
{
	output.minFilter	=	getGLFilterMode(input.mMinFilter);
	output.maxFilter	=	getGLFilterMode(input.mMaxFilter);
	output.wrapVertical =	getGLWrapMode(input.mWrapVertical);
	output.wrapHorizontal = getGLWrapMode(input.mWrapHorizontal);
	output.maxLodLevel =	input.mMaxLodLevel;
}


//////////////////////////////////////////////////////////////////////////

namespace nap
{
	namespace 
	{
		bool createImage(VmaAllocator vmaAllocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, VmaAllocation& allocation, VmaAllocationInfo& allocationInfo, utility::ErrorState& errorState)
		{
			VkImageCreateInfo image_info = {};
			image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_info.imageType = VK_IMAGE_TYPE_2D;
			image_info.extent.width = width;
			image_info.extent.height = height;
			image_info.extent.depth = 1;
			image_info.mipLevels = 1;
			image_info.arrayLayers = 1;
			image_info.format = format;
			image_info.tiling = tiling;
			image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_info.usage = usage;
			image_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo alloc_info = {};
			alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			alloc_info.flags = 0;

			VkResult result = vmaCreateImage(vmaAllocator, &image_info, &alloc_info, &image, &allocation, &allocationInfo);
			if (!errorState.check(result == VK_SUCCESS, "Failed to create image for texture"))
				return false;

			return true;
		}


		VkFormat getTextureFormat(ESurfaceChannels channels, ESurfaceDataType dataType, EColorSpace colorSpace)
		{
			switch (channels)
			{
			case ESurfaceChannels::R:
				{
					switch (dataType)
					{
					case nap::ESurfaceDataType::BYTE:
						return colorSpace == EColorSpace::Linear ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_SRGB;
					case nap::ESurfaceDataType::FLOAT:
						return VK_FORMAT_R32_SFLOAT;
					case nap::ESurfaceDataType::USHORT:
						return VK_FORMAT_R16_UNORM;
					}
					break;
				}
			case ESurfaceChannels::RGBA:
				{
					switch (dataType)
					{
					case nap::ESurfaceDataType::BYTE:
						return colorSpace == EColorSpace::Linear ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB;
					case nap::ESurfaceDataType::FLOAT:
						return VK_FORMAT_R32G32B32A32_SFLOAT;
					case nap::ESurfaceDataType::USHORT:
						return VK_FORMAT_R16G16B16A16_UNORM;
					}
					break;
				}
			case ESurfaceChannels::BGRA:
				{
					switch (dataType)
					{
					case nap::ESurfaceDataType::BYTE:
						return colorSpace == EColorSpace::Linear ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_B8G8R8A8_SRGB;
					case nap::ESurfaceDataType::FLOAT:
						return VK_FORMAT_UNDEFINED;
					case nap::ESurfaceDataType::USHORT:
						return VK_FORMAT_UNDEFINED;
					}
					break;
				}
			}
			return VK_FORMAT_UNDEFINED;
		}

		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) 
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else 
			{
				throw std::invalid_argument("unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width,
				height,
				1
			};

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		bool createImageView(VkDevice device, VkImage image, VkFormat format, VkImageView& imageView, utility::ErrorState& errorState) 
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (!errorState.check(vkCreateImageView(device, &viewInfo, nullptr, &imageView) == VK_SUCCESS, "Failed to create texture image view"))
				return false;

			return true;
		}
	}	

	//////////////////////////////////////////////////////////////////////////

	Texture2D::Texture2D(Core& core) :
		mRenderService(core.getService<RenderService>())
	{
	}

	bool Texture2D::init(const SurfaceDescriptor& descriptor, bool compressed, utility::ErrorState& errorState)
	{
		VkDevice device = mRenderService->getDevice();
		VkPhysicalDevice physicalDevice = mRenderService->getPhysicalDevice();

		mImageSizeInBytes = descriptor.getSizeInBytes();

		VmaAllocator vulkan_allocator = mRenderService->getVulkanAllocator();

		// Here we create staging buffers. Client data is copied into staging buffers. The staging buffers are then used as a source to update
		// the GPU texture. The updating of the GPU textures is done on the command buffer. The updating of the staging buffers can be done
		// at any time. However, as the staging buffers serve as a source for updating the GPU buffers, they are part of the command buffer.
		// 
		// We can only safely update the staging buffer if we know it isn't used anymore. We generally make enough resources for each frame
		// that can be in flight. Once we've passed RenderService::beginRendering, we know that the resources for the current frame are 
		// not in use anymore. If we would use this strategy, we could only safely use a staging buffer during rendering. To be more 
		// specific, we could only use the staging buffer during rendering, but before the render pass was set (as this is a Vulkan
		// requirement for buffer transfers). This is very inconvenient for texture updating, as we'd ideally like to update texture contents
		// at any point in the frame. We also don't want to make an extra copy of the texture that would be used during rendering. To solve 
		// this problem, we use one additional staging buffer. This guarantees that there's always a single staging buffer free at any point 
		// in the frame. So the amount of staging buffers is:  'maxFramesInFlight' + 1. Updating the staging buffer multiple times within a 
		// frame will just overwrite the same staging buffer.
		//
		// A final note: this system is built to be able to handle changing the texture every frame. But if the texture is changed less frequently,
		// or never, that works as well. When update is called, the RenderService is notified of the change, and during rendering, the upload is
		// called, which moves the index one place ahead. 
		mStagingBuffers.resize(mUsage == opengl::ETextureUsage::DynamicWrite ? 3 : 1);
		for (int index = 0; index < mStagingBuffers.size(); ++index)
		{
			StagingBuffer& imageBuffer = mStagingBuffers[index];

			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = mImageSizeInBytes;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			allocInfo.flags = 0;

			if (!errorState.check(vmaCreateBuffer(vulkan_allocator, &bufferInfo, &allocInfo, &imageBuffer.mStagingBuffer, &imageBuffer.mStagingBufferAllocation, &imageBuffer.mStagingBufferAllocationInfo) == VK_SUCCESS, "Could not allocate buffer for texture"))
				return false;
		}

		// We create images and imageviews for the amount of frame in flight
		mImageData.resize(mUsage == opengl::ETextureUsage::DynamicWrite ? 2 : 1);
		for (int index = 0; index < mImageData.size(); ++index)
		{
			ImageData& imageData = mImageData[index];
			VkFormat texture_format = getTextureFormat(descriptor.mChannels, descriptor.mDataType, descriptor.mColorSpace);
			if (!errorState.check(texture_format != VK_FORMAT_UNDEFINED, "Unsupported texture format"))
				return false;

			if (!createImage(vulkan_allocator, descriptor.mWidth, descriptor.mHeight, texture_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageData.mTextureImage, imageData.mTextureAllocation, imageData.mTextureAllocationInfo, errorState))
				return false;

			if (!createImageView(device, imageData.mTextureImage, texture_format, imageData.mTextureView, errorState))
				return false;
		}

		mCurrentImageIndex = 0;
		mCurrentStagingBufferIndex = 0;
		mDescriptor = descriptor;

		return true;
	}


	const glm::vec2 Texture2D::getSize() const
	{
		return glm::vec2(getWidth(), getHeight());
	}


	int Texture2D::getWidth() const
	{
		return mDescriptor.mWidth;
	}


	int Texture2D::getHeight() const
	{
		return mDescriptor.mHeight;
	}

	void Texture2D::upload(VkCommandBuffer commandBuffer)
	{
		assert(mCurrentStagingBufferIndex != -1);
		StagingBuffer& buffer = mStagingBuffers[mCurrentStagingBufferIndex];
		mCurrentStagingBufferIndex = (mCurrentStagingBufferIndex + 1) % mStagingBuffers.size();
		
		// We must pre-increment the image index, because mCurrentImageIndex is used here and later in getImageView.
		// We cannot bump the index between upload and getImageView.
		mCurrentImageIndex = (mCurrentImageIndex + 1) % mImageData.size();
		ImageData& imageData = mImageData[mCurrentImageIndex];

		transitionImageLayout(commandBuffer, imageData.mTextureImage, imageData.mCurrentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(commandBuffer, buffer.mStagingBuffer, imageData.mTextureImage, mDescriptor.mWidth, mDescriptor.mHeight);
		transitionImageLayout(commandBuffer, imageData.mTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// We store the last image layout, which is used as input for a subsequent upload
		imageData.mCurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
		// Notify listeners such as Samplers, so that they can use another ImageView for this texture.
		changed(*this);
	}

	void Texture2D::update(const void* data, const SurfaceDescriptor& surfaceDescriptor)
	{
		update(data, surfaceDescriptor.getWidth(), surfaceDescriptor.getHeight(), surfaceDescriptor.getPitch(), surfaceDescriptor.getChannels());
	}

	void Texture2D::update(const void* data, int width, int height, int pitch, ESurfaceChannels channels)
	{
		// We can only upload when the texture usage is dynamic, OR this is the first upload for a static texture
		assert(mUsage == opengl::ETextureUsage::DynamicWrite || mImageData[0].mCurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED);
		assert(mDescriptor.mWidth == width && mDescriptor.mHeight == height);

		// We use a staging buffer that is guaranteed to be free
		assert(mCurrentStagingBufferIndex != -1);
		StagingBuffer& buffer = mStagingBuffers[mCurrentStagingBufferIndex];

		// Update the staging buffer using the Bitmap contents
		VmaAllocator vulkan_allocator = mRenderService->getVulkanAllocator();

		void* mapped_memory;
		VkResult result = vmaMapMemory(vulkan_allocator, buffer.mStagingBufferAllocation, &mapped_memory);
		assert(result == VK_SUCCESS);

		copyImageData((const uint8_t*)data, pitch, channels, (uint8_t*)mapped_memory, mDescriptor.getPitch(), mDescriptor.mChannels, mDescriptor.mWidth, mDescriptor.mHeight);

		//memcpy(mapped_memory, data, mImageSizeInBytes);
		vmaUnmapMemory(vulkan_allocator, buffer.mStagingBufferAllocation);

		// Notify the RenderService that it should upload the texture contents during rendering
		mRenderService->requestTextureUpdate(*this);
	}


	void Texture2D::getData(Bitmap& bitmap)
	{
// 		if (bitmap.empty())
// 			bitmap.initFromTexture(mTexture.getSettings());
// 
// 		mTexture.getData(bitmap.getData(), bitmap.getSizeInBytes());
	}


	nap::uint Texture2D::getHandle() const
	{
		return getTexture().getTextureId();
	}


	VkImageView Texture2D::getImageView() const
	 { 
		assert(mCurrentImageIndex != -1);
		return mImageData[mCurrentImageIndex].mTextureView; 
	}


	void Texture2D::startGetData()
	{
		//getTexture().asyncStartGetData();
	}


	void Texture2D::endGetData(Bitmap& bitmap)
	{
// 		if (bitmap.empty())
// 			bitmap.initFromTexture(mTexture.getSettings());
// 
// 		mTexture.getData(bitmap.getData(), bitmap.getSizeInBytes());
	}
}

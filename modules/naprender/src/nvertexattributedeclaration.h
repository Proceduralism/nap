#pragma once

// External Includes
#include <string>
#include <unordered_map>
#include "vulkan/vulkan_core.h"
#include "rtti/typeinfo.h"

namespace nap
{
	/**
	* Represents an Vulkan vertex shader attribute
	*/
	class VertexAttributeDeclaration
	{
	public:
		// Constructor
		VertexAttributeDeclaration(const std::string& name, int location, VkFormat format);
		VertexAttributeDeclaration() = delete;

		std::string		mName;							///< Name of the shader attribute
		int				mLocation;
		VkFormat		mFormat;
	};

	using VertexAttributeDeclarations = std::unordered_map<std::string, std::unique_ptr<VertexAttributeDeclaration>>;
}

//////////////////////////////////////////////////////////////////////////
// Hash
//////////////////////////////////////////////////////////////////////////
namespace std
{
	template<>
	struct hash<nap::VertexAttributeDeclaration>
	{
		size_t operator()(const nap::VertexAttributeDeclaration &k) const
		{
			return hash<std::string>()(k.mName);
		}
	};
}

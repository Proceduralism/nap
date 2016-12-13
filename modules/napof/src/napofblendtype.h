#pragma once

// GL includes
#include <nap.h>
#include <unordered_map>

namespace nap
{
	// OF derived GL Blend Modes
	enum class OFBlendType
	{
		SRC_ALPHA ,
		ONE_MINUS_SRC_ALPHA,
		ONE,
		ONE_MINUS_DST_COLOR,
		SRC_COLOR,
		DST_COLOR,
	};

	// Defines a blend mode together with a name
	struct OFBlendDefinition
	{
		OFBlendDefinition(nap::uint inGLid, const std::string& inName) :
			mGLid(inGLid),
			mBindingName(inName)	{ }

		nap::uint		mGLid;
		std::string		mBindingName;
	};

	// All the blend modes as names (in order as defined above)
	using OFBlendModeMap = std::unordered_map<OFBlendType, OFBlendDefinition>;

	// Map containing all available blend modes
	extern const nap::OFBlendModeMap gBlendTypes;

	// Type converters
	bool convert_string_to_ofblendtype(const std::string& inValue, OFBlendType& outValue);
	bool convert_ofblendtype_to_string(const OFBlendType& inValue, std::string& outValue);
}

RTTI_DECLARE_DATA(nap::OFBlendType)

namespace std
{
	template<>
	struct hash<nap::OFBlendType> {
		size_t operator()(const nap::OFBlendType &k) const {
			return hash<int>()((int)k);
		}
	};
}





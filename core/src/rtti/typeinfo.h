#pragma once

#include <rttr/type>
#include <rttr/registration>

namespace RTTI
{
	using TypeInfo = rttr::type;
	using Property = rttr::property;
	using Variant = rttr::variant;
	using Instance = rttr::instance;
	using VariantArray = rttr::variant_array_view;
	using VariantMap = rttr::variant_associative_view;

	enum class EPropertyMetaData
	{
		Required,
		FileLink
	};
}

// Macros
#define RTTI_OF(Type) rttr::type::get<Type>()

// Defines an RTTI object with create function
#define CONCAT_UNIQUE_NAMESPACE(x, y)				namespace x##y
#define UNIQUE_REGISTRATION_NAMESPACE(id)			CONCAT_UNIQUE_NAMESPACE(__rtti_registration_, id)

#define RTTI_BEGIN_BASE_CLASS(Type)							\
	UNIQUE_REGISTRATION_NAMESPACE(__COUNTER__)			\
	{													\
		RTTR_REGISTRATION								\
		{												\
			using namespace rttr;						\
			registration::class_<Type>(#Type)			\


#define RTTI_PROPERTY(Name, Member)						\
						  .property(Name, Member)

#define RTTI_PROPERTY_REQUIRED(Name, Member)			\
						  .property(Name, Member)(metadata(RTTI::EPropertyMetaData::Required, true))

#define RTTI_PROPERTY_FILE_LINK(Name, Member)			\
						  .property(Name, Member)(metadata(RTTI::EPropertyMetaData::Required, true), metadata(RTTI::EPropertyMetaData::FileLink, true))

#define RTTI_END_CLASS									\
		;												\
		}												\
	}													\

#define RTTI_BEGIN_CLASS(Type)							\
	RTTI_BEGIN_BASE_CLASS(Type)							\
	.constructor<>()(policy::ctor::as_raw_ptr)

#define RTTI_DEFINE(Type)							\
	RTTI_BEGIN_CLASS(Type)							\
	RTTI_END_CLASS

#define RTTI_DEFINE_BASE(Type)						\
	RTTI_BEGIN_BASE_CLASS(Type)							\
	RTTI_END_CLASS

#define RTTI_ENABLE(...) \
	RTTR_ENABLE(__VA_ARGS__) \
	RTTR_REGISTRATION_FRIEND

#define RTTI_ENABLE_DERIVED_FROM(...) \
	RTTI_ENABLE(__VA_ARGS__)
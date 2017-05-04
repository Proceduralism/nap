#include "jsonwriter.h"

namespace nap
{
	JSONWriter::JSONWriter() :
		mWriter(mStringBuffer)
	{
	}


	std::string JSONWriter::GetJSON()
	{
		return mStringBuffer.GetString();
	}


	bool JSONWriter::start()
	{
		return mWriter.StartObject();
	}


	bool JSONWriter::finish()
	{
		if (!mWriter.EndObject())
			return false;

		return true;
	}


	bool JSONWriter::startRootObject(const RTTI::TypeInfo& type)
	{
		if (!mWriter.String(type.get_name().data()))
			return false;

		return mWriter.StartObject();
	}


	bool JSONWriter::finishRootObject()
	{
		return mWriter.EndObject();
	}


	bool JSONWriter::startCompound(const RTTI::TypeInfo& type)
	{
		return mWriter.StartObject();
	}


	bool JSONWriter::finishCompound()
	{
		return mWriter.EndObject();
	}


	bool JSONWriter::startArray(int length)
	{
		return mWriter.StartArray();
	}


	bool JSONWriter::finishArray()
	{
		return mWriter.EndArray();
	}


	bool JSONWriter::writeProperty(const std::string& propertyName)
	{
		return mWriter.String(propertyName);
	}


	bool JSONWriter::writePointer(const std::string& pointeeID)
	{
		//std::string pointee_id = "<" + pointeeID + ">";
		return mWriter.String(pointeeID);
	}


	bool JSONWriter::writePrimitive(const RTTI::TypeInfo& type, const RTTI::Variant& value)
	{
		if (type.is_arithmetic())
		{
			if (type == RTTI::TypeInfo::get<bool>())
				return mWriter.Bool(value.to_bool());
			else if (type == RTTI::TypeInfo::get<char>())
				return mWriter.Bool(value.to_bool());
			else if (type == RTTI::TypeInfo::get<int8_t>())
				return mWriter.Int(value.to_int8());
			else if (type == RTTI::TypeInfo::get<int16_t>())
				return mWriter.Int(value.to_int16());
			else if (type == RTTI::TypeInfo::get<int32_t>())
				return mWriter.Int(value.to_int32());
			else if (type == RTTI::TypeInfo::get<int64_t>())
				return mWriter.Int64(value.to_int64());
			else if (type == RTTI::TypeInfo::get<uint8_t>())
				return mWriter.Uint(value.to_uint8());
			else if (type == RTTI::TypeInfo::get<uint16_t>())
				return mWriter.Uint(value.to_uint16());
			else if (type == RTTI::TypeInfo::get<uint32_t>())
				return mWriter.Uint(value.to_uint32());
			else if (type == RTTI::TypeInfo::get<uint64_t>())
				return mWriter.Uint64(value.to_uint64());
			else if (type == RTTI::TypeInfo::get<float>())
				return mWriter.Double(value.to_double());
			else if (type == RTTI::TypeInfo::get<double>())
				return mWriter.Double(value.to_double());

			return false;
		}
		else if (type.is_enumeration())
		{
			// Try to convert the enum to string first
			bool conversion_succeeded = false;
			auto result = value.to_string(&conversion_succeeded);
			if (conversion_succeeded)
			{
				return mWriter.String(value.to_string());
			}
			else
			{
				// Failed to convert enum to string; try to write as int
				conversion_succeeded = false;
				auto value_int = value.to_uint64(&conversion_succeeded);
				if (conversion_succeeded)
					return mWriter.Uint64(value_int);
			}

			return false;
		}
		else if (type == RTTI::TypeInfo::get<std::string>())
		{
			return mWriter.String(value.to_string());
		}

		return false;
	}
}

#include "timestamp.h"
#include <iostream>

// nap::timestamp run time class definition 
RTTI_BEGIN_STRUCT(nap::TimeStamp)
	RTTI_PROPERTY("Time", &nap::TimeStamp::mTimeStamp, nap::rtti::EPropertyMetaData::Default)
	RTTI_VALUE_CONSTRUCTOR(const nap::utility::SystemTimeStamp&)
RTTI_END_STRUCT

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	TimeStamp::TimeStamp(utility::SystemTimeStamp systemTime) 
	{
		fromSystemTime(systemTime);
	}


	void TimeStamp::fromSystemTime(const utility::SystemTimeStamp& systemTime)
	{
		// construct time_point based on system_clock, but with the precision of milliseconds instead of whatever precision your system_clock has.
		auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(systemTime);
			
		// store epoch is in milliseconds as long
		mTimeStamp = ms.time_since_epoch().count();
	}


	nap::utility::SystemTimeStamp TimeStamp::toSystemTime() const
	{
		std::chrono::milliseconds dur(mTimeStamp);
		return std::chrono::time_point<std::chrono::system_clock>(dur);
	}
}
#include "apicomponent.h"

// External Includes
#include <entity.h>
#include <nap/core.h>
#include <entity.h>

// nap::apicomponent run time class definition 
RTTI_BEGIN_CLASS(nap::APIComponent)
	RTTI_PROPERTY("Deferred",	&nap::APIComponent::mDeferred,	 nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("CallFilter", &nap::APIComponent::mCallFilter, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

// nap::apicomponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::APIComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{

	APIComponentInstance::~APIComponentInstance()
	{
		if (mAPIService != nullptr)
		{
			mAPIService->removeAPIComponent(*this);
		}
	}


	bool APIComponentInstance::init(utility::ErrorState& errorState)
	{
		// Store api service and register
		mAPIService = getEntityInstance()->getCore()->getService<nap::APIService>();
		assert(mAPIService != nullptr);
		mAPIService->registerAPIComponent(*this);

		// Copy over list of accepted calls
		std::vector<std::string>& calls = getComponent<APIComponent>()->mCallFilter;
		for (const auto& call : calls)
			mCallFilter.emplace(call);

		// Store if we need to execute deferred
		mDeferred = getComponent<APIComponent>()->mDeferred;
		
		return true;
	}


	bool APIComponentInstance::accepts(const std::string& action) const
	{
		const auto it = mCallFilter.find(action);
		return it != mCallFilter.end();
	}


	void APIComponentInstance::update(double deltaTime)
	{
		// Copy api events thread safe
		std::queue<APICallEventPtr> out_events;
		{
			std::lock_guard<std::mutex> lock(mCallMutex);
			out_events.swap(mCalls);
			std::queue<APICallEventPtr> empty_queue;
			mCalls.swap(empty_queue);
		}

		// Process api events
		while (!out_events.empty())
		{
			APICallEvent* call_event = out_events.front().get();
			// TODO: call
			out_events.pop();
		}
	}


	bool APIComponentInstance::call(APICallEventPtr apiEvent, nap::utility::ErrorState& error)
	{
		// Make sure the call is accepted
		assert(accepts(apiEvent->getID()));

		// TODO: Check signature

		// If we defer the call, add it and run over it later
		if (mDeferred)
		{
			std::lock_guard<std::mutex> lock_guard(mCallMutex);
			mCalls.push(std::move(apiEvent));
			return true;
		}

		// TODO: Execute
		APICallEventPtr api_event(std::move(apiEvent));

		return true;
	}
}
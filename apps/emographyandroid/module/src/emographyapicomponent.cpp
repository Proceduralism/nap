#include "emographyapicomponent.h"

// External Includes
#include <entity.h>
#include <nap/logger.h>

// nap::apihandlecomponent run time class definition 
RTTI_BEGIN_CLASS(nap::EmographyAPIComponent)
	// Put additional properties here
RTTI_END_CLASS

// nap::apihandlecomponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::EmographyAPIComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	void EmographyAPIComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.emplace_back(RTTI_OF(nap::APIComponent));
	}


	bool EmographyAPIComponentInstance::init(utility::ErrorState& errorState)
	{
		 mComponentInstance = getEntityInstance()->findComponent<APIComponentInstance>();
		 if (!errorState.check(mComponentInstance != nullptr, "%s: unable to find required API component", this->mID.c_str()))
			 return false;

		 const nap::APISignature* api_signature = mComponentInstance->findSignature("updateView");
		 if (!errorState.check(api_signature != nullptr, "%s: unable to find method with signature: %s", this->mID.c_str(), "updateView"))
			 return false;
		 mComponentInstance->registerCallback(*api_signature, mUpateViewSlot);

		return true;
	}


	void EmographyAPIComponentInstance::update(double deltaTime)
	{

	}


	void EmographyAPIComponentInstance::updateView(const nap::APIEvent& apiEvent)
	{
		nap::Logger::info("received: %s", apiEvent.getID().c_str());
		int64_t start_time = apiEvent[0].asLong();
		int64_t end_time = apiEvent[1].asLong();
		int samples = apiEvent[2].asInt();
		const std::vector<float>* parray = apiEvent[3].asArray<float>();
	}
}
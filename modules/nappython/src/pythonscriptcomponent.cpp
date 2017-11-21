#include <pythonscriptcomponent.h>
#include <nap/entity.h>
#include <rtti/pythonmodule.h>
#include <utility/fileutils.h>
#include "nap/core.h"
#include "pythonscriptservice.h"
#include "nap/logger.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PythonScriptComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::PythonScriptComponent)
	RTTI_PROPERTY("Path", &nap::PythonScriptComponent::mPath, nap::rtti::EPropertyMetaData::Required | nap::rtti::EPropertyMetaData::FileLink)
RTTI_END_CLASS

namespace nap
{
	void PythonScriptComponentInstance::update(double deltaTime)
	{
        call("update", getEntityInstance(), getEntityInstance()->getCore()->getElapsedTime(), deltaTime);
	}

    
	bool PythonScriptComponentInstance::init(utility::ErrorState& errorState)
	{
		PythonScriptComponent* script_component = getComponent<PythonScriptComponent>();

		PythonScriptService* script_service = getEntityInstance()->getCore()->getService<PythonScriptService>();
		assert(script_service != nullptr);
		if (!errorState.check(script_service->TryLoad(script_component->mPath, mScript, errorState), "Failed to load %s", script_component->mPath.c_str()))
			return false;
        
        call("init", getEntityInstance());
		
		return true;
	}
}

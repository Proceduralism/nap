#include <pythonscriptcomponent.h>
#include <nap/entity.h>
#include <rtti/pythonmodule.h>
#include "nap/fileutils.h"
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
		try
		{
			mScript.attr("update")(getEntity(), getEntity()->getCore()->getElapsedTime(), deltaTime);
		}
		catch (const pybind11::error_already_set& err)
		{
			nap::Logger::info("Runtime python error while executing %s: %s", getResource<PythonScriptComponent>()->mPath.c_str(), err.what());
		}
	}

	bool PythonScriptComponentInstance::init(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		PythonScriptComponent* script_component = getResource<PythonScriptComponent>();

		PythonScriptService* script_service = getEntity()->getCore()->getOrCreateService<PythonScriptService>();
		if (!errorState.check(script_service->TryLoad(script_component->mPath, mScript, errorState), "Failed to load %s", script_component->mPath.c_str()))
			return false;
		
		return true;
	}
}
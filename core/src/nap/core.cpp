// Local Includes
#include "core.h"
#include "resourcemanager.h"
#include "logger.h"
#include "serviceobjectgraphitem.h"
#include "objectgraph.h"
#include "fileutils.h"

// External Includes
#include <rtti/pythonmodule.h>
#include <iostream>

using namespace std;

RTTI_BEGIN_CLASS(nap::Core)
	RTTI_FUNCTION("getService", (nap::Service* (nap::Core::*)(const std::string&))&nap::Core::getService)
	RTTI_FUNCTION("getResourceManager", &nap::Core::getResourceManager)
RTTI_END_CLASS


namespace nap
{
	/**
	@brief Constructor

	Creates a default entity as the root
	**/
	Core::Core()
	{
		// Initialize timer
		mTimer.reset();

		// Add resource manager service and listen to file changes
		mResourceManager = std::make_unique<ResourceManager>(*this);
		mResourceManager->mFileLoadedSignal.connect(mFileLoadedSlot);
	}

    
	Core::~Core()
	{
		mResourceManager->mFileLoadedSignal.disconnect(mFileLoadedSlot);

		// In order to ensure a correct order of destruction we want our entities, components, etc. to be deleted before other services are deleted.
		// Because entities and components are managed and owned by the resource manager we explicitly delete this first.
		// Erase it
		mResourceManager.reset();
	}
	
    
	bool Core::initializeEngine(utility::ErrorState& error)
	{ 
		// Load all modules
		// TODO: This should be correctly resolved, ie: the dll's should always
		// be in the executable directory
#ifdef _WIN32
		mModuleManager.loadModules(nap::getExecutableDir());
#else
		std::string exe_dir = "../../lib/" + nap::getFileName(nap::getExecutableDir());
		mModuleManager.loadModules(exe_dir);
#endif // _WIN32

		// Create the various services based on their dependencies
		if (!createServices(error))
			return false;

		// Initialize the various services
		if (!initializeServices(error))
			return false;

		// Here we register a callback that is called when the nap python module is imported.
		// We register a 'core' attribute so that we can write nap.core.<function>() in python
		// to access core functionality as a 'global'.
		nap::rtti::PythonModule::get("nap").registerImportCallback([this](pybind11::module& module)
		{
			module.attr("core") = this;
		});

		return true;
	}


	bool Core::initializeServices(utility::ErrorState& errorState)
	{
		std::vector<Service*> objects;
		for (const auto& service : mServices)
		{
			nap::Logger::info("initializing service: %s", service->getTypeName().c_str());
			if (!service->init(errorState))
				return false;
		}
		return true;
	}


	void Core::resourceFileChanged(const std::string& file)
	{
		for (auto& service : mServices)
		{
			service->resourcesLoaded();
		}
	}


	void Core::start()
	{
		mTimer.reset();
	}


	double Core::update(std::function<void(double)>& updateFunction)
	{
		// Get delta time
		double new_time = getElapsedTime();
		mDeltaTime = new_time - mLastTimeStamp;
		mLastTimeStamp = new_time;

		// Perform update call before we check for file changes
		for (auto& service : mServices)
		{
			service->preUpdate(mDeltaTime);
		}

		// Check for file changes
		mResourceManager->checkForFileChanges();

		// Update rest of the services
		for (auto& service : mServices)
		{
			service->update(mDeltaTime);
		}

		// Update the resource manager, ie: all entities and their respective components
		mResourceManager->update(mDeltaTime);

		// Call update function
		updateFunction(mDeltaTime);

		// Update rest of the services
		for (auto& service : mServices)
		{
			service->postUpdate(mDeltaTime);
		}

		return mDeltaTime;
	}


	void Core::shutdown()
	{
		for (auto it = mServices.rbegin(); it != mServices.rend(); it++)
		{
			Service& service = **it;
			nap::Logger::info("shutting down service: %s", service.getTypeName().c_str());
			service.shutdown();
		}
	}


	bool Core::createServices(utility::ErrorState& errorState)
	{
		// First create and add all the services (unsorted)
		std::vector<Service*> services;
		for (auto& service : mModuleManager.mModules)
		{
			if (service.mService == rtti::TypeInfo::empty())
				continue;

			// Create the service
			if (!addService(service.mService, services, errorState))
				return false;
		}

		// Create dependency graph
		ObjectGraph<ServiceObjectGraphItem> graph;

		// Build service dependency graph
		bool success = graph.build(services, [&](Service* service)
		{
			return ServiceObjectGraphItem::create(service, &services);
		}, errorState);

		// Make sure the graph was successfully build
		if (!errorState.check(success, "unable to build service dependency graph"))
			return false;

		// Add services in right order
		for (auto& node : graph.getSortedNodes())
		{
			nap::Service* service = node->mItem.mObject;
			mServices.emplace_back(std::unique_ptr<nap::Service>(service));
		}
		return true;
	}


	// Returns service that matches @type
	Service* Core::getService(const rtti::TypeInfo& type)
	{
		// Find service of type 
		const auto& found_service = std::find_if(mServices.begin(), mServices.end(), [&type](const auto& service)
		{
            return service->get_type().is_derived_from(type);
		});

		// Check if found
		return found_service == mServices.end() ? nullptr : (*found_service).get();
	}


	nap::Service* Core::getService(const std::string& type)
	{
		rtti::TypeInfo stype = rtti::TypeInfo::get_by_name(type.c_str());
		return getService(stype);
	}


	// Add a new service
	bool Core::addService(const rtti::TypeInfo& type, std::vector<Service*>& outServices, utility::ErrorState& errorState)
	{
        assert(type.is_valid());
		assert(type.can_create_instance());
		assert(type.is_derived_from<Service>());

		// Check if service doesn't already exist
		const auto& found_service = std::find_if(outServices.begin(), outServices.end(), [&type](const auto& service)
		{
			return service->get_type() == type.get_raw_type();
		});

		bool new_service = found_service == outServices.end();
		if (!errorState.check(new_service, "can't add service of type: %s, service already exists", type.get_name().data()))
			return false;

		// Add service
		Service* service = type.create<Service>();
		service->mCore = this;
		service->registerObjectCreators(mResourceManager->getFactory());

		// Signal creation
		service->created();
		
		// Add
		outServices.emplace_back(service);
		return true;
	}


	// return number of elapsed ticks
	uint32 Core::getTicks() const
	{
		return mTimer.getTicks();
	}


	// Return elapsed time
	double Core::getElapsedTime() const
	{
		return mTimer.getElapsedTime();
	}


	// Returns start time of core module as point in time
	TimePoint Core::getStartTime() const
	{
		return mTimer.getStartTime();
	}
}

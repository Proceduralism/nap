#include "defaultlinkresolver.h"
#include "rttiobject.h"

namespace nap
{
	namespace rtti
	{
		DefaultLinkResolver::DefaultLinkResolver(const OwnedObjectList& objects)
		{
			for (auto& object : objects)
				mObjectsByID.insert({ object->mID, object.get() });
		}


		/**
		 * Resolves the target ID to an object, using the ObjectsByID mapping
		 */
		RTTIObject* DefaultLinkResolver::findTarget(const std::string& targetID)
		{
			ObjectsByIDMap::iterator pos = mObjectsByID.find(targetID);
			if (pos == mObjectsByID.end())
				return nullptr;

			return pos->second;
		}


		bool DefaultLinkResolver::sResolveLinks(const OwnedObjectList& objects, const UnresolvedPointerList& unresolvedPointers, utility::ErrorState& errorState)
		{
			DefaultLinkResolver resolver(objects);
			return resolver.resolveLinks(unresolvedPointers, errorState);
		}
	}
}
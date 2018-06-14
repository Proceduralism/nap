#include "scene.h"
#include "entity.h"
#include "entityobjectgraphitem.h"
#include "entitycreationparameters.h"
#include "entityptr.h"
#include <nap/core.h>
#include <rtti/rttiutilities.h>
#include <nap/objectgraph.h>
#include "sceneservice.h"
#include "transformcomponent.h"

RTTI_BEGIN_CLASS(nap::RootEntity)
	RTTI_PROPERTY("Entity",				&nap::RootEntity::mEntity,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("InstanceProperties", &nap::RootEntity::mInstanceProperties,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::Scene)
	RTTI_FUNCTION("findEntity",			&nap::Scene::findEntity)
	RTTI_PROPERTY("Entities",			&nap::Scene::mEntities,					nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
	using ClonedResourceMap = std::unordered_map<rtti::Object*, std::vector<rtti::Object*>>;
	using ObjectsByTypeMap = std::unordered_map<rtti::TypeInfo, std::vector<rtti::Object*>>;
	using RootEntityInstanceMap = std::unordered_map<std::string, EntityInstance*>;

	/**
	 * This class is only used as a 'grouping' for static functions that are needed for scene instantiation. The reason for this is that during scene instantiation we need to access
	 * private members of several classes. We'd like these functions to be the only ones with access to those members, but adding friend functions is pretty tedious.
	 * By grouping them in a class, we can simply make this class the friend.
	*/
	class SceneInstantiation
	{
	public:
		/**
		 * Helper function to generate a unique ID for an entity or component instance, based on instances already created
		 */
		static std::string sGenerateInstanceID(const std::string& baseID, EntityCreationParameters& entityCreationParams)
		{
			std::string result = baseID;

			int index = 0;
			while (entityCreationParams.mAllInstancesByID.find(result) != entityCreationParams.mAllInstancesByID.end())
				result = utility::stringFormat("%s_%d", baseID.c_str(), index++);

			return result;
		}


		/**
		 * Helper function to generate a unique ID based on a set of previously generated IDs
		 */
		static std::string sGenerateUniqueID(const std::string& baseID, std::unordered_set<std::string>& allIDs)
		{
			std::string result = baseID;

			int index = 0;
			while (allIDs.find(result) != allIDs.end())
				result = utility::stringFormat("%s_%d", baseID.c_str(), index++);

			allIDs.insert(result);

			return result;
		}


		/**
		 * Helper function to get the instance ID of an entity or component
		 */
		static const std::string sGetInstanceID(const std::string& baseID)
		{
			return baseID + "_instance";
		}


		/**
		 * Adds object to the type map. It will add itself and all its base types to the map so that the map contains the entire inheritance hierarchy
		 */
		static void sRecursiveAddToObjectsByType(rtti::Object& object, const rtti::TypeInfo& type, ObjectsByTypeMap& objectsByType)
		{
			objectsByType[type].push_back(&object);
			for (const rtti::TypeInfo& base : type.get_base_classes())
				sRecursiveAddToObjectsByType(object, base, objectsByType);
		}


		/**
		 * Init all components in the object graph in the correct order
		 */
		static bool sInitComponents(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
		{
			std::vector<EntityObjectGraph::Node*> sorted_nodes = entityCreationParams.mObjectGraph->getSortedNodes();

			for (EntityObjectGraph::Node* node : sorted_nodes)
			{
				Component* component = rtti_cast<Component>(node->mItem.mObject);
				if (component == nullptr)
					continue;

				auto pos = entityCreationParams.mComponentInstanceMap.find(component);

				// In case we have instance properties for an object, it is possible that the original resource does 
				// not have an associated instance. We skip these objects.
				if (pos == entityCreationParams.mComponentInstanceMap.end())
					continue;

				for (ComponentInstance* component_instance : pos->second)
					if (!component_instance->init(errorState))
						return false;
			}

			return true;
		}


		/**
		 * Searches for a cloned component with path @componentResourcePath
		 */
		static Component* sFindClonedComponent(const ComponentResourcePath& componentResourcePath, const ClonedComponentResourceList* clonedComponents)
		{
			if (clonedComponents == nullptr)
				return nullptr;

			for (const ClonedComponentResource& clonedComponent : *clonedComponents)
				if (clonedComponent.mPath == componentResourcePath)
					return clonedComponent.mResource.get();

			return nullptr;
		}

		/**
		* Helper to find the target entity that a EntityInstance or ComponentInstnace path should be resolved to.
		* Cannot deal with single-element (i.e. direct target ID) paths; that should be dealt with in the calling code.
		*
		* @param sourceComponentInstance The component containing the path. If the path is relative, the path will be interpreted relative to this component
		* @param targetEntityInstancePath The path (absolute or relative) to the target EntityInstance
		* @param rootEntityInstances All root entities. Used to resolve absolute EntityInstance paths
		* @param errorState The error state
		* @return Pointer to the resolved EntityInstance. Null if the path failed to resolve (in which case errorState will contain more information)
		*/
		static EntityInstance* sResolveEntityInstancePath(ComponentInstance* sourceComponentInstance, const std::string& targetEntityInstancePath, const RootEntityInstanceMap& rootEntityInstances,
			utility::ErrorState& errorState)
		{
			// Split the path into its components
			std::vector<std::string> path_components;
			utility::splitString(targetEntityInstancePath, '/', path_components);

			assert(!path_components.empty());

			// The path consists out of multiple elements, indicating either a relative or absolute path to a component instance.
			// We need to determine the entity that the path 'starts' at so that we can resolve the rest
			nap::EntityInstance* current_entity = nullptr;
			const std::string& root_element = path_components[0];

			// If the part starts with a period, it means we should start in the entity that the source component is in
			if (root_element == ".")
			{
				current_entity = sourceComponentInstance->getEntityInstance();
			}
			else if (root_element == "..")
			{
				// Part starts with a double period; start at the parent of the entity that the source component is in
				current_entity = sourceComponentInstance->getEntityInstance()->getParent();
				if (!errorState.check(current_entity != nullptr, "Error resolving entity path %s: path starts with '..' but source entity has no parent", targetEntityInstancePath.c_str()))
					return nullptr;
			}
			else
			{
				// No relative path components: the first element on the path represents the ID of a root entity. We find it here.
				RootEntityInstanceMap::const_iterator pos = rootEntityInstances.find(root_element);
				if (!errorState.check(pos != rootEntityInstances.end(), "Error resolving entity path %s: root entity '%s' not found", targetEntityInstancePath.c_str(), root_element.c_str()))
					return nullptr;

				current_entity = pos->second;
			}

			// Now resolve the rest of the path. Note that we iterate from the second element (because we've already processed the root) to the second-to-last element (because the last element specifies the component we're looking for )
			for (int index = 1; index < path_components.size(); ++index)
			{
				const std::string& part = path_components[index];

				// If we encounter a double period, go up another level
				if (part == "..")
				{
					current_entity = current_entity->getParent();
					if (!errorState.check(current_entity != nullptr, "Error resolving entity path %s: path contains a '..' at a point where there are no more parents", targetEntityInstancePath.c_str()))
						return nullptr;
				}
				else if (part != ".")
				{
					// If we encountered a non-relative component, we need to look for a child entity of the current entity that matches the child specifier

					// Split the child specifier on ':'. Note that the ':' is optional and is only used to disambiguate between multiple children
					std::vector<std::string> element_parts;
					utility::splitString(part, ':', element_parts);
					if (!errorState.check(element_parts.size() <= 2, "Error resolving entity path %s: path contains a child specifier with an invalid format (multiple colons found)", targetEntityInstancePath.c_str()))
						return nullptr;

					// Find all child entities matching the ID
					std::vector<EntityInstance*> matching_children;
					for (EntityInstance* entity_instance : current_entity->getChildren())
						if (entity_instance->getEntity()->mID == element_parts[0])
							matching_children.push_back(entity_instance);

					// There must be at least one match
					if (!errorState.check(matching_children.size() != 0, "Error resolving entity path %s: child with ID '%s' not found in entity with ID '%s'", targetEntityInstancePath.c_str(), element_parts[0].c_str(), current_entity->getEntity()->mID.c_str()))
						return nullptr;

					// If the child specifier was a single ID, there must be only a single match and we set that entity as the new current entity
					if (element_parts.size() == 1)
					{
						if (!errorState.check(matching_children.size() == 1, "Error resolving entity path %s: path is ambiguous; found %d children with ID '%s' in entity with ID '%s'. Use the child specifier syntax 'child_id:child_index' to disambiguate.", targetEntityInstancePath.c_str(), matching_children.size(), element_parts[0].c_str(), current_entity->getEntity()->mID.c_str()))
							return nullptr;

						current_entity = matching_children[0];
					}
					else
					{
						// The child specifier contained an index to disambiguate between multiple children with the same ID; parse the index
						int array_index;
						if (!errorState.check(sscanf(element_parts[1].c_str(), "%d", &array_index) == 1, "Error resolving entity path %s: path contains a child specifier with an invalid format (unable to parse int from %s)", targetEntityInstancePath.c_str(), element_parts[1].c_str()))
							return nullptr;

						if (!errorState.check(array_index < matching_children.size(), "Error resolving entity path %s: path contains an invalid child specifier; found %d eligible children but index %d is out of range", targetEntityInstancePath.c_str(), matching_children.size(), array_index))
							return nullptr;

						// Use the child with the specified index as current entity
						current_entity = matching_children[array_index];
					}
				}
			}

			return current_entity;
		}

		/**
		* Resolves a path to an entity instance to an actual EntityInstance.
		* @param sourceComponentInstance The component containing the path. If the path is relative, the path will be interpreted relative to this component
		* @param targetEntityInstancePath The path (absolute or relative) to the target EntityInstance
		* @param rootEntityInstances All root entities. Used to resolve absolute EntityInstance paths
		* @param entityInstances All ComponentInstances. Used to resolve single-element (i.e. direct target ID) paths
		* @param errorState The error state
		* @return Pointer to the resolved EntityInstance. Null if the path failed to resolve (in which case errorState will contain more information)
		*/
		static EntityInstance* sResolveEntityInstancePath(ComponentInstance* sourceComponentInstance,
														  const std::string& targetEntityInstancePath,
														  Entity* targetEntityResource,
														  const RootEntityInstanceMap& rootEntityInstances,
														  const EntityCreationParameters::EntityInstanceMap& entityInstances,
														  utility::ErrorState& errorState)
		{
			// Split the path into its components
			std::vector<std::string> path_components;
			utility::splitString(targetEntityInstancePath, '/', path_components);

			// If the path consists out of a single element, we're linking directly to a specific entity so we can just look that up
			if (path_components.size() == 1)
			{
				EntityCreationParameters::EntityInstanceMap::const_iterator pos = entityInstances.find(targetEntityResource);
				if (!errorState.check(pos != entityInstances.end(), "Error resolving entity path %s: target entity not found", targetEntityInstancePath.c_str()))
					return nullptr;

				// If we're linking directly to a specific component, ensure there is no ambiguity
				if (!errorState.check(pos->second.size() == 1, "Error resolving entity path %s: target entity is ambiguous because there are %d instances. Use a relative or absolute path to fix.", targetEntityInstancePath.c_str(), pos->second.size()))
					return nullptr;

				return pos->second[0];
			}

			return sResolveEntityInstancePath(sourceComponentInstance, targetEntityInstancePath, rootEntityInstances, errorState);
		}

		/**
		 * Resolves a path to a component instance to an actual ComponentInstance.
		 *
		 * @param sourceComponentInstance
		 * 		The component containing the path.
		 * 		If the path is relative, the path will be interpreted relative to this component
		 *
		 * @param targetComponentInstancePath
		 * 		The path (absolute or relative) to the target ComponentInstance
		 *
		 * @param rootEntityInstances
		 * 		All root entities. Used to resolve absolute ComponentInstance paths
		 *
		 * @param componentInstances All ComponentInstnaces.
		 * 		Used to resolve single-element (i.e. direct target ID) paths
		 *
		 * @param errorState The error state
		 * @return Pointer to the resolved ComponentInstance.
		 * 		Null if the path failed to resolve (in which case errorState will contain more information)
		 */
		static ComponentInstance* sResolveComponentInstancePath(ComponentInstance* sourceComponentInstance,
																const std::string& targetComponentInstancePath,
																Component* targetComponentResource,
																const RootEntityInstanceMap& rootEntityInstances,
																const EntityCreationParameters::ComponentInstanceMap& componentInstances,
																utility::ErrorState& errorState)
		{
			// Split the path into its components
			std::vector<std::string> path_components;
			utility::splitString(targetComponentInstancePath, '/', path_components);

			// If the path consists of a single element,
			// we're linking directly to a specific component so we can just use that
			if (path_components.size() == 1)
			{
				EntityCreationParameters::ComponentInstanceMap::const_iterator pos = componentInstances.find(targetComponentResource);
				if (!errorState.check(pos != componentInstances.end(),
									  "Error resolving component path %s: taret component not found",
									  targetComponentInstancePath.c_str()))
					return nullptr;

				// If we're linking directly to a specific component, ensure there is no ambiguity
				if (!errorState.check(pos->second.size() == 1, "Encountered ambiguous component pointer"))
					return nullptr;

				return pos->second[0];
			}

			assert(!path_components.empty());

			// cut off trailing element (the component), result will be the path to the owning Entity
			std::string entityInstancePath = targetComponentInstancePath.substr(0, targetComponentInstancePath.find_last_of('/'));

			// find owning Entity
			nap::EntityInstance* target_entity = sResolveEntityInstancePath(sourceComponentInstance,
																			entityInstancePath,
																			rootEntityInstances,
																			errorState);
			if (!errorState.check(target_entity != nullptr,
								  "Error resolving ComponentPtr with path %s: target entity instance for %s not found",
								  targetComponentInstancePath.c_str(), entityInstancePath.c_str()))
				return nullptr;

			// Now that we've gone through the path, we know the entity must contain a component with an ID equal
			// to the last element on the path. We look for it here.
			assert(target_entity != nullptr);
			for (ComponentInstance* component : target_entity->getComponents())
			{
				// If this ComponentInstance's resource is a clone (for instance properties),
				// we need to check the ID of the original object,
				// since the clone will have a generated ID, which will never match any path.
				if (component->getComponent()->getOriginalID() == path_components.back())
					return component;
			}

			errorState.fail("Error resolving ComponentPtr with path %s: component not found on entity %s", targetComponentInstancePath.c_str(), target_entity->getEntity()->mID.c_str());
			return nullptr;
		}


		/**
		 * This function resolves pointers in a ComponentResource of the types EntityInstancePtr & ComponentInstancePtr.
		 * Although the RTTI resource pointers in EntityPtr and ComponentPtr have already been resolved by the regular
		 * RTTI pointer resolving step, this step is meant explicitly to resolve pointers to instances that are stored
		 * internally in the ComponentInstancePtr and EntityInstancePtr.
		 *
		 * The resolving step of entities and components is more difficult than regular objects,
		 * as the entity/component structure is mirrored into a resource structure
		 * (the static objects from json) and instances (the runtime counterpart of the resources).
		 *
		 * EntityPtr and ComponentPtr are pointers that live on the resource object as the resources need to specify
		 * what other resource they are pointing to.
		 * However, the instantiated object often needs to point to other instantiated objects.
		 * In this function, we fill in the instance pointers in EntityInstancePtr and ComponentInstancePtr.
		 */
		static bool sResolveInstancePointers(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
		{
			RootEntityInstanceMap root_entity_instances;
			for (auto& kvp : entityCreationParams.mEntityInstancesByID)
			{
				EntityInstance* entity_instance = kvp.second.get();
				if (entity_instance->getParent() == nullptr)
					root_entity_instances.emplace(std::make_pair(kvp.first, entity_instance));
			}

			// We go over all component instances and resolve all Entity & Component instance pointers
			for (auto& kvp : entityCreationParams.mComponentInstanceMap)
			{
				// Resolve the component pointers for all instances of this component resource
				for (ComponentInstance* source_component_instance : kvp.second)
				{
					// Resolve all component links for this instance
					ComponentInstance::ComponentLinkMap& componentLinkmap = source_component_instance->mComponentLinkMap;
					for (auto& link : componentLinkmap)
					{
						nap::Component* target_component_resource = link.first;
						if (target_component_resource == nullptr)
							continue;

						// It's possible for the same ComponentInstance to link to a particular component multiple times, so we need to resolve all those links individually (the paths might be different)
						for (ComponentInstance::TargetComponentLink& target_component_link : link.second)
						{
							// Resolve the path to the target ComponentInstance
							nap::ComponentInstance* target_component_instance = sResolveComponentInstancePath(source_component_instance, target_component_link.mInstancePath, target_component_resource, root_entity_instances, entityCreationParams.mComponentInstanceMap, errorState);
							if (!errorState.check(target_component_instance != nullptr, "Unable to resolve component pointer to %s from %s; target not found", target_component_link.mInstancePath.c_str(), source_component_instance->getComponent()->mID.c_str()))
								return false;

							// Update the ComponentInstancePtr
							*target_component_link.mTargetPtr = target_component_instance;
						}
					}

					// Resolve all entity links for this instance
					ComponentInstance::EntityLinkMap& entityLinkmap = source_component_instance->mEntityLinkMap;
					for (auto& link : entityLinkmap)
					{
						nap::Entity* target_entity_resource = link.first;

						// It's possible for the same ComponentInstance to link to a particular component multiple times, so we need to resolve all those links individually (the paths might be different)
						for (ComponentInstance::TargetEntityLink& target_entity_link : link.second)
						{
							// Resolve the path to the target ComponentInstance
							nap::EntityInstance* target_entity_instance = sResolveEntityInstancePath(source_component_instance, target_entity_link.mInstancePath, target_entity_resource, root_entity_instances, entityCreationParams.mEntityInstanceMap, errorState);
							if (!errorState.check(target_entity_instance != nullptr, "Unable to resolve entity pointer to %s from %s; target not found", target_entity_link.mInstancePath.c_str(), source_component_instance->getComponent()->mID.c_str()))
								return false;

							// Update the EntityInstancePtr
							*target_entity_link.mTargetPtr = target_entity_instance;
						}
					}
				}
			}

			return true;
		}
	};

	/**
	 * Recursively update the transforms of all entities in the hierarchy
	 */
	static void sUpdateTransformsRecursive(EntityInstance& entity, bool parentDirty, const glm::mat4& parentTransform)
	{
		glm::mat4 new_transform = parentTransform;

		bool is_dirty = parentDirty;
		TransformComponentInstance* transform = entity.findComponent<TransformComponentInstance>();
		if (transform && (transform->isDirty() || parentDirty))
		{
			is_dirty = true;
			transform->update(parentTransform);
			new_transform = transform->getGlobalTransform();
		}

		for (EntityInstance* child : entity.getChildren())
			sUpdateTransformsRecursive(*child, is_dirty, new_transform);
	}

	//////////////////////////////////////////////////////////////////////////

	Scene::Scene(Core& core) :
		mCore(&core)
	{
		mCore->getService<SceneService>()->registerScene(*this);

		mRootEntity = std::make_unique<EntityInstance>(*mCore, nullptr);
	}


	Scene::~Scene()
	{
		mCore->getService<SceneService>()->unregisterScene(*this);
	}


	void Scene::update(double deltaTime)
	{
		mRootEntity->update(deltaTime);
	}


	void Scene::updateTransforms(double deltaTime)
	{
		sUpdateTransformsRecursive(*mRootEntity, false, glm::mat4(1.0f));
	}


	EntityInstance* Scene::createChildEntityInstance(const nap::Entity& entity, int childIndex, EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		entityCreationParams.mCurrentEntityPath.push(childIndex);
		EntityInstance* entity_instance = createEntityInstance(entity, entityCreationParams, errorState);
		entityCreationParams.mCurrentEntityPath.pop();

		if (!errorState.check(entity_instance != nullptr, "Failed to create child entity"))
			return nullptr;

		return entity_instance;
	}


	EntityInstance* Scene::createEntityInstance(const nap::Entity& entity, EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		EntityInstance* entity_instance = new EntityInstance(*mCore, &entity);
		entity_instance->mID = SceneInstantiation::sGenerateInstanceID(SceneInstantiation::sGetInstanceID(entity.mID), entityCreationParams);

		entityCreationParams.mEntityInstancesByID.emplace(std::make_pair(entity_instance->mID, std::unique_ptr<EntityInstance>(entity_instance)));
		entityCreationParams.mAllInstancesByID.insert(std::make_pair(entity_instance->mID, entity_instance));
		entityCreationParams.mEntityInstanceMap[const_cast<Entity*>(&entity)].push_back(entity_instance);

		for (auto& original_component_resource : entity.mComponents)
		{
			entityCreationParams.mCurrentEntityPath.pushComponent(original_component_resource->mID);

			Component* cloned_component_resource = SceneInstantiation::sFindClonedComponent(entityCreationParams.mCurrentEntityPath, entityCreationParams.mCurrentEntityClonedComponents);
			Component* component_resource = cloned_component_resource != nullptr ? cloned_component_resource : original_component_resource.get();

			const rtti::TypeInfo& instance_type = component_resource->getInstanceType();
			assert(instance_type.can_create_instance());

			std::unique_ptr<ComponentInstance> component_instance(instance_type.create<ComponentInstance>({ *entity_instance, *component_resource }));
			assert(component_instance);
			component_instance->mID = SceneInstantiation::sGenerateInstanceID(SceneInstantiation::sGetInstanceID(component_resource->mID), entityCreationParams);

			entityCreationParams.mComponentInstanceMap[component_resource].push_back(component_instance.get());
			entityCreationParams.mAllInstancesByID.insert(std::make_pair(component_instance->mID, component_instance.get()));
			entity_instance->addComponent(std::move(component_instance));

			entityCreationParams.mCurrentEntityPath.popComponent();
		}

		if (!entity_instance->init(*this, entityCreationParams, errorState))
			return nullptr;

		return entity_instance;
	}

	bool Scene::spawnInternal(const RootEntityList& rootEntities, const std::vector<rtti::Object*>& allObjects, bool clearChildren, std::vector<EntityInstance*>& spawnedRootEntityInstances, utility::ErrorState& errorState)
	{
		EntityObjectGraph object_graph;
		ObjectsByTypeMap objects_by_type;			// Used by EntityObjectGraphItem to find dependencies between types
		ClonedResourceMap cloned_resource_map;		// Used by EntityObjectGraphItem to add edges to cloned resources

		// Build map of objects per type, this is used for tracking type dependencies while building the graph
		for (rtti::Object* object : allObjects)
			SceneInstantiation::sRecursiveAddToObjectsByType(*object, object->get_type(), objects_by_type);

		if (!object_graph.build(allObjects, [&objects_by_type, &cloned_resource_map](rtti::Object* object) { return EntityObjectGraphItem::create(object, objects_by_type, cloned_resource_map); }, errorState))
			return false;

		ClonedComponentByEntityMap		cloned_components_by_entity;	// Map owning the cloned component resource, is moved later to mClonedComponentsByEntity on success
		std::unordered_set<std::string>	cloned_component_ids;			// Set used to generate unique IDs for all cloned components

		EntityCreationParameters entityCreationParams(object_graph);
        
        // Fill EntityCreationParams.mAllInstancesByID with all existing entities and components in the Scene.
        // When we spawn entities at runtime after initialization we need this information in order to generate unique mID values.
        for (auto& entityPair : mEntityInstancesByID)
        {
            entityCreationParams.mAllInstancesByID[entityPair.first] = entityPair.second.get();
            for (auto component : entityPair.second->getComponents())
                entityCreationParams.mAllInstancesByID[component->mID] = component;
        }

		// Create clones of all Components in all entities that have InstanceProperties set for them.
		// Note that while InstanceProperties can only be set on the root Entity, they can still target Components in child entities
		for (const RootEntity& root_entity : rootEntities)
		{
			if (root_entity.mInstanceProperties.empty())
				continue;

			const Entity* entity = root_entity.mEntity.get();

			ClonedComponentResourceList& clonedComponents = cloned_components_by_entity[entity];
			for (const ComponentInstanceProperties& instance_property : root_entity.mInstanceProperties)
			{
				// Clone target component. The cloned resources are not going into the regular resource manager resource lists, 
				// but into a special map of cloned resources that is thrown away whenever something changes
				// Note: We have to generate a unique ID for it because the ObjectGraph expects IDs to be unique
				std::unique_ptr<Component> cloned_target_component = rtti::cloneObject<Component>(*instance_property.mTargetComponent, mCore->getResourceManager()->getFactory());
				cloned_target_component->mID = SceneInstantiation::sGenerateUniqueID(cloned_target_component->mID + "_instanceproperties", cloned_component_ids);
				cloned_target_component->mOriginalComponent = instance_property.mTargetComponent.get();

				// Update the objects by type and cloned resource maps, used by RTTIGraphObjectItem and required to rebuild the ObjectGraph later
				SceneInstantiation::sRecursiveAddToObjectsByType(*cloned_target_component, cloned_target_component->get_type(), objects_by_type);
				cloned_resource_map[instance_property.mTargetComponent.get()].push_back(cloned_target_component.get());

				ComponentResourcePath resolved_component_path;
				if (!errorState.check(ComponentResourcePath::fromString(*entity, instance_property.mTargetComponent.getInstancePath(), resolved_component_path, errorState), "Failed to apply instance property for entity %s: invalid component path %s", entity->mID.c_str(), instance_property.mTargetComponent.getInstancePath().c_str()))
					return false;

				// Apply instance properties to the cloned object
				for (const TargetAttribute& attribute : instance_property.mTargetAttributes)
					if (!errorState.check(attribute.apply(*cloned_target_component, errorState), "Failed to apply instance properties for entity %s", entity->mID.c_str()))
						return false;

				clonedComponents.emplace_back(ClonedComponentResource(resolved_component_path, std::move(cloned_target_component)));
			}
		}

		// After cloning all relevant resources, we need to rebuild the object graph, in order to incorporate the newly cloned nodes and their incoming/outgoing links
		// Otherwise, the depth of some nodes may be wrong, leading to the wrong init order
		if (!object_graph.rebuild(errorState))
			return false;

		// Create all entity instances and component instances
		for (const RootEntity& root_entity : rootEntities)
		{
			const Entity* root_entity_resource = root_entity.mEntity.get();

			// Here we spawn a single entity hierarchy. Set up the path for this entity
			entityCreationParams.mCurrentEntityPath = ComponentResourcePath(*root_entity_resource);

			// Store the cloned components used for this entity
			ClonedComponentByEntityMap::iterator clonedListPos = cloned_components_by_entity.find(root_entity_resource);
			entityCreationParams.mCurrentEntityClonedComponents = clonedListPos != cloned_components_by_entity.end() ? &clonedListPos->second : nullptr;

			// Spawn the entity hierarchy
			if (!errorState.check(createEntityInstance(*root_entity_resource, entityCreationParams, errorState) != nullptr, "Failed to create entity instance"))
				return false;
		}

		// After all entity hierarchies and their components are created, the component & entity pointers are resolved and then initted in the correct order.
		if (!errorState.check(SceneInstantiation::sResolveInstancePointers(entityCreationParams, errorState), "Unable to resolve pointers in components"))
			return false;

		if (!errorState.check(SceneInstantiation::sInitComponents(entityCreationParams, errorState), "Unable to init components!"))
			return false;

		// Start with an empty root and add all entities without a parent to the root
		if (clearChildren)
			mRootEntity->clearChildren();

		for (auto& kvp : entityCreationParams.mEntityInstancesByID)
		{
			if (kvp.second->getParent() == nullptr)
			{
				spawnedRootEntityInstances.push_back(kvp.second.get());
				mRootEntity->addChild(*kvp.second);
			}
		}

		// In realtime editing scenarios, clients may have pointers to Entity & Component Instances that will have been respawned.
		// We need to patch all ObjectPtrs to those instances here so that clients don't have to deal with it themselves.
		// For example, a camera may have been stored by the app and stored in an ObjectPtr.
		rtti::ObjectPtrManager::get().patchPointers(entityCreationParams.mAllInstancesByID);

		// Replace entities currently in the resource manager with the new set
		for (auto& kvp : entityCreationParams.mEntityInstancesByID)
			mEntityInstancesByID[kvp.first] = std::move(kvp.second);

		for (auto& kvp : entityCreationParams.mAllInstancesByID)
			mInstancesByID[kvp.first] = kvp.second;

		for (auto& kvp : cloned_components_by_entity)
		{
			ClonedComponentResourceList& clonedComponents = mClonedComponentsByEntity[kvp.first];
			ClonedComponentResourceList& newClonedComponents = kvp.second;

			for (auto& clonedComponent : newClonedComponents)
				clonedComponents.emplace_back(std::move(clonedComponent));
		}

		return true;
	}


	SpawnedEntityInstance Scene::spawn(const Entity& entity, utility::ErrorState& errorState)
	{
		std::vector<rtti::Object*> all_objects;
		rtti::getPointeesRecursive(entity, all_objects);
		all_objects.push_back(const_cast<Entity*>(&entity));

		RootEntity rootEntity;		
		rootEntity.mEntity = const_cast<Entity*>(&entity);

		std::vector<EntityInstance*> spawnedRootEntities;
		if (!spawnInternal({ rootEntity }, all_objects, false, spawnedRootEntities, errorState))
			return nullptr;

		assert(spawnedRootEntities.size() == 1);
		return SpawnedEntityInstance(spawnedRootEntities[0]);
	}


	static void sGetInstancesToDestroyRecursive(EntityInstance& entity, std::vector<rtti::Object*>& instancesToDestroy)
	{
		instancesToDestroy.push_back(&entity);

		for (ComponentInstance* instance : entity.getComponents())
			instancesToDestroy.push_back(instance);

		for (EntityInstance* child : entity.getChildren())
			sGetInstancesToDestroyRecursive(*child, instancesToDestroy);
	}


	void Scene::destroy(SpawnedEntityInstance& entity)
	{
		// Recursively get all instances to destroy (i.e. Entity and Component instances)
		std::vector<rtti::Object*> all_instances;
		sGetInstancesToDestroyRecursive(*entity, all_instances);

		// First remove the entity from the root
		mRootEntity->removeChild(*entity);

		// Note: we simply move all entity instances that need to be deleted into this vector. 
		// Since they're stored through unique_ptrs, the entities will be deleted once the vector goes out of scope.
		// This is needed so that we can still access the entity's data during iteration
		std::vector<std::unique_ptr<EntityInstance>> entity_instances_to_delete;

		// Remove instances from the map, but don't delete them yet
		for (rtti::Object* instance : all_instances)
		{
			rtti::TypeInfo type_info = instance->get_type();

			if (type_info.is_derived_from<EntityInstance>())
			{
				EntityByIDMap::iterator entity_instance = mEntityInstancesByID.find(instance->mID);
				assert(entity_instance != mEntityInstancesByID.end());
				entity_instances_to_delete.emplace_back(std::move(entity_instance->second));

				mEntityInstancesByID.erase(instance->mID);
				mInstancesByID.erase(instance->mID);
			}
			else if (type_info.is_derived_from<ComponentInstance>())
			{
				mInstancesByID.erase(instance->mID);
			}
		}
	}


	bool Scene::init(utility::ErrorState& errorState)
	{
		std::vector<rtti::Object*> all_objects;
		rtti::getPointeesRecursive(*this, all_objects);
		all_objects.push_back(this);

		std::vector<EntityInstance*> spawnedRootEntities;
		return spawnInternal(mEntities, all_objects, true, spawnedRootEntities, errorState);
	}


	const rtti::ObjectPtr<EntityInstance> Scene::findEntity(const std::string& inID) const
	{
		EntityByIDMap::const_iterator pos = mEntityInstancesByID.find(SceneInstantiation::sGetInstanceID(inID));
		if (pos == mEntityInstancesByID.end())
			return nullptr;

		return pos->second.get();
	}
}

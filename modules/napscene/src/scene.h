#pragma once

#include "instanceproperty.h"
#include "entitycreationparameters.h"
#include <rtti/rttiobject.h>
#include <utility/uniqueptrmapiterator.h>
#include <rtti/factory.h>

namespace nap
{
	class Entity;
	class SpawnedEntityInstance;
	class Core;

	/**
	 * Represent a root entity in a scene, along with its instance properties.
	 */
	class RootEntity final
	{
	public:
		rtti::ObjectPtr<Entity>							mEntity;				///< Root entity to spawn
		std::vector<ComponentInstanceProperties>	mInstanceProperties;	//< The instance properties for this entity (and all of its children)
	};

	/**
	 * Container for entities. The Scene is responsible for instantiating all of the entities.
	 */
	class NAPAPI Scene : public rtti::RTTIObject
	{
		RTTI_ENABLE(rtti::RTTIObject)

	public:
		using EntityByIDMap = std::unordered_map<std::string, std::unique_ptr<EntityInstance>>;
		using EntityIterator = utility::UniquePtrMapWrapper<EntityByIDMap, EntityInstance*>;
        using RootEntityList = std::vector<RootEntity>;
		using InstanceByIDMap = std::unordered_map<std::string, rtti::RTTIObject*>;

		Scene(Core& core);
		virtual ~Scene() override;

		/**
		 * Initialize the scene. Will spawn all entities contained in this scene.
		 * As soon as this is called, EntityInstances will become available
		 * and are accessible through #getRootEntity() and #getEntities()
		 */
        virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * Update all entities contained in this scene
		 */
		void update(double deltaTime);

        /**
         * Spawns an entity hierarchy.
		 * @param entity Root Entity to spawn.
		 * @param errorState Contains error information if the returned object is nullptr.
		 * @return On succes, the EntityInstance that was spawned. The Entity can be destroyed 
		 *         by calling destroy with the value returned from this function.
         */		
		SpawnedEntityInstance spawn(const Entity& entity, utility::ErrorState& errorState);

        /**
		 * Destroys a spawned Entity.
		 * @param entity The Entity to destroy. This is the Entity as created using the spawn function.
         */		
		void destroy(SpawnedEntityInstance& entity);

		/**
		 * Update the transform hierarchy of the entities contained in this scene. For any TransformComponent the world transform is updated.
		 */
		void updateTransforms(double deltaTime);

		/**
		 * @return Iterator to all entity instances in this scene.
		 */
		EntityIterator getEntities() { return EntityIterator(mEntityInstancesByID); }

		/**
		 *
		 * @return EntityInstance with the specified identifier from this scene.
		 */
		const rtti::ObjectPtr<EntityInstance> findEntity(const std::string& inID) const;

		/**
		 * @return The root EntityInstance of this scene
		 */
		const EntityInstance& getRootEntity() const		{ return *mRootEntity;}

		/**
		 * @return The root EntityInstance of this scene
		 */
		EntityInstance& getRootEntity()		{ return *mRootEntity;}

        /**
         * @return The RootEntity resources in the scene.
         */
        RootEntityList getEntityResources() { return mEntities; }

	private:
		EntityInstance* createEntityInstance(const Entity& entityResource, EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState);

		/**
		 * Instantiates an EntityInstance from an Entity.
		 */
		EntityInstance* createChildEntityInstance(const Entity& entity, int childIndex, EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState);

		/**
		 * Helper for spawning entities. Used by both spawn and init functions.
		 */
		bool spawnInternal(const RootEntityList& rootEntities, const std::vector<rtti::RTTIObject*>& allObjects, bool clearChildren, std::vector<EntityInstance*>& spawnedRootEntityInstances, utility::ErrorState& errorState);

	public:
		RootEntityList 						mEntities;						///< List of root entities owned by the Scene

	private:
		friend class EntityInstance;

		Core*								mCore;
		std::unique_ptr<EntityInstance>		mRootEntity;					///< Root entity, owned and created by this scene
		EntityByIDMap						mEntityInstancesByID;			///< Holds all spawned entities
		InstanceByIDMap						mInstancesByID;					///< Holds all spawned entities & components
		ClonedComponentByEntityMap			mClonedComponentsByEntity;		///< All cloned components, stored by entity. This map owns the cloned resources.
	};

	using SceneCreator = rtti::ObjectCreator<Scene, Core>;
}

#pragma once

// Local Includes
#include "rendercomponent.h"
#include "nap/objectptr.h"
#include "vao.h"


namespace nap
{
	class MeshResource;
	class MaterialInstance;
	class TransformComponent;
	class RenderableMeshComponent;
	class RenderableMeshComponentResource : public RenderableComponentResource
	{
		RTTI_ENABLE(RenderableComponentResource)

		virtual void getDependentComponents(std::vector<rtti::TypeInfo>& components)
		{
			components.push_back(RTTI_OF(TransformComponent));
		}

		virtual const rtti::TypeInfo getInstanceType() const
		{
			return RTTI_OF(RenderableMeshComponent);
		}

	public:
		ObjectPtr<MeshResource>				mMeshResource;
		ObjectPtr<MaterialInstance>			mMaterialInstance;					///< MaterialInstance, which is used to override uniforms for this instance
	};

	/**
	 * Represents a drawable mesh that can be used as a component in an object tree
	 * Every RenderableMeshComponent has a pointer to a RenderableMeshResource, which
	 * is an object that represents a mesh with a material assigned to it. To override
	 * uniform shader values from that material for this particular instance, the 
	 * component has a MaterialInstance.
	 */
	class RenderableMeshComponent : public RenderableComponent
	{
		RTTI_ENABLE(RenderableComponent)

	public:
		RenderableMeshComponent(EntityInstance& entity);

		/**
		 * 
		 */
		virtual bool init(const ObjectPtr<ComponentResource>& resource, utility::ErrorState& errorState);

		/**
		 * Renders the model from the ModelResource, using the material on the ModelResource.
		 */
		virtual void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;

		/**
		 * @return MaterialInstance for this component.
		 */
		MaterialInstance* getMaterialInstance();

	private:
		void pushUniforms();

	private:
		ObjectPtr<RenderableMeshComponentResource>	mResource;
		TransformComponent*							mTransformComponent;
		std::unique_ptr<VAOHandle>					mVAOHandle;
	};
}

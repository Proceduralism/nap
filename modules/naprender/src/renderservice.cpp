// Local Includes
#include "renderservice.h"
#include "renderablemeshcomponent.h"
#include "rendercomponent.h"
#include "renderwindowresource.h"
#include "transformcomponent.h"
#include "cameracomponent.h"
#include "renderglobals.h"
#include "meshresource.h"
#include "rtti/factory.h"
#include "nap/resourcemanager.h"
#include "depthsorter.h"

// External Includes
#include <nap/core.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace nap
{
	// Register all types
	void RenderService::registerTypes(nap::Core& core)
	{
	}

	// Register all object creation functions
	void RenderService::registerObjectCreators(rtti::Factory& factory)
	{
		factory.addObjectCreator(std::make_unique<RenderWindowResourceCreator>(*this));
	}


	std::unique_ptr<RenderWindow> RenderService::addWindow(RenderWindowResource& window, utility::ErrorState& errorState)
	{
		assert(mRenderer != nullptr);

		// Get settings
		RenderWindowSettings window_settings;
		window_settings.width		= window.mWidth;
		window_settings.height		= window.mHeight;
		window_settings.borderless	= window.mBorderless;
		window_settings.resizable	= window.mResizable;
		window_settings.title		= window.mTitle;

		std::unique_ptr<RenderWindow> new_window = mRenderer->createRenderWindow(window_settings, errorState);
		if (new_window == nullptr)
			return nullptr;

		mWindows.push_back(&window);

		// After window creation, make sure the primary window stays active, so that render resource creation always goes to that context
		getPrimaryWindow().makeCurrent();

		return new_window;
	}


	void RenderService::removeWindow(RenderWindowResource& window)
	{
		WindowList::iterator pos = std::find_if(mWindows.begin(), mWindows.end(), [&](auto val) { return val == &window; });
		assert(pos != mWindows.end());
		mWindows.erase(pos);
	}
	

	RenderWindowResource* RenderService::findWindow(void* nativeWindow) const
	{
		WindowList::const_iterator pos = std::find_if(mWindows.begin(), mWindows.end(), [&](auto val) { return val->getWindow()->getNativeWindow() == nativeWindow; });
		if (pos != mWindows.end())
			return *pos;

		return nullptr;
	}


	ObjectPtr<RenderWindowResource> RenderService::getWindow(uint id) const
	{
		SDL_Window* gl_window = opengl::getWindow(id);
		return gl_window != nullptr ? findWindow(gl_window) : nullptr;
	}


	RenderWindow& RenderService::getPrimaryWindow()
	{
		return mRenderer->getPrimaryWindow();
	}


	// Shut down render service
	RenderService::~RenderService()
	{
		shutdown();
	}


	// Render all objects in scene graph using specified camera
	void RenderService::renderObjects(opengl::RenderTarget& renderTarget, CameraComponent& camera)
	{
		// Get all render components
 		std::vector<nap::RenderableComponent*> render_comps;

		for (EntityInstance* entity : getCore().getService<ResourceManagerService>()->getEntities())
			entity->getComponentsOfType<nap::RenderableComponent>(render_comps);

		// Split into front to back and back to front meshes
		std::vector<nap::RenderableComponent*> front_to_back;
		std::vector<nap::RenderableComponent*> back_to_front;

		for (nap::RenderableComponent* component : render_comps)
		{
			nap::RenderableMeshComponent* renderable_mesh = rtti_cast<RenderableMeshComponent>(component);
			if (renderable_mesh != nullptr)
			{
				EBlendMode blend_mode = renderable_mesh->getMaterialInstance().getBlendMode();
				if (blend_mode == EBlendMode::AlphaBlend)
					back_to_front.push_back(component);
				else
					front_to_back.push_back(component);
			}
		}
		
		// Sort front to back and render those first
		DepthSorter front_to_back_sorter(DepthSorter::EMode::FrontToBack, camera.getViewMatrix());
		std::sort(front_to_back.begin(), front_to_back.end(), front_to_back_sorter);
		renderObjects(renderTarget, camera, front_to_back);

		// Then sort back to front and render these
		DepthSorter back_to_front_sorter(DepthSorter::EMode::BackToFront, camera.getViewMatrix());
		std::sort(back_to_front.begin(), back_to_front.end(), back_to_front_sorter);
		renderObjects(renderTarget, camera, back_to_front);
	}

	// Updates the current context's render state by using the latest render state as set by the user.
	void RenderService::updateRenderState()
	{
		opengl::GLContext context = opengl::getCurrentContext();
		ContextSpecificStateMap::iterator context_state = mContextSpecificState.find(context);
		if (context_state == mContextSpecificState.end())
		{
			mContextSpecificState.emplace(std::make_pair(context, mRenderState));
			mContextSpecificState[context].force();
		}
		else
		{
			context_state->second.update(mRenderState);
		}
	}

	// Renders all available objects to a specific renderTarget.
	void RenderService::renderObjects(opengl::RenderTarget& renderTarget, CameraComponent& camera, const std::vector<RenderableComponent*>& comps)
	{
		renderTarget.bind();

		// Before we render, we always set render target size. This avoids overly complex
		// responding to various changes in render target sizes.
		camera.setRenderTargetSize(renderTarget.getSize());

		updateRenderState();

		// Extract camera projection matrix
		const glm::mat4x4 projection_matrix = camera.getProjectionMatrix();

		// Extract view matrix
		glm::mat4x4 view_matrix = camera.getViewMatrix();

		// Draw
		for (auto& comp : comps)
			comp->draw(view_matrix, projection_matrix);

		renderTarget.unbind();
	}


	// Clears the render target.
	void RenderService::clearRenderTarget(opengl::RenderTarget& renderTarget, opengl::EClearFlags flags)
	{
		renderTarget.bind();
		renderTarget.clear(flags);
		renderTarget.unbind();
	}


	// Set the currently active renderer
	bool RenderService::init(nap::utility::ErrorState& errorState)
	{
		std::unique_ptr<Renderer> renderer = std::make_unique<nap::Renderer>();
		if (!renderer->init(errorState))
			return false;

		mRenderer = std::move(renderer);

		return true;
	}
	
	void RenderService::queueResourceForDestruction(std::unique_ptr<opengl::IGLContextResource> resource) 
	{ 
		if (resource != nullptr)
			mGLContextResourcesToDestroy.emplace_back(std::move(resource)); 
	}


	void RenderService::destroyGLContextResources(const std::vector<ObjectPtr<RenderWindowResource>>& renderWindows)
	{
		// If there is anything scheduled, destroy
		if (!mGLContextResourcesToDestroy.empty())
		{
			// Destroy resources for primary window
			getPrimaryWindow().makeCurrent();
			for (auto& resource : mGLContextResourcesToDestroy)
				resource->destroy(getPrimaryWindow().getContext());

			// We go over the windows to make the GL context active, and then destroy 
			// the resources for that context
			for (const ObjectPtr<RenderWindowResource>& render_window : renderWindows)
			{
				render_window->makeActive();
				for (auto& resource : mGLContextResourcesToDestroy)
					resource->destroy(render_window->getWindow()->getContext());
			}
			mGLContextResourcesToDestroy.clear();
		}
	}


	// Shut down renderer
	void RenderService::shutdown()
	{
		assert(mRenderer != nullptr);
		mRenderer->shutdown();
	}


	std::unique_ptr<VAOHandle> RenderService::acquireVertexArrayObject(const Material& material, const MeshResource& meshResource, utility::ErrorState& errorState)
	{
		/// Construct a key based on material-mesh, and see if we have a VAO for this combination
		VAOKey key(material, meshResource);
		VAOMap::iterator kvp = mVAOMap.find(key);
		if (kvp != mVAOMap.end())
		{
			// Increase refcount and return handle to our internal opengl object
			++kvp->second.mRefCount;
			return VAOHandle::create(*this, kvp->second.mObject.get());
		}

		// VAO was not found for this material-mesh combination, create a new one
		RefCountedVAO ref_counted_vao;
		ref_counted_vao.mObject = std::make_unique<opengl::VertexArrayObject>();

		// Use the mapping in the material to bind mesh vertex attrs to shader vertex attrs
		for (auto& kvp : material.getShader()->getShader().getAttributes())
		{
			const opengl::VertexAttribute* shader_vertex_attribute = kvp.second.get();

			const Material::VertexAttributeBinding* material_binding = material.findVertexAttributeBinding(kvp.first);
			if (!errorState.check(material_binding != nullptr, "Unable to find binding %s for shader %s in material %s", kvp.first.c_str(), material.getShader()->mVertPath.c_str(), material.mID.c_str()))
				return nullptr;

			const opengl::VertexAttributeBuffer* vertex_buffer = meshResource.getMesh().findVertexAttributeBuffer(material_binding->mMeshAttributeID);
			if (!errorState.check(shader_vertex_attribute != nullptr, "Unable to find vertex attribute %s in mesh %s", material_binding->mMeshAttributeID.c_str(), meshResource.mID.c_str()))
				return nullptr;

			ref_counted_vao.mObject->addVertexBuffer(shader_vertex_attribute->mLocation, *vertex_buffer);
		}

		auto inserted = mVAOMap.emplace(key, std::move(ref_counted_vao));

		return VAOHandle::create(*this, inserted.first->second.mObject.get());
	}


	void RenderService::releaseVertexArrayObject(opengl::VertexArrayObject* vao)
	{
		// Find the VAO in the map by value
		VAOMap::iterator it = find_if(mVAOMap.begin(), mVAOMap.end(), [&](auto&& kvp) { return kvp.second.mObject.get() == vao; });
		assert(it != mVAOMap.end());

		// If this is the last usage of this VAO, queue it for destruction (VAOs need to be destructed per active context,
		// so we defer destruction)
		if (--it->second.mRefCount == 0)
		{
			queueResourceForDestruction(std::move(it->second.mObject));
			mVAOMap.erase(it);
		}
	}

} // Renderservice

RTTI_DEFINE(nap::RenderService)
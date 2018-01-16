#pragma once

// Mod nap render includes
#include <renderablemeshcomponent.h>
#include <renderwindow.h>
#include <sdlinput.h>
#include <sdlwindow.h>

// Nap includes
#include <nap/resourcemanager.h>
#include <sceneservice.h>
#include <inputservice.h>
#include <inputrouter.h>
#include <videoservice.h>
#include <video.h>
#include <app.h>

namespace nap
{
	/**
	 * Main application that is called from within the main loop
	 */
	class VideoApp : public App
	{
		RTTI_ENABLE(App)
	public:
		VideoApp(nap::Core& core) : App(core)	{}
		
		/**
		 *	Initialize app specific data structures
		 */
		bool init(utility::ErrorState& error) override;
		
		/**
		 *	Update is called before render, performs all the app logic
		 */
		void update(double deltaTime) override;

		/**
		 *	Render is called after update, pushes all renderable objects to the GPU
		 */
		void render() override;

		/**
		 *	Called when a window event is received
		 */
		void handleWindowEvent(const WindowEvent& windowEvent);

		/**
		 *	Forwards the received window event to the render service
		 */
		void windowMessageReceived(WindowEventPtr windowEvent) override;
		
		/**
		 *  Forwards the received input event to the input service
		 */
		void inputMessageReceived(InputEventPtr inputEvent) override;
		
		/**
		 *	Toggles full screen
		 */
		void setWindowFullscreen(std::string windowIdentifier, bool fullscreen);
		
		/**
		 *	Called when loop finishes
		 */
		int shutdown() override;
		
	private:
		
		// Nap Services
		RenderService*		mRenderService = nullptr;				//< Render Service that handles render calls
		ResourceManager*	mResourceManager = nullptr;				//< Manages all the loaded resources
		SceneService*		mSceneService = nullptr;				//< Manages all the objects in the scene
		InputService*		mInputService = nullptr;				//< Input service for processing input
		VideoService*		mVideoService = nullptr;				//< Service for video playback
		
		std::vector<ObjectPtr<RenderWindow>> mRenderWindows;		//< Vector holding pointers to the spawned render windows
		
		ObjectPtr<EntityInstance> mCameraEntity = nullptr;			//< The entity that holds the camera
		ObjectPtr<EntityInstance> mVideoEntity = nullptr;			//< The video that holds the camera

		ObjectPtr<Video> mVideoResource = nullptr;					//< Our video resource
	};
}

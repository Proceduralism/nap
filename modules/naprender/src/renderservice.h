#pragma once

// External Includes
#include <nap/attribute.h>
#include <nap/service.h>
#include <nopengl.h>
#include <thread>
#include <nwindow.h>

namespace nap
{
	/**
	 * Holds a reference to all drawable objects
	 */
	class RenderService : public Service
	{
		RTTI_ENABLE_DERIVED_FROM(Service)

	public:
		RenderService() = default;

		/**
		 * Call this in your app loop to emit a render call
		 * When called the draw bang is emitted
		 */
		void render();

		/**
		 * The draw signal that is emitted every render call
		 * Register to this event to receive draw calls
		 */
		SignalAttribute draw = {this, "draw"};

        opengl::Window* getWindow();
        void destroyWindow(opengl::Window* window);

	protected:
		/**
		 * Type registration
		 */
		virtual void registerTypes(nap::Core& core) override;

    private:

        void renderLoop();

        void renderCall();

        void updateViewport(int width, int height);

        bool initOpenGL();

        bool mIsRunning = false;

        opengl::Window* mWindow = nullptr;

        opengl::Camera camera;
        unsigned int windowWidth = 512;
        unsigned int windowHeight = 512;
        std::unique_ptr<std::thread> mThread;


	};
} // nap

RTTI_DECLARE(nap::RenderService)

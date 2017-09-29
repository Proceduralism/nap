// main.cpp : Defines the entry point for the console application.
//
// Local Includes
#include "testrunner.h"

// Nap includes
#include <nap/core.h>

// predefines
void runGame(nap::Core& core, nap::TestRunner* testRunner);

// Main loop
int main(int argc, char *argv[])
{
	// Create core
	nap::Core core;
    
    nap::TestRunner* testRunner = new nap::TestRunner();
    
	// Initialize render stuff
	if (!testRunner->init(core))
		return -1;

	// Run Gam
	runGame(core, testRunner);

	return 0;
}

void runGame(nap::Core& core, nap::TestRunner* testRunner)
{
	// Run function
	bool loop = true;


	// Loop
	while (loop)
	{
		opengl::Event event;
		if (opengl::pollEvent(event))
		{
			// Check if we are dealing with an input event (mouse / keyboard)
			if (nap::isInputEvent(event))
			{
				nap::InputEventPtr input_event = nap::translateInputEvent(event);

				// If we pressed escape, quit the loop
				if (input_event->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
				{
					nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(input_event.get());
					if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
						loop = false;

					if (press_event->mKey == nap::EKeyCode::KEY_f)
					{
						static bool fullscreen = true;
                        testRunner->setWindowFullscreen("Viewport", fullscreen);
						fullscreen = !fullscreen;
					}
				}

				// Add event to input service for further processing
                testRunner->addInputEvent(std::move(input_event));
			}

			// Check if we're dealing with a window event
			else if (nap::isWindowEvent(event))
			{
				nap::WindowEventPtr nap_event = nap::translateWindowEvent(event);
                testRunner->addWindowEvent(std::move(nap_event));
			}
		}

		//////////////////////////////////////////////////////////////////////////

		// Update
		testRunner->onUpdate();

		// Render
		testRunner->onRender();
	}
    
    // Shutdown
	testRunner->shutdown();
    
    // Delete TestRunner now so that its entities etc are cleaned up before ObjectPtrManager destruction
    delete testRunner;
}
       
 

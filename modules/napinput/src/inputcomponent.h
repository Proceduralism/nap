#pragma once

#include <rtti/rtti.h>
#include <nap/signalslot.h>
#include <component.h>
#include <inputevent.h>
#include <utility/dllexport.h>

namespace nap
{
	// Forward declares
	class InputService;

	/**
	 * Base class for all input components. The trigger function is called when an InputRouter-derived class
	 * decides to route the input to this specific component.
	 */
	class NAPAPI InputComponentInstance : public ComponentInstance
	{
		RTTI_ENABLE(ComponentInstance)
	public:
		InputComponentInstance(EntityInstance& entity, Component& resource) :
			ComponentInstance(entity, resource)
		{
		}

		/**
		 * This function is called by an InputRouter derived class if it decides to route the input to this component.
		 * Implement this function in a derived class to handle input. 
		 * @param inEvent An InputEvent-derived class. Use RTTI queries to test the type of event and to retrieve data from it.
		 */
		virtual void trigger(const nap::InputEvent& inEvent) = 0;
	};


	/**
	 * The resource class for the InputComponent.
	 */
	class InputComponent : public Component
	{
		RTTI_ENABLE(Component)
		DECLARE_COMPONENT(InputComponent, InputComponentInstance)
	};



	/**
	 * Input component for press/release key events.
	 * Register to the various signals to receive keyboard events
	 */
	class NAPAPI KeyInputComponentInstance : public InputComponentInstance
	{
		friend class InputService;
		RTTI_ENABLE(InputComponentInstance)
	
	public:
		KeyInputComponentInstance(EntityInstance& entity, Component& resource) :
			InputComponentInstance(entity, resource)
		{
		}

		// Signals
		Signal<const KeyPressEvent&>		pressed;		///< Signal emitted when a key is pressed
		Signal<const KeyReleaseEvent&>		released;		///< Signal emitted when a key is released

	protected:
		virtual void trigger(const nap::InputEvent& inEvent) override;
	};


	/**
 	 * Resource class for KeyInputComponent.
	 */
	class NAPAPI KeyInputComponent : public InputComponent
	{
		RTTI_ENABLE(InputComponent)
		DECLARE_COMPONENT(KeyInputComponent, KeyInputComponentInstance)
	};


	/**
	 * Input component for mouse/touch events.
	 * Register to the various signals to receive mouse events
	 */
	class NAPAPI PointerInputComponentInstance : public InputComponentInstance
	{
		friend class InputService;
		RTTI_ENABLE(InputComponentInstance)

	public:
		PointerInputComponentInstance(EntityInstance& entity, Component& resource) :
			InputComponentInstance(entity, resource)
		{
		}

		Signal<const PointerPressEvent&>	pressed;		///< Signal emitted when the input component receives a click
		Signal<const PointerReleaseEvent&>	released;		///< Signal emitted when the click is released
		Signal<const PointerMoveEvent&>		moved;			///< Signal emitted when this component receives a mouse move

	protected:
		virtual void trigger(const nap::InputEvent& inEvent) override;
	};


	/**
	 * Resource class for PointerInputComponent.
	 */
	class NAPAPI PointerInputComponent : public InputComponent
	{
		RTTI_ENABLE(InputComponent)
		DECLARE_COMPONENT(PointerInputComponent, PointerInputComponentInstance)
	};

}
#pragma once

#include <rtti/rtti.h>
#include <nap/signalslot.h>
#include <nap/component.h>
#include <inputevent.h>
#include <nap/dllexport.h>

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
		InputComponentInstance(EntityInstance& entity) :
			ComponentInstance(entity)
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
	public:
		virtual const rtti::TypeInfo getInstanceType() const { return RTTI_OF(InputComponentInstance); }
	};



	/**
	 * Input component for press/release key events.
	 */
	class NAPAPI KeyInputComponentInstance : public InputComponentInstance
	{
		friend class InputService;
		RTTI_ENABLE(InputComponentInstance)
	
	public:
		KeyInputComponentInstance(EntityInstance& entity) :
			InputComponentInstance(entity)
		{
		}

		// Signals
		Signal<const KeyPressEvent&>		pressed;		//< If the key has been pressed
		Signal<const KeyReleaseEvent&>		released;		//< If the key has been released

	protected:
		virtual void trigger(const nap::InputEvent& inEvent) override;
	};


	/**
 	 * Resource class for KeyInputComponent.
	 */
	class NAPAPI KeyInputComponent : public InputComponent
	{
		RTTI_ENABLE(InputComponent)

	public:
		virtual const rtti::TypeInfo getInstanceType() const { return RTTI_OF(KeyInputComponentInstance); }
	};


	/**
	 * Input component for mouse/touch events.
	 */
	class NAPAPI PointerInputComponentInstance : public InputComponentInstance
	{
		friend class InputService;
		RTTI_ENABLE(InputComponentInstance)

	public:
		PointerInputComponentInstance(EntityInstance& entity) :
			InputComponentInstance(entity)
		{
		}

		Signal<const PointerPressEvent&>	pressed;		//< If the input component was clicked
		Signal<const PointerReleaseEvent&>	released;		//< If the input component click has been released
		Signal<const PointerDragEvent&>		dragged;		//< If the component received a drag (mousedrag)
		Signal<const PointerMoveEvent&>		moved;			//< If the component received a move (mousemove)

	protected:
		virtual void trigger(const nap::InputEvent& inEvent) override;
	};


	/**
	 * Resource class for PointerInputComponent.
	 */
	class NAPAPI PointerInputComponent : public InputComponent
	{
		RTTI_ENABLE(InputComponent)

	public:
		virtual const rtti::TypeInfo getInstanceType() const { return RTTI_OF(PointerInputComponentInstance); }
	};

}
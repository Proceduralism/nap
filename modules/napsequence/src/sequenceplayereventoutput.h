#pragma once

// local includes
#include "sequenceplayeroutput.h"
#include "sequenceevent.h"

// nap includes
#include <nap/resourceptr.h>
#include <nap/signalslot.h>

// external includes
#include <queue>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	// forward declares
	class SequenceService;

	/**
	 * SequencePlayerEventOutput is used to link an SequenceEventReceiver to a SequenceEventTrack
	 */
	class NAPAPI SequencePlayerEventOutput : public SequencePlayerOutput
	{
		friend class SequencePlayerEventAdapter;

		RTTI_ENABLE(SequencePlayerOutput);
	public:
		/**
		 * Constructor
		 * @param service reference to SequenceService
		 */
		SequencePlayerEventOutput(SequenceService& service);

	public:
		/**
		 * Signal will be triggered from main thread
		 */
		nap::Signal<const SequenceEventBase&> mSignal;
	protected:
		/**
		 * called from sequence service main thread
		 * @param deltaTime time since last update
		 */
		virtual void update(double deltaTime) override ;

		/**
		 * called from sequence player thread, adds event to queue
		 * @param the event ptr that needs to be dispatched. Receiver takes ownership of the event
		 */
		void addEvent(SequenceEventPtr event);
	private:
		// the queue of events
		std::queue<SequenceEventPtr> mEvents;

		// thread mutex
		std::mutex mEventMutex;
	};

	using SequencePlayerEventInputObjectCreator = rtti::ObjectCreator<SequencePlayerEventOutput, SequenceService>;
}
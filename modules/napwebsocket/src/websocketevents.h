#pragma once

// Local Includes
#include "websocketmessage.h"

// External Includes
#include <nap/event.h>

namespace nap
{
	/**
	 * Base class of all web socket related events
	 */
	class NAPAPI WebSocketEvent : public Event
	{
		RTTI_ENABLE(Event)
	};


	/**
	 *  Base class for all web-socket connection related events
	 */
	class NAPAPI WebSocketConnectionEvent : public WebSocketEvent
	{
		RTTI_ENABLE(WebSocketEvent)
	public:
		WebSocketConnectionEvent(WebSocketConnection connection) :
			mConnection(connection) { }
	private:
		WebSocketConnection mConnection;
	};


	/**
	 * Occurs when a connection is closed
	 */
	class NAPAPI WebSocketConnectionClosedEvent : public WebSocketConnectionEvent
	{
		RTTI_ENABLE(WebSocketConnectionEvent)
	public:
		WebSocketConnectionClosedEvent(WebSocketConnection connection) :
			WebSocketConnectionEvent(connection) { }
	};


	/**
	 * Occurs when a connection is opened
	 */
	class NAPAPI WebSocketConnectionOpenedEvent : public WebSocketConnectionEvent
	{
		RTTI_ENABLE(WebSocketConnectionEvent)
	public:
		WebSocketConnectionOpenedEvent(WebSocketConnection connection) :
			WebSocketConnectionEvent(connection) { }
	};


	/**
	 * Occurs when a connection failed to open
	 */
	class NAPAPI WebSocketConnectionFailedEvent : public WebSocketConnectionEvent
	{
		RTTI_ENABLE(WebSocketConnectionEvent)
	public:
		WebSocketConnectionFailedEvent(WebSocketConnection connection) :
			WebSocketConnectionEvent(connection) { }
	};


	/**
	 * A received web-socket message
	 */
	class NAPAPI WebSocketMessageReceivedEvent : public WebSocketEvent
	{
		RTTI_ENABLE(WebSocketEvent)
	public:
		WebSocketMessageReceivedEvent(WebSocketMessage message) : mMessage(message) { }
	private:
		WebSocketMessage mMessage;
	};

	using WebSocketEventPtr = std::unique_ptr<nap::WebSocketEvent>;
}

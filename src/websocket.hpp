/*
 * websocket.hpp
 *
 *  Created on: 28 авг. 2023 г.
 *      Author: vevdokimov
 */

#ifndef WEBSOCKET_HPP_
#define WEBSOCKET_HPP_

#include <iostream>

#include <drogon/drogon.h>

#include <drogon/WebSocketController.h>
#include <drogon/PubSubService.h>
#include <drogon/HttpAppFramework.h>

using namespace drogon;

class WebSocketChat : public drogon::WebSocketController<WebSocketChat>
{
	public:
		virtual void handleNewMessage(const WebSocketConnectionPtr &,
			std::string &&, const WebSocketMessageType &) override;
		virtual void handleConnectionClosed(const WebSocketConnectionPtr &) override;
		virtual void handleNewConnection(const HttpRequestPtr &, const WebSocketConnectionPtr &) override;
		WS_PATH_LIST_BEGIN
		WS_PATH_ADD("/chat", Get);
		WS_PATH_LIST_END
	private:
		PubSubService<std::string> chatRooms_;
};

struct Subscriber
{
    std::string chatRoomName_;
    drogon::SubscriberID id_;
};

#endif /* WEBSOCKET_HPP_ */

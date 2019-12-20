/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "MainSceneClient.h"
#include "InputMemoryStream.h"

#include <set>

USING_NS_CC;


MainSceneClient::~MainSceneClient()
{
	ShutdownClient();
}

Scene* MainSceneClient::createScene()
{
    return MainSceneClient::create();
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

// on "init" you need to initialize your instance
bool MainSceneClient::init()
{
	//////////////////////////////
	// 1. super init first
	if (!Scene::init())
	{
		return false;
	}

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	// add a "close" icon to exit the progress. it's an autorelease object
	auto closeItem = MenuItemImage::create("CloseNormal.png", "CloseSelected.png", CC_CALLBACK_1(MainSceneClient::menuCloseCallback, this));
	if (closeItem != nullptr && closeItem->getContentSize().width > 0 && closeItem->getContentSize().height > 0)
	{
		float x = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
		float y = origin.y + closeItem->getContentSize().height / 2;
		closeItem->setPosition(Vec2(x, y));
	}

	// create menu, it's an autorelease object
	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);

	// add a label shows "Hello World"
	auto label = Label::createWithTTF("Hello World", "fonts/Marker Felt.ttf", 24);
	if (label != nullptr)
	{
		label->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height - label->getContentSize().height));
		this->addChild(label, 1);
	}

	// add "HelloWorld" splash screen"
	sprite = Sprite::create("HelloWorld.png");
	if (sprite != nullptr)
	{
		sprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
		this->addChild(sprite, 0);
	}

	clientIDLabel = Label::createWithTTF("", "fonts/Marker Felt.ttf", 24);
	clientIDLabel->setPosition(Vec2(origin.x, origin.y + visibleSize.height - clientIDLabel->getHeight() - 30));
	clientIDLabel->setAnchorPoint({ 0, 0 });
	this->addChild(clientIDLabel, 1);

	// init keyboard
	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyPressed = CC_CALLBACK_2(MainSceneClient::OnKeyPressed, this);
	keyboardListener->onKeyReleased = CC_CALLBACK_2(MainSceneClient::OnKeyReleased, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

	// init client
	if (!InitClient())
	{
		return false;
	}

	this->scheduleUpdate();

    return true;
}


void MainSceneClient::menuCloseCallback(Ref* pSender)
{
    //Close the cocos2d-x game scene and quit the application
    Director::getInstance()->end();
}

void MainSceneClient::update(float delta)
{
	time += delta;

	Vec2 direction = { 0.0f, 0.0f };
	if (inputState[InputAction::Input_Up])
	{
		direction.y += 1;
	}
	if (inputState[InputAction::Input_Down])
	{
		direction.y -= 1;
	}
	if (inputState[InputAction::Input_Left])
	{
		direction.x -= 1;
	}
	if (inputState[InputAction::Input_Right])
	{
		direction.x += 1;
	}

	sprite->setPosition(sprite->getPosition() + direction * velocity * delta);


	SendState();
	ListenNet();

	//CCLOG("update: %f\n", time);
	float elapsedTime = time - lastTickTime;
	CCLOG("elapsed: %f\n", elapsedTime);

	for (auto& netSprite : netSprites)
	{
		if (netSprite.second.clientID == clientID)
		{
			continue;
		}

		//Vec2 position = netSprite.second.position1 + netSprite.second.velocity * elapsedTime;
		//netSprite.second.sprite->setPosition(position);

		if (!netSprite.second.finished && elapsedTime < (netSprite.second.timeDelta))
		{
			Vec2 position = netSprite.second.position1 + netSprite.second.velocity * elapsedTime;
			netSprite.second.sprite->setPosition(position);
		}
		else
		{
			netSprite.second.sprite->setPosition(netSprite.second.position2);
			netSprite.second.finished = true;
		}

	}


}

bool MainSceneClient::InitClient()
{
	const char* host = "localhost";
	const enet_uint16 port = 5001;
	const size_t peerCount = 1;
	const size_t channelLimit = 2;
	const enet_uint32 incomingBandwidth = 0;
	const enet_uint32 outgoingBandwidth = 0;


	if (enet_initialize() != 0)
	{
		CCLOG("An error occurred while initializing ENet.\n");
		return false;
	}

	client = enet_host_create(NULL, peerCount, channelLimit, incomingBandwidth, outgoingBandwidth);
	if (client == NULL)
	{
		CCLOG("An error occurred while trying to create an ENet client host.\n");
		return false;
	}

	if (!ConnectClient(host, port))
	{
		return false;
	}

	return true;
}

bool MainSceneClient::ConnectClient(const char* host, const enet_uint16 port)
{
	ENetAddress address;
	enet_address_set_host(&address, host);
	address.port = port;

	server = enet_host_connect(client, &address, 2, 0);
	if (server == NULL)
	{
		CCLOG("Connection initiation to %x failed.\n", address.host);
		return false;
	}

	ENetEvent event;
	if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
	{
		CCLOG("Connection to server %x succeeded.\n", address.host);

		if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE)
		{
			clientID = *reinterpret_cast<unsigned int*>(event.packet->data);
			CCLOG("Received id #%u from server.\n", clientID);
			char buf[16];
			itoa(clientID, buf, 10);
			clientIDLabel->setString(std::string(buf));
		}
	}
	else
	{
		enet_peer_reset(server);
		CCLOG("Connection to %x failed.\n", address.host);
		return false;
	}

	return true;
}

void MainSceneClient::ShutdownClient()
{
	enet_peer_disconnect(server, 0);
	ENetEvent event;
	while (enet_host_service(client, &event, 1000) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			CCLOG("Disconnection succeeded.");
			return;
		}
	}
	enet_peer_reset(server);

	enet_deinitialize();
}

void MainSceneClient::SendState()
{
	ENetPacket* packet;
	packet = enet_packet_create(inputState, sizeof(inputState), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(server, 0, packet);
}

void MainSceneClient::ListenNet()
{
	ENetEvent  event;
	while (enet_host_service(client, &event, 5) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:
		{
			CCLOG("data received\n");

			InputMemoryStream stream(reinterpret_cast<char*>(event.packet->data), event.packet->dataLength);
			uint32_t size;
			std::set<uint32_t> serverSprites;

			stream.Read(size);
			for (int i = 0; i < size; i++)
			{
				uint32_t clientID;
				Vec2 position;

				stream.Read(clientID);
				stream.Read(&position, sizeof(position));
				
				// don't change our own sprite
				if (clientID == this->clientID)
				{
					bool keybStateTemp[InputAction::Input_Count];
					//stream.Read(keybStateTemp, sizeof(keybStateTemp));
					continue;
				}

				// if we don't have this client - add it to us
				auto it = netSprites.find(clientID);
				if (it == netSprites.end())
				{
					auto visibleSize = Director::getInstance()->getVisibleSize();
					Vec2 origin = Director::getInstance()->getVisibleOrigin();

					NetSprite2 netSprite;
					netSprite.clientID = clientID;
					netSprite.sprite = Sprite::create("HelloWorld.png");
					netSprite.sprite->setPosition(position);
					netSprite.position1 = position;
					netSprite.position2 = position;
					netSprite.velocity = Vec2::ZERO;
					netSprite.timeDelta = 0.0f;
					netSprites[clientID] = netSprite;
					this->addChild(netSprite.sprite, 0);
				}

				// update client position
				NetSprite2& netSprite = netSprites[clientID];
				netSprite.position1 = netSprite.position2;
				netSprite.position2 = position;
				netSprite.timeDelta = time - lastTickTime;
				netSprite.velocity = (netSprite.position2 - netSprite.position1) / netSprite.timeDelta;
				netSprite.finished = false;
				netSprite.sprite->setPosition(netSprite.position1);

				serverSprites.insert(clientID);
			}

			// remove absent on server sprites from our scene
			for (auto it = netSprites.cbegin(); it != netSprites.cend(); )
			{
				if (serverSprites.find(it->second.clientID) == serverSprites.cend())
				{
					this->removeChild(it->second.sprite);
					it = netSprites.erase(it);
				}
				else
				{
					++it;
				}
			}

			
			lastTickTime = time;
			CCLOG("receive: %f\n", lastTickTime);

			enet_packet_destroy(event.packet);
		}
		break;

		case ENET_EVENT_TYPE_DISCONNECT:
			CCLOG("You have been disconnected.\n");
			return;
		}
	}
}

void MainSceneClient::OnKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event * event)
{
	switch (keyCode)
	{
	case EventKeyboard::KeyCode::KEY_W:
	case EventKeyboard::KeyCode::KEY_UP_ARROW:
		inputState[InputAction::Input_Up] = true;
		break;
	case EventKeyboard::KeyCode::KEY_S:
	case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
		inputState[InputAction::Input_Down] = true;
		break;
	case EventKeyboard::KeyCode::KEY_A:
	case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
		inputState[InputAction::Input_Left] = true;
		break;
	case EventKeyboard::KeyCode::KEY_D:
	case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
		inputState[InputAction::Input_Right] = true;
		break;
	default:
		break;
	}
}

void MainSceneClient::OnKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event * event)
{
	switch (keyCode)
	{
	case EventKeyboard::KeyCode::KEY_W:
	case EventKeyboard::KeyCode::KEY_UP_ARROW:
		inputState[InputAction::Input_Up] = false;
		break;
	case EventKeyboard::KeyCode::KEY_S:
	case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
		inputState[InputAction::Input_Down] = false;
		break;
	case EventKeyboard::KeyCode::KEY_A:
	case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
		inputState[InputAction::Input_Left] = false;
		break;
	case EventKeyboard::KeyCode::KEY_D:
	case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
		inputState[InputAction::Input_Right] = false;
		break;
	default:
		break;
	}
}

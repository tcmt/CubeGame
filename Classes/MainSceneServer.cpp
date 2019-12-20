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

#include "MainSceneServer.h"
#include "OutputMemoryStream.h"

USING_NS_CC;

MainSceneServer::~MainSceneServer()
{
	enet_host_destroy(server);
	atexit(enet_deinitialize);
}

Scene* MainSceneServer::createScene()
{
    return MainSceneServer::create();
}

// on "init" you need to initialize your instance
bool MainSceneServer::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create("CloseNormal.png", "CloseSelected.png",  CC_CALLBACK_1(MainSceneServer::menuCloseCallback, this));
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

	// init server
	connectionsCounter = 100;

	if (!InitServer())
	{
		return false;
	}
	
	this->scheduleUpdate();

    return true;
}


void MainSceneServer::menuCloseCallback(Ref* pSender)
{
    //Close the cocos2d-x game scene and quit the application
    Director::getInstance()->end();
}

void MainSceneServer::update(float delta)
{
	ListenNet();

	for (auto& netSprite : netSprites)
	{
		Vec2 direction = { 0.0f, 0.0f };
		if (netSprite.second.inputState[InputAction::Input_Right])
		{
			direction.x += 1;
		}
		if (netSprite.second.inputState[InputAction::Input_Left])
		{
			direction.x -= 1;
		}
		if (netSprite.second.inputState[InputAction::Input_Up])
		{
			direction.y += 1;
		}
		if (netSprite.second.inputState[InputAction::Input_Down])
		{
			direction.y -= 1;
		}

		netSprite.second.sprite->setPosition(netSprite.second.sprite->getPosition() + direction * velocity * delta);
	}

	gameTime += delta;
	if (std::abs(gameTime - gameTimeSinceLastTick) > TickRate)
	{
		//Broadcast new world state 
		BroadcastState();

		gameTimeSinceLastTick = gameTime;
	}
}

void MainSceneServer::ListenNet()
{
	ENetEvent event;
	while (enet_host_service(server, &event, 1) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			{
				CCLOG("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);

				enet_uint32 clientID = connectionsCounter;
				connectionsCounter++;

				event.peer->data = reinterpret_cast<void*>(clientID);
				
				ENetPacket* packet;
				packet = enet_packet_create(&clientID, sizeof(clientID), ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(event.peer, 0, packet);
				enet_host_flush(server);

				auto visibleSize = Director::getInstance()->getVisibleSize();
				Vec2 origin = Director::getInstance()->getVisibleOrigin();

				NetSprite netSprite;
				netSprite.clientID = clientID;
				netSprite.sprite = Sprite::create("HelloWorld.png");
				netSprite.sprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
				netSprites[clientID] = netSprite;
				this->addChild(netSprite.sprite, 0);
			}
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			{
				// CCLOG("A packet of length %u was received from %x on channel %u.\n", event.packet->dataLength, event.peer->address.host, event.channelID);

				enet_uint32 clientID = reinterpret_cast<enet_uint32>(event.peer->data);

				NetSprite& netSprite = netSprites.at(clientID);
				for (int i = 0; i < event.packet->dataLength; i++)
				{
					netSprite.inputState[i] = (bool)event.packet->data[i];
				}

				enet_packet_destroy(event.packet);
			}
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			CCLOG("%x disconnected.\n", event.peer->address.host);

			enet_uint32 clientID = reinterpret_cast<enet_uint32>(event.peer->data);
			this->removeChild(netSprites.at(clientID).sprite);
			netSprites.erase(clientID);
			event.peer->data = nullptr;
			break;
		}
	}
}

void MainSceneServer::BroadcastState()
{
	OutputMemoryStream stream;
	stream.Write(netSprites.size());
	for (auto& netSprite : netSprites)
	{
		stream.Write(netSprite.second.clientID);
		auto position = netSprite.second.sprite->getPosition();
		stream.Write(&position, sizeof(position));
	}

	ENetPacket* packet;
	packet = enet_packet_create(stream.GetBufferPtr(), stream.GetLength(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(server, 0, packet);
	//enet_host_flush(server);
}

bool MainSceneServer::InitServer()
{
	const enet_uint16 port = 5001;
	const size_t peerCount = 32;
	const size_t channelLimit = 2;
	const enet_uint32 incomingBandwidth = 0;
	const enet_uint32 outgoingBandwidth = 0;

	if (enet_initialize() != 0)
	{
		CCLOG("An error occurred while initializing ENet.\n");
		return false;
	}

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;
	server = enet_host_create(&address, peerCount, channelLimit, incomingBandwidth, outgoingBandwidth);
	if (server == NULL)
	{
		CCLOG("An error occurred while trying to create an ENet server host.\n");
		return false;
	}

	return true;
}
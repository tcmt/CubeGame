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

#ifndef __MAIN_SCENE_SERVER_H__
#define __MAIN_SCENE_SERVER_H__

#include "cocos2d.h"
#include <map>
#include <enet/enet.h>
#include "NetData.h"



class MainSceneServer : public cocos2d::Scene
{
public:

	~MainSceneServer();
    static cocos2d::Scene* createScene();

    virtual bool init();
    
    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);
    
    // implement the "static create()" method manually
    CREATE_FUNC(MainSceneServer);

	void update(float) override;

private:
	static constexpr float TickRate = 1.0f / 10.0f;

	bool InitServer();
	void ListenNet();
	void BroadcastState();

	enet_uint32 connectionsCounter;
	ENetHost* server;
	bool inputState[InputAction::Input_Count];
	std::map<enet_uint32, NetSprite> netSprites;
	float velocity = 400.0f;

	float gameTime{ 0.0f };
	float gameTimeSinceLastTick{ 0.0f };
};

#endif // __MAIN_SCENE_SERVER_H__

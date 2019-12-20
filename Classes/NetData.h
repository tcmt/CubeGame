#pragma once
#include "cocos2d.h"

enum InputAction : unsigned char
{
	Input_Up,
	Input_Down,
	Input_Left,
	Input_Right,
	Input_Count
};

struct NetSprite
{
	unsigned int clientID;
	bool inputState[InputAction::Input_Count];
	cocos2d::Sprite* sprite;
};

struct NetMessage
{
	unsigned int clientID;
	float timeDelta;
	bool input[InputAction::Input_Count];
};

struct NetSprite2
{
	uint32_t clientID;
	cocos2d::Sprite* sprite;
	cocos2d::Vec2 position1;
	cocos2d::Vec2 position2;
	cocos2d::Vec2 velocity;
	float timeDelta;
	bool finished;
};
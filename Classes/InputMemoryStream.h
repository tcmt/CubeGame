#pragma once
#include <stdint.h>


class InputMemoryStream
{
public:
	InputMemoryStream(char* inBuffer, uint32_t inByteCount);
	~InputMemoryStream();

	uint32_t GetRemainingDataSize() const;
	void Read(void* outData, uint32_t inByteCount);
	void Read(uint32_t& outData);
	void Read(int32_t& outData);

private:
	char* mBuffer;
	uint32_t mHead;
	uint32_t mCapacity;
};


#include "InputMemoryStream.h"
#include <cstdlib>
#include <memory>

InputMemoryStream::InputMemoryStream(char* inBuffer, uint32_t inByteCount) :
	mCapacity(inByteCount), mHead(0), mBuffer(inBuffer)
{}


InputMemoryStream::~InputMemoryStream()
{
	//std::free(mBuffer);
}

uint32_t InputMemoryStream::GetRemainingDataSize() const 
{
	return mCapacity - mHead;
}

void InputMemoryStream::Read(void* outData, uint32_t inByteCount)
{
	std::memcpy(outData, mBuffer + mHead, inByteCount);

	mHead += inByteCount;
}
void InputMemoryStream::Read(uint32_t& outData)
{
	Read(&outData, sizeof(outData));
}

void InputMemoryStream::Read(int32_t& outData)
{
	Read(&outData, sizeof(outData));
}

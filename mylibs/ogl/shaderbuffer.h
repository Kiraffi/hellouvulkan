#pragma once

class ShaderBuffer
{
public:
	ShaderBuffer(unsigned int bufferType, unsigned int size, unsigned int usasge, 
		void *dataPtr = nullptr, bool immutable = false);
	~ShaderBuffer();

	void updateBuffer(unsigned int offset, unsigned int size, void *dataPtr);
	void bind(unsigned int slot);
	void unbind();

public:
	unsigned int handle = 0;
	unsigned int bufferType = 0;
	unsigned int size = 0;
	unsigned int usage = 0;

	unsigned int boundSlot = 0u;
	bool immutable = false;
};

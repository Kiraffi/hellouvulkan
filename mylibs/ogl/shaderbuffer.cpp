#include "shaderbuffer.h"

#include "../../external/glad/glad.h"

#include <cassert>

ShaderBuffer::ShaderBuffer(unsigned int bufferType, unsigned int size, unsigned int usage, void *dataPtr,
	bool immutable)
{
	assert(size > 0 && "Buffer size must be greater than 0 bytes");
	if(bufferType != GL_ELEMENT_ARRAY_BUFFER)
		assert(size % 16 == 0 && "Buffer size should be 16 byte size multiplier.");

	switch(bufferType)
	{
		case GL_ARRAY_BUFFER:
		case GL_ATOMIC_COUNTER_BUFFER:
		case GL_COPY_READ_BUFFER:
		case GL_COPY_WRITE_BUFFER:
		case GL_DISPATCH_INDIRECT_BUFFER:
		case GL_DRAW_INDIRECT_BUFFER:
		case GL_ELEMENT_ARRAY_BUFFER:
		case GL_PIXEL_PACK_BUFFER:
		case GL_PIXEL_UNPACK_BUFFER:
		case GL_QUERY_BUFFER:
		case GL_SHADER_STORAGE_BUFFER:
		case GL_TEXTURE_BUFFER:
		case GL_TRANSFORM_FEEDBACK_BUFFER:
		case GL_UNIFORM_BUFFER:
		{

		}
		break;

		default:
			assert(0 && "Unknown buffer type!");
	}

	switch(usage)
	{
		case GL_STREAM_DRAW:
		case GL_STREAM_COPY:
		case GL_STREAM_READ:
		case GL_STATIC_DRAW:
		case GL_STATIC_COPY:
		case GL_STATIC_READ:
		case GL_DYNAMIC_DRAW:
		case GL_DYNAMIC_READ:
		case GL_DYNAMIC_COPY:
		{

		}
		break;

		default:
			if(!immutable)
				assert(0 && "Unknown buffer usage!");
	}

	this->bufferType = bufferType;
	this->usage = usage;
	this->size = size;

	glCreateBuffers(1, &handle);
	this->handle = handle;
	this->immutable = immutable;
	assert(handle && "Failed to generate buffer handle!");
	if(immutable)
		glNamedBufferStorage(handle, size, dataPtr, usage);
	else
		glNamedBufferData(handle, size, dataPtr, usage);
}

ShaderBuffer::~ShaderBuffer()
{
	glDeleteBuffers(1, &handle);
	handle = 0;

}

void ShaderBuffer::updateBuffer(unsigned int offset, unsigned int size, void *dataPtr)
{
	assert(offset + size <= this->size && "trying to write buffer out of range!");
	assert(dataPtr != nullptr && "No data given to update");

	glNamedBufferSubData(handle, offset, size, dataPtr);
	
}

void ShaderBuffer::bind(unsigned int slot)
{
	glBindBufferBase(bufferType, slot, handle);
	boundSlot = slot;
}

void ShaderBuffer::unbind()
{
	glBindBufferBase(bufferType, boundSlot, 0);
}
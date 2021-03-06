#pragma once
#include "sic/core/serializer.h"

void sic::Serialize_stream::write_raw(const void* in_buffer, ui32 in_buffer_bytesize)
{
	serialize_raw(in_buffer, in_buffer_bytesize, *this);
}

void sic::Deserialize_stream::read_raw(void*& out_buffer, ui32& out_buffer_bytesize)
{
	deserialize_raw(*this, out_buffer, out_buffer_bytesize);
}

void sic::serialize_raw(const void* in_buffer, ui32 in_buffer_bytesize, Serialize_stream& out_stream)
{
	serialize(in_buffer_bytesize, out_stream);

	if (in_buffer_bytesize == 0)
		return;

	const size_t old_size = out_stream.m_bytes.size();
	out_stream.m_bytes.resize(old_size + in_buffer_bytesize);
	memcpy_s(&out_stream.m_bytes[old_size], in_buffer_bytesize, in_buffer, in_buffer_bytesize);
}

void sic::deserialize_raw(Deserialize_stream& in_stream, void*& out_buffer, ui32& out_buffer_bytesize)
{
	deserialize(in_stream, out_buffer_bytesize);
	if (out_buffer_bytesize == 0)
		return;

	out_buffer = new unsigned char[out_buffer_bytesize];

	memcpy_s(out_buffer, out_buffer_bytesize, &(in_stream.m_bytes[in_stream.m_offset]), out_buffer_bytesize);
	in_stream.m_offset += out_buffer_bytesize;
}
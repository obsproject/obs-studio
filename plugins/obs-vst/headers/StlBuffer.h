#pragma once

#include <string>
#include <vector>

namespace StlBuffer
{
	static void pop_buffer(const std::string& buffer, size_t& buffer_readIncrementer, char* writeDst, const size_t amount)
	{
		if (amount + buffer_readIncrementer > buffer.size())
			return;

		memcpy(writeDst, buffer.data() + buffer_readIncrementer, amount);
		buffer_readIncrementer += amount;
	}
};

#pragma once
#include "defines.h"
#include "system.h"
#include "world.h"

#include <memory>
#include <unordered_map>
#include <functional>

namespace impuls
{
	namespace file_management
	{
		std::string load_file(const std::string& in_filepath, bool in_binary = true);
	}
}
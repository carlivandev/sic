#pragma once
#include "defines.h"
#include "system.h"
#include "engine.h"

#include <memory>
#include <unordered_map>
#include <functional>

namespace impuls
{
	namespace file_management
	{
		std::string load_file(const std::string& in_filepath, bool in_binary = true);
		void save_file(const std::string& in_filepath, const std::string& in_filedata, bool in_binary = true);
	}
}
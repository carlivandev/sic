#pragma once
#include "sic/core/defines.h"
#include "sic/core/system.h"
#include "sic/core/engine.h"
#include "sic/core/temporary_string.h"

#include <memory>
#include <unordered_map>
#include <functional>

namespace sic
{
	namespace File_management
	{
		std::string load_file(const std::string& in_filepath, bool in_binary = true);
		Temporary_string load_file_to_temporary(const std::string& in_filepath, bool in_binary = true);
		void save_file(const std::string& in_filepath, const std::string& in_filedata, bool in_binary = true);
	}
}
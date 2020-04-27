#pragma once
#include "sic/defines.h"
#include "sic/system.h"
#include "sic/engine.h"
#include "sic/temporary_string.h"

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
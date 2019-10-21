#include "impuls/file_management.h"
#include "impuls/world.h"
#include "impuls/world_context.h"

#include <fstream>

std::string impuls::file_management::load_file(const std::string& in_filepath, bool in_binary)
{
	std::ifstream infile(in_filepath, in_binary ? std::ios_base::binary : std::ios_base::in);

	infile.seekg(0, std::ios_base::end);
	const size_t length = infile.tellg();
	infile.seekg(0, std::ios_base::beg);

	std::string str((std::istreambuf_iterator<char>(infile)),
		std::istreambuf_iterator<char>());

	return str;
}

void impuls::file_management::save_file(const std::string& in_filepath, const std::string& in_filedata, bool in_binary)
{
	std::ofstream outfile(in_filepath, in_binary ? std::ios_base::binary : std::ios_base::in);

	outfile.write(in_filedata.c_str(), in_filedata.size());
}

#pragma once
#include "sic/core/temporary_string.h"

#include <string>
#include <unordered_set>
#include <optional>

namespace sic
{
	struct Shader_parser
	{
		static std::optional<std::string> parse_shader(Temporary_string&& in_shader_path);

	private:
		struct Shader_parse_result
		{
			std::string m_glsl_code;
			bool m_success = false;
		};

		static Shader_parse_result parse_shader_file_recursive(const Temporary_string& in_shader_path, Temporary_string&& in_shader_code, bool in_is_main, std::unordered_set<Temporary_string>& out_included_paths);
	};
}
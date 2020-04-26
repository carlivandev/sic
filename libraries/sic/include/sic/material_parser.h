#pragma once
#include <string>
#include <unordered_set>
#include <optional>

namespace sic
{
	struct Material_parser
	{
		static std::optional<std::string> parse_material(const std::string& in_shader_path);

	private:
		struct Material_parse_result
		{
			std::string m_glsl_code;
			bool m_success = false;
		};

		static Material_parse_result parse_material_file_recursive(const std::string& in_shader_path, std::string&& in_shader_code, bool in_is_main, std::unordered_set<std::string>& out_included_paths);
	};
}
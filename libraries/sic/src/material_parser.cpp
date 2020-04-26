#pragma once
#include "sic/material_parser.h"

#include "sic/file_management.h"
#include "sic/logger.h"

std::optional<std::string> sic::Material_parser::parse_material(const std::string& in_shader_path)
{
	std::unordered_set<std::string> included_files;
	Material_parse_result main_parse_result = parse_material_file_recursive(in_shader_path, sic::File_management::load_file(in_shader_path), true, included_files);

	if (!main_parse_result.m_success)
		return {};

	return main_parse_result.m_glsl_code;
}

sic::Material_parser::Material_parse_result sic::Material_parser::parse_material_file_recursive(const std::string& in_shader_path, std::string&& in_shader_code, bool in_is_main, std::unordered_set<std::string>& out_included_paths)
{
	Material_parse_result result;

	const std::string extract_always = "//!";
	const std::string extract_if_main = "//?";

	size_t pos = in_shader_code.find(extract_always, 0);

	while (pos != std::string::npos)
	{
		in_shader_code.erase(in_shader_code.begin() + pos, in_shader_code.begin() + pos + extract_always.size());
		pos = in_shader_code.find(extract_always, pos);
	}

	if (in_is_main)
	{
		pos = in_shader_code.find(extract_if_main, 0);

		while (pos != std::string::npos)
		{
			in_shader_code.erase(in_shader_code.begin() + pos, in_shader_code.begin() + pos + extract_if_main.size());
			pos = in_shader_code.find(extract_if_main, pos);
		}
	}

	size_t root_dir_end = in_shader_path.find_last_of('/');
	std::string root_include_directory = root_dir_end != std::string::npos ? in_shader_path.substr(0, root_dir_end + 1) : "";

	if (root_include_directory.empty())
	{
		SIC_LOG_E(sic::g_log_asset, "Failed to find root directory of shader with path: {0}", in_shader_path);
		return Material_parse_result();
	}

	const std::string include_prefix = "#include ";

	pos = in_shader_code.find(include_prefix, 0);

	while (pos != std::string::npos)
	{
		const size_t start_of_include_filename = in_shader_code.find('"', pos);
		if (start_of_include_filename != std::string::npos)
		{
			const size_t end_of_include_filename = in_shader_code.find('"', start_of_include_filename + 1);
			if (end_of_include_filename != std::string::npos)
			{
				const std::string include_path = root_include_directory + in_shader_code.substr(start_of_include_filename + 1, end_of_include_filename - start_of_include_filename - 1);
				const auto insertion_result = out_included_paths.insert(include_path);
				
				if (insertion_result.second)
				{
					Material_parse_result included_result = parse_material_file_recursive(include_path, sic::File_management::load_file(include_path), false, out_included_paths);

					if (!included_result.m_success)
						return Material_parse_result();

					result.m_glsl_code += in_shader_code.substr(0, pos);
					result.m_glsl_code += included_result.m_glsl_code;
				}

				in_shader_code.erase(in_shader_code.begin(), in_shader_code.begin() + end_of_include_filename + 1);
			}
		}

		pos = in_shader_code.find(include_prefix);
	}
	result.m_glsl_code += in_shader_code;
	result.m_success = true;

	return result;
}

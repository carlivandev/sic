#pragma once
#include "sic/material_parser.h"

#include "sic/file_management.h"
#include "sic/logger.h"

std::optional<std::string> sic::Material_parser::parse_material(Temporary_string&& in_shader_path)
{
	Scoped_temporary_storage_reset resetter;

	std::unordered_set<Temporary_string> included_files;
	Material_parse_result main_parse_result = parse_material_file_recursive(in_shader_path, sic::File_management::load_file_to_temporary(in_shader_path.get_c_str()), true, included_files);

	if (!main_parse_result.m_success)
		return {};

	return main_parse_result.m_glsl_code;
}

sic::Material_parser::Material_parse_result sic::Material_parser::parse_material_file_recursive(const Temporary_string& in_shader_path, Temporary_string&& in_shader_code, bool in_is_main, std::unordered_set<Temporary_string>& out_included_paths)
{
	Scoped_temporary_storage_reset resetter;

	Material_parse_result result;

	const Temporary_string extract_always = "//!";
	const Temporary_string extract_if_main = "//?";

	std::optional<size_t> pos = in_shader_code.find_string_from_start(extract_always.get_c_str());

	while (pos.has_value())
	{
		in_shader_code = in_shader_code.remove(pos.value(), extract_always.get_length());
		pos = in_shader_code.find_string_from_start(extract_always.get_c_str(), pos.value());
	}

	if (in_is_main)
	{
		pos = in_shader_code.find_string_from_start(extract_if_main.get_c_str());

		while (pos.has_value())
		{
			in_shader_code = in_shader_code.remove(pos.value(), extract_if_main.get_length());
			pos = in_shader_code.find_string_from_start(extract_if_main.get_c_str(), pos.value());
		}
	}

	size_t root_dir_end = std::string_view(in_shader_path.get_c_str(), in_shader_path.get_length()).find_last_of('/');
	Temporary_string root_include_directory = root_dir_end != std::string::npos ? in_shader_path.substring(0, root_dir_end + 1) : "";

	if (root_include_directory.get_is_empty())
	{
		SIC_LOG_E(sic::g_log_asset, "Failed to find root directory of shader with path: {0}", in_shader_path.get_c_str());
		return Material_parse_result();
	}

	const char* include_prefix = "#include ";

	pos = in_shader_code.find_string_from_start(include_prefix);

	while (pos.has_value())
	{
		const std::optional<size_t> start_of_include_filename = in_shader_code.find_char_from_start('"', pos.value());
		if (start_of_include_filename.has_value())
		{
			const std::optional<size_t> end_of_include_filename = in_shader_code.find_char_from_start('"', start_of_include_filename.value() + 1);
			if (end_of_include_filename.has_value())
			{
				const Temporary_string include_path = root_include_directory + in_shader_code.substring(start_of_include_filename.value() + 1, end_of_include_filename.value() - start_of_include_filename.value() - 1);
				const auto insertion_result = out_included_paths.insert(include_path);
				
				if (insertion_result.second)
				{
					Material_parse_result included_result = parse_material_file_recursive(include_path, sic::File_management::load_file_to_temporary(include_path.get_c_str()), false, out_included_paths);

					if (!included_result.m_success)
						return Material_parse_result();

					result.m_glsl_code += in_shader_code.substring(0, pos.value()).get_c_str();
					result.m_glsl_code += included_result.m_glsl_code;
				}

				in_shader_code = in_shader_code.remove(0, end_of_include_filename.value() + 1);
			}
		}

		pos = in_shader_code.find_string_from_start(include_prefix);
	}
	result.m_glsl_code += in_shader_code.get_c_str();
	result.m_success = true;

	return result;
}
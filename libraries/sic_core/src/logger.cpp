#pragma once
#include "sic/core/logger.h"

#include "fmt/format.h"

#include <chrono>
#include <time.h>
#include <stdio.h>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef WIN32
#include <Windows.h>
#endif

sic::Log sic::g_log_game("Game");
sic::Log sic::g_log_game_verbose("Game", false);

sic::Log sic::g_log_renderer("Renderer");
sic::Log sic::g_log_renderer_verbose("Renderer", false);

sic::Log sic::g_log_asset("Asset");
sic::Log sic::g_log_asset_verbose("Asset", false);

void sic::Logger::print(const Log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_normal, in_log.m_name, in_text, in_file);
}

void sic::Logger::print_warning(const Log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_warning, in_log.m_name, in_text, in_file);
}

void sic::Logger::print_error(const Log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_error, in_log.m_name, in_text, in_file);
}

void sic::Logger::print_validation(const Log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_validation, in_log.m_name, in_text, in_file);
}

void sic::Logger::print_raw(ui16 in_text_color, const std::string& in_log_name, const std::string& in_text, std::string in_file)
{
	std::scoped_lock lock(get_instance().m_print_mutex);

	auto n = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(n);
	std::tm buf;
	localtime_s(&buf, &in_time_t);

	std::stringstream time_stream;
	time_stream << std::put_time(&buf, "%X");

	size_t substr_idx = 0;
	size_t substr_backslash_idx = in_file.find_last_of("\\");
	size_t substr_forwardslash_idx = in_file.find_last_of("/");

	if (substr_backslash_idx == std::string::npos || (substr_forwardslash_idx != std::string::npos && substr_forwardslash_idx > substr_backslash_idx))
		substr_idx = substr_forwardslash_idx;
	else
		substr_idx = substr_backslash_idx;

	++substr_idx;

	const std::string to_print =
		fmt::format
		(
			"[{0}][{1}][{2}]: {3}\n",
			in_log_name.c_str(),
			time_stream.str().c_str(),
			in_file.empty() ? "" : in_file.substr(substr_idx).c_str(),
			in_text.c_str()
		);

#ifdef WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, in_text_color);
#endif
	printf(to_print.c_str());

#ifdef WIN32
	SetConsoleTextAttribute(hConsole, text_color_normal);
#endif
	//in future write to file
}

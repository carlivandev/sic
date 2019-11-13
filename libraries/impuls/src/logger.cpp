#pragma once
#include "impuls/logger.h"

#include "fmt/format.h"

#include <chrono>
#include <time.h>
#include <stdio.h>
#include <mutex>
#include <sstream>
#include <iomanip>

#ifdef WIN32
#include <Windows.h>
#endif

void impuls::logger::print(const log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_normal, in_log.m_name, in_text, in_file);
}

void impuls::logger::print_warning(const log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_warning, in_log.m_name, in_text, in_file);
}

void impuls::logger::print_error(const log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_error, in_log.m_name, in_text, in_file);
}

void impuls::logger::print_validation(const log& in_log, const std::string& in_text, std::string in_file)
{
	print_raw(text_color_validation, in_log.m_name, in_text, in_file);
}

void impuls::logger::print_raw(ui16 in_text_color, const std::string& in_log_name, const std::string& in_text, std::string in_file)
{
	std::scoped_lock lock(get_instance().m_print_mutex);

	auto n = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(n);
	std::tm buf;
	localtime_s(&buf, &in_time_t);

	std::stringstream time_stream;
	time_stream << std::put_time(&buf, "%X");

	const std::string to_print =
		fmt::format
		(
			"[{0}][{1}][{2}]: {3}\n",
			in_log_name.c_str(),
			time_stream.str().c_str(),
			in_file.empty() ? "" : in_file.substr(in_file.find_last_of("\\") + 1).c_str(),
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

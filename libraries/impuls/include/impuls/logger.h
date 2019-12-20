#pragma once
#include "impuls/defines.h"

#include "fmt/format.h"

#include <string>
#include <mutex>

namespace impuls
{
	struct Log
	{
		Log(const std::string& in_name) : m_name(in_name) {}
		std::string m_name;
		bool m_enabled = true;
	};

	extern Log g_log_game;
	extern Log g_log_game_verbose;

	extern Log g_log_renderer;
	extern Log g_log_renderer_verbose;

	extern Log g_log_asset;
	extern Log g_log_asset_verbose;

	class Logger
	{
	public:
		constexpr static ui16 text_color_normal = 7;
		constexpr static ui16 text_color_warning = 14;
		constexpr static ui16 text_color_error = 12;
		constexpr static ui16 text_color_validation = 11;

		static void print(const Log& in_log, const std::string& in_text, std::string in_file = "");
		static void print_warning(const Log& in_log, const std::string& in_text, std::string in_file = "");
		static void print_error(const Log& in_log, const std::string& in_text, std::string in_file = "");
		static void print_validation(const Log& in_log, const std::string& in_text, std::string in_file = "");

		static void print_raw(ui16 in_text_color, const std::string& in_log_name, const std::string& in_text, std::string in_file);

	private:
		static Logger & get_instance()
		{
			static Logger instance;
			return instance;
		}

		Logger() = default;
		~Logger() = default;
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		std::mutex m_print_mutex;
	};
}

#define IMPULS_LOG_INTERNAL(in_print_function, in_log_type, in_format, ...) in_print_function(in_log_type, fmt::format(in_format, __VA_ARGS__), std::move(fmt::format("{0}({1})", __FILE__, __LINE__)))

#define IMPULS_LOG(in_log_type, in_format, ...) IMPULS_LOG_INTERNAL(impuls::Logger::print, in_log_type, in_format, __VA_ARGS__)
#define IMPULS_LOG_W(in_log_type, in_format, ...) IMPULS_LOG_INTERNAL(impuls::Logger::print_warning, in_log_type, in_format, __VA_ARGS__)
#define IMPULS_LOG_E(in_log_type, in_format, ...) IMPULS_LOG_INTERNAL(impuls::Logger::print_error, in_log_type, in_format, __VA_ARGS__)
#define IMPULS_LOG_V(in_log_type, in_format, ...) IMPULS_LOG_INTERNAL(impuls::Logger::print_validation, in_log_type, in_format, __VA_ARGS__)
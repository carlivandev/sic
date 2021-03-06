#pragma once
#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

#include "fmt/format.h"

#include <string>
#include <mutex>

namespace sic
{
	struct Log : Noncopyable
	{
		Log(const std::string& in_name, bool in_enabled = true) : m_name(in_name), m_enabled(in_enabled) {}
		std::string m_name;
		bool m_enabled = true;
	};

	extern Log g_log_game;
	extern Log g_log_game_verbose;

	extern Log g_log_renderer;
	extern Log g_log_renderer_verbose;

	extern Log g_log_asset;
	extern Log g_log_asset_verbose;

	struct Logger : Noncopyable
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
		static Logger& get_instance()
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

#define SIC_LOG_INTERNAL(in_print_function, in_log_type, in_format, ...)	\
{if (in_log_type.m_enabled)													\
in_print_function(in_log_type, fmt::format(in_format, __VA_ARGS__), std::move(fmt::format("{0}({1})", __FILE__, __LINE__))); }

#define SIC_LOG(in_log_type, in_format, ...) SIC_LOG_INTERNAL(sic::Logger::print, in_log_type, in_format, __VA_ARGS__)
#define SIC_LOG_W(in_log_type, in_format, ...) SIC_LOG_INTERNAL(sic::Logger::print_warning, in_log_type, in_format, __VA_ARGS__)
#define SIC_LOG_E(in_log_type, in_format, ...) SIC_LOG_INTERNAL(sic::Logger::print_error, in_log_type, in_format, __VA_ARGS__)
#define SIC_LOG_V(in_log_type, in_format, ...) SIC_LOG_INTERNAL(sic::Logger::print_validation, in_log_type, in_format, __VA_ARGS__)
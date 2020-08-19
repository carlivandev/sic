#pragma once
#include "system.h"
#include "engine_context.h"
#include "sic/level_context.h"

#include "threadpool.h"

#include <mutex>
#include <string>
#include <vector>
#include <functional>

namespace sic
{
	using File_load_callback = std::function<void(std::string&& in_filedata)>;
	using File_save_callback = std::function<void(const std::string& in_path)>;

	struct File_load_request
	{
		File_load_request(std::string&& in_path, File_load_callback in_callback) : m_path(in_path), m_callback(in_callback) {}
		std::string m_path;
		File_load_callback m_callback;
	};

	struct File_save_request
	{
		File_save_request(std::string&& in_path, std::string&& in_filedata, File_save_callback in_callback) : m_path(in_path), m_filedata(in_filedata), m_callback(in_callback) {}
		std::string m_path;
		std::string m_filedata;
		File_save_callback m_callback;
	};

	struct State_filesystem : public State
	{
		friend struct System_file;

		void request_load(File_load_request&& in_request);
		void request_load(std::vector<File_load_request>&& in_requests);

		void request_save(File_save_request&& in_request);
		void request_save(std::vector<File_save_request>&& in_requests);

	private:
		std::vector<File_load_request> m_load_requests;
		std::vector<File_save_request> m_save_requests;
		std::mutex m_load_mutex;
		std::mutex m_save_mutex;
		Threadpool m_worker_pool;
	};

	struct System_file : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_tick(Scene_context in_context, float in_time_delta) const override;
		virtual void on_end_simulation(Scene_context in_context) const override;
	};
}
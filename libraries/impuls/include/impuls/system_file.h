#pragma once
#include "system.h"
#include "world_context.h"
#include "threadpool.h"

#include <mutex>
#include <string>
#include <vector>
#include <functional>

namespace impuls
{
	typedef std::function<void(std::string&& in_filedata)> file_load_callback;
	typedef std::function<void(const std::string& in_path)> file_save_callback;

	struct file_load_request
	{
		file_load_request(std::string&& in_path, file_load_callback in_callback) : m_path(in_path), m_callback(in_callback) {}
		std::string m_path;
		file_load_callback m_callback;
	};

	struct file_save_request
	{
		file_save_request(std::string&& in_path, std::string&& in_filedata, file_save_callback in_callback) : m_path(in_path), m_filedata(in_filedata), m_callback(in_callback) {}
		std::string m_path;
		std::string m_filedata;
		file_save_callback m_callback;
	};

	struct state_filesystem : public i_state
	{
		friend struct system_file;

		void request_load(file_load_request&& in_request);
		void request_load(std::vector<file_load_request>&& in_requests);

		void request_save(file_save_request&& in_request);
		void request_save(std::vector<file_save_request>&& in_requests);

	private:
		std::vector<file_load_request> m_load_requests;
		std::vector<file_save_request> m_save_requests;
		std::mutex m_load_mutex;
		std::mutex m_save_mutex;
		threadpool m_worker_pool;
	};

	struct system_file : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
		virtual void on_end_simulation(world_context&& in_context) const override;
	};
}
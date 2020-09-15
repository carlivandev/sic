#include "sic/system_file.h"
#include "sic/file_management.h"

void sic::State_filesystem::request_load(File_load_request&& in_request)
{
	std::scoped_lock lock(m_load_mutex);
	m_load_requests.push_back(in_request);
}

void sic::State_filesystem::request_load(std::vector<File_load_request>&& in_requests)
{
	std::scoped_lock lock(m_load_mutex);
	std::move(in_requests.begin(), in_requests.end(), std::back_inserter(m_load_requests));
}

void sic::State_filesystem::request_save(File_save_request&& in_request)
{
	std::scoped_lock lock(m_save_mutex);
	m_save_requests.push_back(in_request);
}

void sic::State_filesystem::request_save(std::vector<File_save_request>&& in_requests)
{
	std::scoped_lock lock(m_save_mutex);
	std::move(in_requests.begin(), in_requests.end(), std::back_inserter(m_save_requests));
}

void sic::System_file::on_created(Engine_context in_context)
{
	in_context.register_state<State_filesystem>("state_filesystem");
	in_context.get_state<State_filesystem>()->m_worker_pool.spawn(2);
}

void sic::System_file::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_filesystem& file_state = in_context.get_state_checked<State_filesystem>();

	if (file_state.m_load_requests.size() == 0 &&
		file_state.m_save_requests.size() == 0)
	{
		return;
	}

	std::scoped_lock load_lock(file_state.m_load_mutex);
	std::scoped_lock save_lock(file_state.m_save_mutex);

	std::vector<Threadpool::Closure> closures;
	closures.reserve(file_state.m_load_requests.size() + file_state.m_save_requests.size());

	for (const File_load_request& load_req : file_state.m_load_requests)
	{
		if (!load_req.m_callback)
			continue;

		closures.push_back
		(
			[load_req]()
			{
				load_req.m_callback(File_management::load_file(load_req.m_path));
			}
		);
	}

	for (const File_save_request& save_req : file_state.m_save_requests)
	{
		closures.push_back
		(
			[save_req]()
			{
				File_management::save_file(save_req.m_path, save_req.m_filedata);
				save_req.m_callback(save_req.m_path);
			}
		);
	}

	file_state.m_worker_pool.batch(std::move(closures));

	file_state.m_save_requests.clear();
	file_state.m_load_requests.clear();
}

void sic::System_file::on_end_simulation(Scene_context in_context) const
{
	State_filesystem* file_state = in_context.get_engine_context().get_state<State_filesystem>();

	if (!file_state)
		return;

	file_state->m_worker_pool.shutdown();
}
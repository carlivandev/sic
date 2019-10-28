#include "impuls/system_file.h"
#include "impuls/file_management.h"

void impuls::state_filesystem::request_load(file_load_request&& in_request)
{
	std::scoped_lock lock(m_load_mutex);
	m_load_requests.push_back(in_request);
}

void impuls::state_filesystem::request_load(std::vector<file_load_request>&& in_requests)
{
	std::scoped_lock lock(m_load_mutex);
	std::move(in_requests.begin(), in_requests.end(), std::back_inserter(m_load_requests));
}

void impuls::state_filesystem::request_save(file_save_request&& in_request)
{
	std::scoped_lock lock(m_save_mutex);
	m_save_requests.push_back(in_request);
}

void impuls::state_filesystem::request_save(std::vector<file_save_request>&& in_requests)
{
	std::scoped_lock lock(m_save_mutex);
	std::move(in_requests.begin(), in_requests.end(), std::back_inserter(m_save_requests));
}

void impuls::system_file::on_created(world_context&& in_context) const
{
	in_context.register_state<state_filesystem>("state_filesystem");
	in_context.get_state<state_filesystem>()->m_worker_pool.spawn(4);
}

void impuls::system_file::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_filesystem* file_state = in_context.get_state<state_filesystem>();

	if (!file_state)
		return;

	if (file_state->m_load_requests.size() == 0 &&
		file_state->m_save_requests.size() == 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		return;
	}

	std::scoped_lock load_lock(file_state->m_load_mutex);
	std::scoped_lock save_lock(file_state->m_save_mutex);

	std::vector<threadpool::closure> closures;
	closures.reserve(file_state->m_load_requests.size() + file_state->m_save_requests.size());

	for (const file_load_request& load_req : file_state->m_load_requests)
	{
		if (!load_req.m_callback)
			continue;

		closures.push_back
		(
			[load_req]()
			{
				load_req.m_callback(file_management::load_file(load_req.m_path));
			}
		);
	}

	for (const file_save_request& save_req : file_state->m_save_requests)
	{
		closures.push_back
		(
			[save_req]()
			{
				file_management::save_file(save_req.m_path, save_req.m_filedata);
				save_req.m_callback(save_req.m_path);
			}
		);
	}

	file_state->m_worker_pool.batch(std::move(closures));

	file_state->m_save_requests.clear();
	file_state->m_load_requests.clear();
}

void impuls::system_file::on_end_simulation(world_context&& in_context) const
{
	state_filesystem* file_state = in_context.get_state<state_filesystem>();

	if (!file_state)
		return;

	file_state->m_worker_pool.shutdown();
}

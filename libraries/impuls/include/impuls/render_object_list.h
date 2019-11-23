#pragma once
#include "impuls/double_buffer.h"

#include <vector>
#include <mutex>

namespace impuls
{
	enum class e_render_object_update_type
	{
		create,
		destroy,
		update
	};

	template <typename t_render_object_type>
	struct render_object_list
	{
		struct render_object_update
		{
			using callback = std::function<void(t_render_object_type& in_out_object)>;

			callback m_callback;
			size_t m_render_object_id = 0;
			e_render_object_update_type m_type = e_render_object_update_type::create;
		};

		i32 create_render_object(typename render_object_update::callback&& in_update_callback = nullptr);
		void destroy_render_object(i32 in_id);

		void update_render_object(i32 in_object_id, typename render_object_update::callback&& in_update_callback);

		void flush_updates();

		std::vector<t_render_object_type> m_render_objects;
		std::vector<size_t> m_render_objects_free_indices;

		std::mutex m_render_object_update_lock;

		double_buffer<std::vector<render_object_update>> m_render_object_data_updates;
	};

	template<typename t_render_object_type>
	inline i32 render_object_list<t_render_object_type>::create_render_object(typename render_object_update::callback&& in_update_callback)
	{
		std::scoped_lock lock(m_render_object_update_lock);

		size_t new_idx = 0;

		if (m_render_objects_free_indices.empty())
		{
			new_idx = m_render_objects.size();
		}
		else
		{
			new_idx = m_render_objects_free_indices.back();
			m_render_objects_free_indices.pop_back();
		}

		m_render_object_data_updates.write
		(
			[in_update_callback, new_idx](std::vector<render_object_update>& in_out_updates)
			{
				in_out_updates.push_back({ in_update_callback, new_idx, e_render_object_update_type::create });
			}
		);

		return static_cast<i32>(new_idx);
	}

	template<typename t_render_object_type>
	inline void render_object_list<t_render_object_type>::destroy_render_object(i32 in_id)
	{
		std::scoped_lock lock(m_render_object_update_lock);

		m_render_objects_free_indices.push_back(in_id);

		m_render_object_data_updates.write
		(
			[in_id](std::vector<render_object_update>& in_out_updates)
			{
				in_out_updates.push_back({ nullptr, static_cast<size_t>(in_id), e_render_object_update_type::destroy });
			}
		);
	}
	template<typename t_render_object_type>
	inline void render_object_list<t_render_object_type>::update_render_object(i32 in_object_id, typename render_object_update::callback&& in_update_callback)
	{
		m_render_object_data_updates.write
		(
			[in_object_id, &in_update_callback](std::vector<render_object_update>& in_out_updates)
			{
				in_out_updates.push_back({ in_update_callback, static_cast<size_t>(in_object_id), e_render_object_update_type::update });
			}
		);
	}

	template<typename t_render_object_type>
	inline void render_object_list<t_render_object_type>::flush_updates()
	{
		std::scoped_lock lock(m_render_object_update_lock);

		m_render_object_data_updates.read
		(
			[this](const std::vector<render_object_update>& in_updates)
			{
				for (const render_object_update& update : in_updates)
				{
					if (update.m_type == e_render_object_update_type::create)
					{
						if (m_render_objects.size() >= update.m_render_object_id)
							m_render_objects.resize(update.m_render_object_id + 1);
						else
							new (&m_render_objects[update.m_render_object_id]) t_render_object_type();
					}
					else if (update.m_type == e_render_object_update_type::destroy)
					{
						m_render_objects[update.m_render_object_id].~t_render_object_type();
						m_render_objects_free_indices.push_back(update.m_render_object_id);
					}
					
					if (update.m_callback)
						update.m_callback(m_render_objects[update.m_render_object_id]);
				}
			}
		);

		m_render_object_data_updates.swap
		(
			[](std::vector<render_object_update>& in_out_read, std::vector<render_object_update>&)
			{
				in_out_read.clear();
			}
		);
	}
}
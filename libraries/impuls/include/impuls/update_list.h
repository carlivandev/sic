#pragma once
#include "impuls/double_buffer.h"

#include <vector>
#include <mutex>

namespace impuls
{
	enum class List_update_type
	{
		create,
		destroy,
		Update
	};

	/*
	list of t_object_type, where all updates to the list are pushed from thead A and executed on thread B
	which means thead B is the owner of the data
	*/

	template <typename t_object_type>
	struct Update_list_id
	{
		template <typename t>
		friend struct Update_list;
		Update_list_id() = default;

	private:
		Update_list_id(i32 in_id) : m_id(in_id) {}

		i32 m_id = -1;
	};

	template <typename t_object_type>
	struct Update_list
	{
		struct Update
		{
			using callback = std::function<void(t_object_type& in_out_object)>;

			callback m_callback;
			Update_list_id<t_object_type> m_object_id;
			List_update_type m_type = List_update_type::create;
		};

		Update_list_id<t_object_type> create_object(typename Update::callback&& in_update_callback = nullptr);
		void destroy_object(Update_list_id<t_object_type> in_id);

		void update_object(Update_list_id<t_object_type> in_object_id, typename Update::callback&& in_update_callback);

		void flush_updates();

		std::vector<t_object_type> m_objects;
		std::vector<size_t> m_objects_free_indices;

		std::mutex m_update_lock;

		Double_buffer<std::vector<Update>> m_object_data_updates;
		size_t m_current_create_id = 0;
	};

	template<typename t_object_type>
	inline Update_list_id<t_object_type> Update_list<t_object_type>::create_object(typename Update::callback&& in_update_callback)
	{
		std::scoped_lock lock(m_update_lock);

		size_t new_idx = 0;

		if (m_objects_free_indices.empty())
		{
			new_idx = m_current_create_id++;
		}
		else
		{
			new_idx = m_objects_free_indices.back();
			m_objects_free_indices.pop_back();
		}

		Update_list_id<t_object_type> new_id(static_cast<i32>(new_idx));

		m_object_data_updates.write
		(
			[in_update_callback, new_id](std::vector<Update>& in_out_updates)
			{
				in_out_updates.push_back({ in_update_callback, new_id, List_update_type::create });
			}
		);

		return new_id;
	}

	template<typename t_object_type>
	inline void Update_list<t_object_type>::destroy_object(Update_list_id<t_object_type> in_id)
	{
		assert(in_id.m_id != -1 && "Invalid object ID!");

		std::scoped_lock lock(m_update_lock);

		m_objects_free_indices.push_back(in_id.m_id);

		m_object_data_updates.write
		(
			[in_id](std::vector<Update>& in_out_updates)
			{
				in_out_updates.push_back({ nullptr, in_id, List_update_type::destroy });
			}
		);
	}
	template<typename t_object_type>
	inline void Update_list<t_object_type>::update_object(Update_list_id<t_object_type> in_object_id, typename Update::callback&& in_update_callback)
	{
		assert(in_object_id.m_id != -1 && "Invalid object ID!");

		m_object_data_updates.write
		(
			[in_object_id, &in_update_callback](std::vector<Update>& in_out_updates)
			{
				in_out_updates.push_back({ in_update_callback, in_object_id, List_update_type::Update });
			}
		);
	}

	template<typename t_object_type>
	inline void Update_list<t_object_type>::flush_updates()
	{
		std::scoped_lock lock(m_update_lock);

		m_object_data_updates.read
		(
			[this](const std::vector<Update>& in_updates)
			{
				for (const Update& update_instance : in_updates)
				{
					if (update_instance.m_type == List_update_type::create)
					{
						if (m_objects.size() >= update_instance.m_object_id.m_id)
						{
							m_objects.emplace_back();
							assert(m_objects.size() - 1 == update_instance.m_object_id.m_id && "When adding an object to end of array it should always match with the ID!");
						}
						else
						{
							new (&m_objects[update_instance.m_object_id.m_id]) t_object_type();
						}
					}
					else if (update_instance.m_type == List_update_type::destroy)
					{
						m_objects[update_instance.m_object_id.m_id].~t_object_type();
						m_objects_free_indices.push_back(update_instance.m_object_id.m_id);
					}
					
					if (update_instance.m_callback)
						update_instance.m_callback(m_objects[update_instance.m_object_id.m_id]);
				}
			}
		);

		m_object_data_updates.swap
		(
			[](std::vector<Update>& in_out_read, std::vector<Update>&)
			{
				in_out_read.clear();
			}
		);

		m_current_create_id = m_objects.size();
	}
}
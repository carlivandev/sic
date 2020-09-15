#pragma once
#include "sic/core/double_buffer.h"

#include <vector>
#include <unordered_map>
#include <mutex>

namespace sic
{
	enum struct List_update_type
	{
		create,
		destroy,
		Update
	};

	/*
	list of T_object_type, where all updates to the list are pushed from thead A and executed on thread B
	which means thead B is the owner of the data
	*/

	template <typename T_object_type>
	struct Update_list_id
	{
		template <typename t>
		friend struct Update_list;
		Update_list_id() = default;

		void reset()
		{
			m_id = -1;
		}

		bool is_valid() const
		{
			return m_id != -1;
		}

	private:
		Update_list_id(i32 in_id) : m_id(in_id) {}

		i32 m_id = -1;
	};

	template <typename T_object_type>
	struct Update_list
	{
		struct Update
		{
			using Callback = std::function<void(T_object_type& in_out_object)>;

			Callback m_callback;
			Update_list_id<T_object_type> m_object_id;
			List_update_type m_type = List_update_type::create;
		};

		Update_list_id<T_object_type> create_object(typename Update::Callback&& in_update_callback = nullptr);
		void destroy_object(Update_list_id<T_object_type> in_id);

		void update_object(Update_list_id<T_object_type> in_object_id, typename Update::Callback&& in_update_callback);

		void flush_updates();

		//only safe from data reciever
		T_object_type* find_object(Update_list_id<T_object_type> in_object_id);

		//only safe from data reciever
		const T_object_type* find_object(Update_list_id<T_object_type> in_object_id) const;

		//only safe from data reciever
		std::vector<T_object_type> m_objects;

	private:
		std::unordered_map<size_t, i32> m_id_to_index_lut;
		std::unordered_map<i32, size_t> m_index_to_id_lut;

		std::vector<size_t> m_objects_free_ids;

		std::mutex m_update_lock;

		std::vector<Update> m_object_data_updates;
		size_t m_current_create_id = 0;
	};

	template<typename T_object_type>
	inline Update_list_id<T_object_type> Update_list<T_object_type>::create_object(typename Update::Callback&& in_update_callback)
	{
		std::scoped_lock lock(m_update_lock);

		size_t new_id = 0;

		if (m_objects_free_ids.empty())
		{
			new_id = m_current_create_id++;
		}
		else
		{
			new_id = m_objects_free_ids.back();
			m_objects_free_ids.pop_back();
		}

		Update_list_id<T_object_type> new_update_list_id(static_cast<i32>(new_id));

		m_object_data_updates.push_back({ in_update_callback, new_update_list_id, List_update_type::create });
		return new_update_list_id;
	}

	template<typename T_object_type>
	inline void Update_list<T_object_type>::destroy_object(Update_list_id<T_object_type> in_id)
	{
		assert(in_id.is_valid() && "Invalid object ID!");

		std::scoped_lock lock(m_update_lock);

		m_objects_free_ids.push_back(in_id.m_id);

		m_object_data_updates.push_back({ nullptr, in_id, List_update_type::destroy });
	}
	template<typename T_object_type>
	inline void Update_list<T_object_type>::update_object(Update_list_id<T_object_type> in_object_id, typename Update::Callback&& in_update_callback)
	{
		assert(in_object_id.is_valid() && "Invalid object ID!");

		std::scoped_lock lock(m_update_lock);

		m_object_data_updates.push_back({ in_update_callback, in_object_id, List_update_type::Update });
	}

	template<typename T_object_type>
	inline void Update_list<T_object_type>::flush_updates()
	{
		std::scoped_lock lock(m_update_lock);

		for (const Update& update_instance : m_object_data_updates)
		{
			if (update_instance.m_type == List_update_type::create)
			{
				m_objects.emplace_back();

				const i32 last_idx = static_cast<i32>(m_objects.size()) - 1;

				m_id_to_index_lut[update_instance.m_object_id.m_id] = last_idx;
				m_index_to_id_lut[last_idx] = update_instance.m_object_id.m_id;
			}
			else if (update_instance.m_type == List_update_type::destroy)
			{
				const i32 remove_idx = m_id_to_index_lut[update_instance.m_object_id.m_id];
				const i32 last_idx = static_cast<i32>(m_objects.size()) - 1;

				if (remove_idx < last_idx)
				{
					std::swap(m_objects[remove_idx], m_objects.back());

					const size_t last_id = m_index_to_id_lut[last_idx];

					m_id_to_index_lut[last_id] = remove_idx;
					m_index_to_id_lut[remove_idx] = last_id;
				}

				m_id_to_index_lut[update_instance.m_object_id.m_id] = -1;
				m_objects.pop_back();
			}
					
			if (update_instance.m_callback)
				update_instance.m_callback(m_objects[m_id_to_index_lut.find(update_instance.m_object_id.m_id)->second]);
		}

		m_object_data_updates.clear();

		m_current_create_id = m_objects.size();
	}
	template<typename T_object_type>
	inline T_object_type* Update_list<T_object_type>::find_object(Update_list_id<T_object_type> in_object_id)
	{
		auto it = m_id_to_index_lut.find(in_object_id.m_id);

		if (it == m_id_to_index_lut.end() || it->second == -1)
			return nullptr;

		return &m_objects[it->second];
	}
	template<typename T_object_type>
	inline const T_object_type* Update_list<T_object_type>::find_object(Update_list_id<T_object_type> in_object_id) const
	{
		auto it = m_id_to_index_lut.find(in_object_id.m_id);

		if (it == m_id_to_index_lut.end() || it->second == -1)
			return nullptr;

		return &m_objects[it->second];
	}
}
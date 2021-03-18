#pragma once
#include "sic/core/double_buffer.h"

#include <vector>
#include <unordered_map>
#include <mutex>

namespace sic
{
	enum struct List_update_type
	{
		Create,
		Destroy,
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

		Update_list_id(i32 in_id) : m_id(in_id) {}

		void set(i32 in_id)
		{
			m_id = in_id;
		}

		void reset()
		{
			m_id = -1;
		}

		bool is_valid() const
		{
			return m_id != -1;
		}

		i32 get_id() const { return m_id; }

	private:

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
			List_update_type m_type = List_update_type::Create;
		};

		void create_object(Update_list_id<T_object_type> in_id, typename Update::Callback&& in_update_callback = nullptr);
		void destroy_object(Update_list_id<T_object_type> in_id, typename Update::Callback&& in_update_callback = nullptr);

		void update_object(Update_list_id<T_object_type> in_object_id, typename Update::Callback&& in_update_callback);

		void flush_updates();

		//only safe from data reciever
		T_object_type* find_object(Update_list_id<T_object_type> in_object_id);

		//only safe from data reciever
		const T_object_type* find_object(Update_list_id<T_object_type> in_object_id) const;

		//only safe from data reciever
		Update_list_id<T_object_type> get_id(const T_object_type& in_object) const
		{
			const T_object_type* data_loc = &in_object;
			assert(data_loc >= m_objects.data() && data_loc < m_objects.data() + m_objects.size() && "Object not part of list.");

			const size_t index = data_loc - m_objects.data();
			return Update_list_id<T_object_type>(m_index_to_id_lut.find(index)->second);
		}

		//only safe from data reciever
		std::vector<T_object_type> m_objects;

	private:
		std::unordered_map<size_t, i32> m_id_to_index_lut;
		std::unordered_map<i32, size_t> m_index_to_id_lut;

		std::mutex m_update_lock;

		std::vector<Update> m_object_data_updates;
	};

	template<typename T_object_type>
	inline void Update_list<T_object_type>::create_object(Update_list_id<T_object_type> in_id, typename Update::Callback&& in_update_callback)
	{
		std::scoped_lock lock(m_update_lock);

		assert(in_id.is_valid() && "Invalid object ID!");

		m_object_data_updates.push_back({ in_update_callback, in_id, List_update_type::Create });
	}

	template<typename T_object_type>
	inline void Update_list<T_object_type>::destroy_object(Update_list_id<T_object_type> in_id, typename Update::Callback&& in_update_callback)
	{
		assert(in_id.is_valid() && "Invalid object ID!");

		std::scoped_lock lock(m_update_lock);

		m_object_data_updates.push_back({ in_update_callback, in_id, List_update_type::Destroy });
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
			if (update_instance.m_type == List_update_type::Create)
			{
				m_objects.emplace_back();

				const i32 last_idx = static_cast<i32>(m_objects.size()) - 1;

				m_id_to_index_lut[update_instance.m_object_id.m_id] = last_idx;
				m_index_to_id_lut[last_idx] = update_instance.m_object_id.m_id;

				if (update_instance.m_callback)
					update_instance.m_callback(m_objects[m_id_to_index_lut.find(update_instance.m_object_id.m_id)->second]);
			}
			else if (update_instance.m_type == List_update_type::Update)
			{
				if (update_instance.m_callback)
					update_instance.m_callback(m_objects[m_id_to_index_lut.find(update_instance.m_object_id.m_id)->second]);
			}
			else if (update_instance.m_type == List_update_type::Destroy)
			{
				if (update_instance.m_callback)
					update_instance.m_callback(m_objects[m_id_to_index_lut.find(update_instance.m_object_id.m_id)->second]);

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
		}

		m_object_data_updates.clear();
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
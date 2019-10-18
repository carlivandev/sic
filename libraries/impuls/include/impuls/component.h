#pragma once
#include "bucket_allocator.h"

namespace impuls
{
	struct i_object_base;
	struct typeinfo;

	struct i_component_base
	{
		friend struct world;
		friend struct component_storage;

		virtual ~i_component_base() = default;

		inline bool is_valid() const
		{
			return m_owner != nullptr;
		}

		inline i_object_base& owner() { return *m_owner; }
		inline const i_object_base& owner() const { return *m_owner; }

	private:
		i_object_base* m_owner = nullptr;
	};

	struct i_component : public i_component_base
	{
		friend struct world_context;

		virtual ~i_component() = default;

	};

	struct component_storage
	{
		template <typename t_component_type>
		void initialize(ui32 in_initial_capacity, ui32 in_bucket_capacity);
		
		void initialize_with_typesize(ui32 in_initial_capacity, ui32 in_bucket_capacity, ui32 in_typesize);

		template <typename t_component_type>
		t_component_type& create_component();

		template <typename t_component_type>
		void destroy_component(t_component_type& in_component_to_destroy);

		i_component_base* next_valid_component(i32 in_instance_index);
		i_component_base* previous_valid_component(i32 in_instance_index);

		bool initialized() const { return m_component_type_size != 0; }

		bucket_byte_allocator m_components;
		std::vector<i_object_base*> m_owners;
		std::vector<byte*> m_free_component_locations;
		ui32 m_component_type_size = 0;
		uint64_t m_component_flag_value = 0;
		ui32 m_bucket_size = 0;
	};

	template<typename t_component_type>
	inline void component_storage::initialize(ui32 in_initial_capacity, ui32 in_bucket_capacity)
	{
		static_assert(std::is_base_of<i_component_base, t_component_type>::value, "t_component_type must derive from struct i_component_base");

		m_component_type_size = sizeof(t_component_type);
		m_bucket_size = in_bucket_capacity;

		m_components.allocate(in_initial_capacity * m_component_type_size, in_bucket_capacity * m_component_type_size, m_component_type_size);
	}

	template<typename t_component_type>
	inline t_component_type& component_storage::create_component()
	{
		static_assert(std::is_base_of<i_component_base, t_component_type>::value, "t_component_type must derive from struct i_component_base");

		if (!m_free_component_locations.empty())
		{
			byte* new_loc = m_free_component_locations.back();
			new (new_loc) t_component_type();

			m_free_component_locations.pop_back();

			t_component_type* new_instance = reinterpret_cast<t_component_type*>(new_loc);

			return *new_instance;
		}

		const i32 index = static_cast<int>(m_components.size() / sizeof(t_component_type));

		t_component_type* new_instance = reinterpret_cast<t_component_type*>(&m_components.emplace_back(sizeof(t_component_type)));
		new (new_instance) t_component_type();

		return *new_instance;
	}

	template<typename t_component_type>
	inline void component_storage::destroy_component(t_component_type& in_component_to_destroy)
	{
		static_assert(std::is_base_of<i_component_base, t_component_type>::value, "t_component_type must derive from struct i_component_base");

		assert(in_component_to_destroy.m_owner && "owner was NULL, component is already destroyed");

		m_free_component_locations.push_back(reinterpret_cast<byte*>(&in_component_to_destroy));

		in_component_to_destroy.~t_component_type();
		in_component_to_destroy.m_owner = nullptr;
	}
}
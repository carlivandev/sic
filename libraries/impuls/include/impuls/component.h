#pragma once
#include "bucket_allocator.h"

#include "impuls/type_restrictions.h"
#include "impuls/colony.h"

namespace impuls
{
	struct i_object_base;
	struct typeinfo;

	struct i_component_base : public i_noncopyable
	{
		friend struct engine;
		friend struct level;

		template <typename t_component_type>
		friend struct component_storage;

		i_component_base() = default;
		virtual ~i_component_base() = default;

		i_component_base(i_component_base&&) noexcept = default;
		i_component_base& operator=(i_component_base&&) noexcept = default;
		
		inline bool is_valid() const
		{
			return m_owner != nullptr;
		}

		inline i_object_base& owner() { return *m_owner; }
		inline const i_object_base& owner() const { return *m_owner; }

	private:
		i_object_base* m_owner = nullptr;
	};

	struct i_component : i_component_base
	{
		i_component() = default;
		virtual ~i_component() = default;

		i_component(i_component&&) noexcept = default;
		i_component& operator=(i_component&&) noexcept = default;
	};

	struct i_component_storage
	{
		bool initialized() const { return m_component_type_size != 0; }

		ui32 m_component_type_size = 0;
	};

	template <typename t_component_type>
	struct component_storage : i_component_storage
	{
		void initialize(ui32 in_initial_capacity);

		t_component_type& create_component();
		void destroy_component(t_component_type& in_component_to_destroy);

		plf::colony<t_component_type> m_components;
	};

	template<typename t_component_type>
	inline void component_storage<t_component_type>::initialize(ui32 in_initial_capacity)
	{
		static_assert(std::is_base_of<i_component_base, t_component_type>::value, "t_component_type must derive from struct i_component_base");

		m_component_type_size = sizeof(t_component_type);
		m_components.reserve(in_initial_capacity);
	}

	template<typename t_component_type>
	inline t_component_type& component_storage<t_component_type>::create_component()
	{
		return (*m_components.emplace());
	}

	template<typename t_component_type>
	inline void component_storage<t_component_type>::destroy_component(t_component_type& in_component_to_destroy)
	{
		m_components.erase(m_components.get_iterator_from_pointer(&in_component_to_destroy));
	}
}
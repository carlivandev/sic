#pragma once
#include "bucket_allocator.h"

#include "sic/type_restrictions.h"
#include "sic/colony.h"

namespace sic
{
	struct Object_base;
	struct Typeinfo;

	struct Component_base : public Noncopyable
	{
		friend struct Engine;
		friend struct Scene;

		template <typename T_component_type>
		friend struct Component_storage;

		Component_base() = default;
		virtual ~Component_base() = default;

		Component_base(Component_base&&) noexcept = default;
		Component_base& operator=(Component_base&&) noexcept = default;
		
		inline bool is_valid() const
		{
			return m_owner != nullptr;
		}

		inline Object_base& get_owner() { return *m_owner; }
		inline const Object_base& get_owner() const { return *m_owner; }

	private:
		Object_base* m_owner = nullptr;
	};

	struct Component : Component_base
	{
		Component() = default;
		virtual ~Component() = default;

		Component(Component&&) noexcept = default;
		Component& operator=(Component&&) noexcept = default;
	};

	struct Component_storage_base
	{
		bool initialized() const { return m_component_type_size != 0; }

		ui32 m_component_type_size = 0;
	};

	template <typename T_component_type>
	struct Component_storage : Component_storage_base
	{
		void initialize(ui32 in_initial_capacity);

		T_component_type& create_component();
		void destroy_component(T_component_type& in_component_to_destroy);

		plf::colony<T_component_type> m_components;
	};

	template<typename T_component_type>
	inline void Component_storage<T_component_type>::initialize(ui32 in_initial_capacity)
	{
		static_assert(std::is_base_of<Component_base, T_component_type>::value, "T_component_type must derive from struct Component_base");

		m_component_type_size = sizeof(T_component_type);
		m_components.reserve(in_initial_capacity);
	}

	template<typename T_component_type>
	inline T_component_type& Component_storage<T_component_type>::create_component()
	{
		return (*m_components.emplace());
	}

	template<typename T_component_type>
	inline void Component_storage<T_component_type>::destroy_component(T_component_type& in_component_to_destroy)
	{
		m_components.erase(m_components.get_iterator_from_pointer(&in_component_to_destroy));
	}
}
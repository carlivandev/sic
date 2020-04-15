#pragma once
#include "engine.h"
#include "level.h"

namespace sic
{
	struct Engine_context
	{
		Engine_context() = default;
		Engine_context(Engine& in_engine) : m_engine(&in_engine) {}

		template<typename t_system_type>
		__forceinline t_system_type& create_subsystem(System& inout_system)
		{
			auto& new_system = m_engine->create_system<t_system_type>();
			inout_system.m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename t_component>
		__forceinline constexpr Type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			return m_engine->register_component_type<t_component>(in_unique_key, in_initial_capacity);
		}

		template <typename t_object>
		__forceinline constexpr Type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			return m_engine->register_object<t_object>(in_unique_key, in_initial_capacity, in_bucket_capacity);
		}

		template <typename t_state>
		__forceinline constexpr Type_reg register_state(const char* in_unique_key)
		{
			const ui32 type_idx = Type_index<State>::get<t_state>();

			auto& state_to_register = m_engine->get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<t_state>();

			return std::move(register_typeinfo<t_state>(in_unique_key));
		}

		template <typename t_type_to_register>
		__forceinline constexpr Type_reg register_typeinfo(const char* in_unique_key)
		{
			return m_engine->register_typeinfo<t_type_to_register>(in_unique_key);
		}

		template <typename t_type>
		__forceinline constexpr Typeinfo* get_typeinfo()
		{
			return m_engine->get_typeinfo<t_type>();
		}

		template <typename t_state>
		__forceinline t_state* get_state()
		{
			return m_engine->get_state<t_state>();
		}

		template <typename t_state>
		__forceinline t_state& get_state_checked()
		{
			t_state* state = m_engine->get_state<t_state>();
			assert(state && "State was not registered before use!");

			return *state;
		}

		template <typename t_event_type, typename t_functor>
		void listen(t_functor in_func)
		{
			m_engine->listen<t_event_type, t_functor>(std::move(in_func));
		}

		template <typename t_event_type, typename event_data>
		void invoke(event_data& event_data_to_send)
		{
			m_engine->invoke<t_event_type, event_data>(event_data_to_send);
		}

		template<typename t_type>
		__forceinline void for_each(std::function<void(t_type&)> in_func)
		{
			if constexpr (std::is_same<t_type, Level>::value)
			{
				for (auto& level : m_engine->m_levels)
					for_each_level(in_func, *level.get());
			}
			else
			{
				for (auto& level : m_engine->m_levels)
					level->for_each<t_type>(in_func);
			}
		}

		template<typename t_type>
		__forceinline void for_each(std::function<void(const t_type&)> in_func) const
		{
			m_level.for_each<t_type>(in_func);
		}

		void shutdown()
		{
			m_engine->shutdown();
		}

	private:
		void for_each_level(std::function<void(Level&)> in_func, Level& inout_level)
		{
			in_func(inout_level);

			for (auto& sublevel : inout_level.m_sublevels)
				for_each_level(in_func, *sublevel.get());
		}

		Engine* m_engine = nullptr;
	};
}


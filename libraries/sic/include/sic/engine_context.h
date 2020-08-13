#pragma once
#include "engine.h"
#include "level.h"

namespace sic
{
	struct Engine_context
	{
		Engine_context() = default;
		Engine_context(Engine& in_engine) : m_engine(&in_engine) {}

		template<typename T_system_type>
		__forceinline T_system_type& create_subsystem(System& inout_system)
		{
			auto& new_system = m_engine->create_system<T_system_type>();
			inout_system.m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename T_component>
		__forceinline constexpr Type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			return m_engine->register_component_type<T_component>(in_unique_key, in_initial_capacity);
		}

		template <typename T_object>
		__forceinline constexpr Type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			return m_engine->register_object<T_object>(in_unique_key, in_initial_capacity, in_bucket_capacity);
		}

		template <typename T_state>
		__forceinline constexpr Type_reg register_state(const char* in_unique_key)
		{
			const ui32 type_idx = Type_index<State>::get<T_state>();

			auto& state_to_register = m_engine->get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<T_state>();

			return std::move(register_typeinfo<T_state>(in_unique_key));
		}

		template <typename T_type_to_register>
		__forceinline constexpr Type_reg register_typeinfo(const char* in_unique_key)
		{
			return m_engine->register_typeinfo<T_type_to_register>(in_unique_key);
		}

		template <typename T_type>
		__forceinline constexpr Typeinfo* get_typeinfo()
		{
			return m_engine->get_typeinfo<T_type>();
		}

		template <typename T_state>
		__forceinline T_state* get_state()
		{
			return m_engine->get_state<T_state>();
		}

		template <typename T_state>
		__forceinline T_state& get_state_checked()
		{
			T_state* state = m_engine->get_state<T_state>();
			assert(state && "State was not registered before use!");

			return *state;
		}

		template <typename T_event_type, typename T_functor>
		void listen(T_functor in_func)
		{
			m_engine->listen<T_event_type, T_functor>(std::move(in_func));
		}

		template <typename T_event_type, typename T_event_data>
		void invoke(T_event_data& event_data_to_send)
		{
			m_engine->invoke<T_event_type, T_event_data>(event_data_to_send);
		}

		template<typename T_type>
		__forceinline void for_each(std::function<void(T_type&)> in_func)
		{
			if constexpr (std::is_same<T_type, Level>::value)
			{
				for (auto& level : m_engine->m_levels)
					for_each_level(in_func, *level.get());
			}
			else
			{
				for (auto& level : m_engine->m_levels)
					level->for_each_w<T_type>(in_func);
			}
		}

		template<typename T_type>
		__forceinline void for_each(std::function<void(const T_type&)> in_func) const
		{
			m_level.for_each_w_w<T_type>(in_func);
		}

		void create_level(Level* in_parent_level)
		{
			m_engine->create_level(in_parent_level);
		}

		void destroy_level(Level& inout_level)
		{
			m_engine->destroy_level(inout_level);
		}

		void shutdown()
		{
			m_engine->shutdown();
		}

		bool get_does_object_exist(Object_id in_id) const
		{
			std::scoped_lock lock(m_engine->m_levels_mutex);

			auto it = m_engine->m_level_id_to_level_lut.find(in_id.get_level_id());

			if (it == m_engine->m_level_id_to_level_lut.end())
				return false;

			return it->second->get_does_object_exist(in_id, false);
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


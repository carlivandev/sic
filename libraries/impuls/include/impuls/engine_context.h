#pragma once
#include "engine.h"

namespace impuls
{
	struct engine_context
	{
		engine_context(engine& in_engine) : m_engine(in_engine) {}

		template<typename t_system_type>
		__forceinline t_system_type& create_subsystem(i_system& inout_system)
		{
			auto& new_system = m_engine.create_system<t_system_type>();
			inout_system.m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename t_component>
		__forceinline constexpr type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			return m_engine.register_component_type<t_component>(in_unique_key, in_initial_capacity);
		}

		template <typename t_object>
		__forceinline constexpr type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			return m_engine.register_object<t_object>(in_unique_key, in_initial_capacity, in_bucket_capacity);
		}

		template <typename t_state>
		__forceinline constexpr type_reg register_state(const char* in_unique_key)
		{
			const ui32 type_idx = type_index<i_state>::get<t_state>();

			auto& state_to_register = m_engine.get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<t_state>();

			return std::move(register_typeinfo<t_state>(in_unique_key));
		}

		template <typename t_type_to_register>
		__forceinline constexpr type_reg register_typeinfo(const char* in_unique_key)
		{
			return m_engine.register_typeinfo<t_type_to_register>(in_unique_key);
		}

		template <typename t_type>
		__forceinline constexpr typeinfo* get_typeinfo()
		{
			return m_engine.get_typeinfo<t_type>();
		}

		template <typename t_state>
		__forceinline t_state* get_state()
		{
			return m_engine.get_state<t_state>();
		}

		template <typename t_event_type, typename t_functor>
		void listen(t_functor in_func)
		{
			m_engine.listen<t_event_type, t_functor>(std::move(in_func));
		}

		template <typename t_event_type, typename event_data>
		void invoke(event_data& event_data_to_send)
		{
			m_engine.invoke<t_event_type, event_data>(event_data_to_send);
		}

		engine& m_engine;
	};
}


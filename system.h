#pragma once
#include "system_base.h"
#include "world.h"
#include "policy.h"

namespace impuls
{
	template <typename ...policies>
	struct i_system : public i_system_base
	{
		object& create_object()
		{
			return m_world->create_object();
		}

		void destroy_object(object& in_object_to_destroy)
		{
			m_world->destroy_object(in_object_to_destroy);
		}

		template <typename component_type>
		component_type& create_component(object& in_object_to_attach_to)
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, component_type>::value;

			static_assert(is_valid_type, "can only create types that derive i_component");
			static_assert((policies::template verify<policy_write<component_type>>() || ...), "this system does not have policy_write of that type!");

			return m_world->create_component<component_type>(in_object_to_attach_to);
		}

		template <typename component_type>
		void destroy_component(component_type& in_component_to_destroy)
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, component_type>::value;

			static_assert(is_valid_type, "can only destroy types that derive i_component");
			static_assert((policies::template verify<policy_write<component_type>>() || ...), "this system does not have policy_write of that type!");

			return m_world->destroy_component<component_type>(in_component_to_destroy);
		}

		template <typename component_type>
		component_type* get_component_w(object& in_object_to_get_from)
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, component_type>::value;

			static_assert(is_valid_type, "can only get types that derive i_component");
			static_assert((policies::template verify<policy_write<component_type>>() || ...), "this system does not have policy_write of that type!");

			return m_world->get_component<component_type>(in_object_to_get_from);
		}

		template <typename component_type>
		const component_type* get_component_r(object& in_object_to_get_from) const
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, component_type>::value;

			static_assert(is_valid_type, "can only get types that derive i_component");
			static_assert((policies::template verify<policy_read<component_type>>() || ...), "this system does not have policy_read of that type!");

			return m_world->get_component<component_type>(in_object_to_get_from);
		}

		template <typename type>
		inline component_view<type, component_storage> all_w()
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, type>::value;

			static_assert(is_valid_type, "can only get types that derive i_component");
			static_assert((policies::template verify<policy_write<type>>() || ...), "this system does not have policy_write of that type!");

			return m_world->all<type>();
		}

		template <typename type>
		inline const component_view<type, const component_storage> all_r() const
		{
			constexpr bool is_valid_type = std::is_base_of<i_component, type>::value;

			static_assert(is_valid_type, "can only get types that derive i_component");
			static_assert((policies::template verify<policy_read<type>>() || ...), "this system does not have policy_read of that type!");

			return m_world->all<type>();
		}

		template <typename event_type, typename functor>
		void listen(functor in_func)
		{
			m_world->listen<event_type, functor>(std::move(in_func));
		}

		template <typename event_type, typename event_data>
		void invoke(event_data& event_data_to_send)
		{
			m_world->invoke<event_type, event_data>(event_data_to_send);
		}

		bool verify_policies_against(const i_system_base& in_sys, std::vector<std::string>& out_failure_reasons) const override
		{
			return (policies::verify_against(in_sys, out_failure_reasons) && ...);
		}

		bool has_policy(ui32 in_policy_id) const override
		{
			return ((policies::id() == in_policy_id) || ...);
		}
	};
}
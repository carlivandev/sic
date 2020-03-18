#pragma once
#include "sic/defines.h"
#include "sic/type_index.h"
#include "sic/type_restrictions.h"

#include <string>
#include <vector>

namespace sic
{
	struct Engine_context;
	struct Level_context;
	struct Engine;
	struct Component_base;
	struct Object_base;

	struct System : Noncopyable
	{
		friend Engine_context;
		friend Engine;

		/*
			happens right after a system has been created in a engine
			useful for creating subsystems, registering types, etc
		*/
		virtual void on_created(Engine_context&& in_context) { in_context; }

		/*
		*/
		virtual void on_shutdown(Engine_context&& in_context) { in_context; }

		/*
			happens after level has finished setting up
		*/
		virtual void on_begin_simulation(Level_context&& in_context) const { in_context; }

		/*
			happens after level has called on_begin_simulation
			called every frame for each root level
		*/
		virtual void on_tick(Level_context&& in_context, float in_time_delta) const { in_context; in_time_delta; }

		/*
			happens when level is destroyed
		*/
		virtual void on_end_simulation(Level_context&& in_context) const { in_context; }

		/*
			happens after engine has finished setting up
			called every frame once
		*/
		virtual void on_engine_tick(Engine_context&& in_context, float in_time_delta) const { in_context; in_time_delta; }

		//unsafe cast, see try_cast for safe version
		template <typename t_to_type, typename t_from_type>
		static __forceinline constexpr t_to_type* cast(t_from_type& in_to_cast)
		{
			static_assert(std::is_base_of<t_from_type, t_to_type>::value, "can only cast to subtype");
			return reinterpret_cast<t_to_type*>(&in_to_cast);
		}

		template <typename t_to_type, typename t_from_type>
		static __forceinline constexpr t_to_type* try_cast(t_from_type& in_to_cast)
		{
			static_assert(std::is_base_of<t_from_type, t_to_type>::value, "can only cast to subtype");

			if (!is_a<t_to_type, t_from_type>(in_to_cast))
				return nullptr;

			return reinterpret_cast<t_to_type*>(&in_to_cast);
		}

		template <typename t_type, typename t_base_type>
		static __forceinline constexpr bool is_a(t_base_type& in_to_check)
		{
			constexpr bool is_component = std::is_base_of<Component_base, t_base_type>::value;
			constexpr bool is_object = std::is_base_of<Object_base, t_base_type>::value;

			if constexpr (is_component)
			{
				const i32 type_idx = Type_index<Component_base>::get<t_base_type>();

				return type_idx == in_to_check.m_type_index;

			}
			else if constexpr (is_object)
			{
				const i32 type_idx = Type_index<Object_base>::get<t_type>();

				return type_idx == in_to_check.m_type_index;
			}
			else
			{
				static_assert(is_component || is_object, "can not do is_a of type");

				return false;
			}
		}

		/*
			a disabled system will never:
				- tick
				- recieve events
		*/
		void set_enabled(bool in_val)
		{
			m_enabled = in_val;
		}

		bool is_enabled() const { return m_enabled; }

		const std::string& name() const { return m_name; }

	protected:
		void execute_tick(Level_context&& in_context, float in_time_delta) const;
		void execute_engine_tick(Engine_context&& in_context, float in_time_delta) const;

		std::string m_name;
		bool m_enabled = true;

	private:
		std::vector<System*> m_subsystems;
		bool m_finished_tick = false;
	};
}
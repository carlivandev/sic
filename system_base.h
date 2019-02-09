#pragma once
#include <string>
#include <vector>

namespace impuls
{
	struct world;

	struct i_system_base
	{
		friend world;

		/*
			happens right after a system has been created in a world
			useful for creating subsystems
		*/
		virtual void on_created() {}

		/*
			happens after world has finished setting up
		*/
		virtual void on_begin_simulation() {}

		/*
			happens after world has called on_begin_simulation
			called every frame
		*/
		virtual void on_tick(float in_time_delta) { in_time_delta; }

		/*
			happens when world is destroyed
		*/
		virtual void on_end_simulation() {}

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
		
		virtual ~i_system_base()
		{
			m_world = nullptr;
		}

		virtual bool verify_policies_against(const i_system_base& in_sys, std::vector<std::string>& out_failure_reasons) const = 0;

		virtual bool has_policy(ui32 in_policy_id) const = 0;

	protected:
		world* m_world = nullptr;
		std::string m_name;
		bool m_enabled = true;
	};
}
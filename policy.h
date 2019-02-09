#pragma once
#include "system_base.h"

namespace impuls
{
	struct policy_id
	{
		template <typename policy>
		static ui32 get_id()
		{
			static ui32 id = m_id_ticker++;

			return id;
		}

		static ui32 m_id_ticker;
	};

	template <typename t>
	struct policy_read
	{
		static ui32 id()
		{
			return policy_id::get_id<policy_read<t>>();
		}

		template <typename other_t>
		static constexpr bool verify()
		{
			return std::is_same<policy_read<t>, other_t>::value;
		}

		static bool verify_against(const i_system_base& in_sys, std::vector<std::string>& out_failure_reasons)
		{
			in_sys; out_failure_reasons;
			//read only fails if something has write, and write already checks for read
			//therefore we can always assume true
			return true;
		}
	};

	template <typename t>
	struct policy_write
	{
		static ui32 id()
		{
			return policy_id::get_id<policy_read<t>>();
		}

		template <typename other_t>
		static constexpr bool verify()
		{
			return
				std::is_same<policy_write<t>, other_t>::value ||
				std::is_same<policy_read<t>, other_t>::value
				;
		}

		static bool verify_against(const i_system_base& in_sys, std::vector<std::string>& out_failure_reasons)
		{
			if (in_sys.has_policy(policy_read<t>::id()) || in_sys.has_policy(policy_write<t>::id()))
			{
				out_failure_reasons.push_back("conflicting read/write policies(" + std::string(typeid(t).name()) + ")");
				return false;
			}

			return true;
		}
	};
}
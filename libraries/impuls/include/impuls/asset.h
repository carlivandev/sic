#pragma once
#include "impuls/serializer.h"
#include "impuls/defines.h"
#include "impuls/leavable_queue.h"

#include "crossguid/guid.hpp"

#include <string>
#include <mutex>

namespace impuls
{
	struct state_assetsystem;

	enum class e_asset_load_state
	{
		not_loaded,
		loading,
		loaded
	};

	struct asset_header
	{
		asset_header(state_assetsystem& in_assetsystem_state) : m_assetsystem_state(&in_assetsystem_state) {}

		void increment_reference_count();
		void decrement_reference_count();

		xg::Guid m_id;
		std::string m_name;
		std::string m_typename;
		std::string m_asset_path;

		std::unique_ptr<byte> m_loaded_asset;
		e_asset_load_state m_load_state = e_asset_load_state::not_loaded;

	private:
		state_assetsystem* m_assetsystem_state = nullptr;

		std::mutex m_reference_mutex;
		ui32 m_reference_count = 0;
		leavable_queue<asset_header*>::queue_ticket m_unload_ticket;
	};

	template <typename t_type>
	struct asset_ref
	{
		friend state_assetsystem;

		asset_ref() = default;
		asset_ref(asset_header* in_header)
		{
			m_header = in_header;

			if (!m_header)
				return;

			m_header->increment_reference_count();
		}

		~asset_ref()
		{
			if (m_header)
				m_header->decrement_reference_count();
		}

		asset_ref<t_type>& operator=(const asset_ref<t_type>& in_ref)
		{
			if (m_header)
				m_header->decrement_reference_count();

			m_header = in_ref.m_header;

			if (!m_header)
				return *this;

			m_header->increment_reference_count();

			return *this;
		}

		t_type* get() const { return reinterpret_cast<t_type*>(m_header->m_loaded_asset.get()); }
		asset_header* get_header() const { return m_header; }

		bool is_valid() const { return m_header != nullptr; }
		e_asset_load_state get_load_state() const { return m_header->m_load_state; }

	private:
		asset_header* m_header = nullptr;
	};

	template <bool t_has_post_load, bool t_has_pre_unload>
	struct i_asset
	{
		static constexpr bool has_post_load() { return t_has_post_load; }
		static constexpr bool has_pre_unload() { return t_has_pre_unload; }

		virtual void load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream) = 0;
		virtual void save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const = 0;
	};
}
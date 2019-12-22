#pragma once
#include "impuls/serializer.h"
#include "impuls/defines.h"
#include "impuls/leavable_queue.h"

#include "crossguid/guid.hpp"

#include <string>
#include <mutex>

namespace sic
{
	struct State_assetsystem;
	struct Asset;

	enum class Asset_load_state
	{
		not_loaded,
		loading,
		loaded,
		unloading
	};

	struct Asset_header
	{
		friend State_assetsystem;

		Asset_header(State_assetsystem& in_assetsystem_state) : m_assetsystem_state(&in_assetsystem_state) {}

		void increment_reference_count();
		void decrement_reference_count();

		xg::Guid m_id;
		std::string m_name;
		std::string m_typename;
		std::string m_asset_path;

		std::unique_ptr<Asset> m_loaded_asset;
		Asset_load_state m_load_state = Asset_load_state::not_loaded;

	private:
		State_assetsystem* m_assetsystem_state = nullptr;

		std::mutex m_reference_mutex;
		ui32 m_reference_count = 0;
		Leavable_queue<Asset_header*>::queue_ticket m_unload_ticket;
	};

	template <typename t_type>
	struct Asset_ref
	{
		friend State_assetsystem;

		Asset_ref() = default;
		Asset_ref(Asset_header* in_header)
		{
			m_header = in_header;

			if (!m_header)
				return;

			m_header->increment_reference_count();
		}

		Asset_ref(const Asset_ref<t_type>& in_ref)
		{
			m_header = in_ref.m_header;

			if (!m_header)
				return;

			m_header->increment_reference_count();
		}

		Asset_ref(Asset_ref&& in_ref) noexcept
		{
			m_header = std::move(in_ref.m_header);
			in_ref.m_header = nullptr;
		}

		~Asset_ref()
		{
			if (m_header)
			{
				m_header->decrement_reference_count();
				m_header = nullptr;
			}
		}

		Asset_ref<t_type>& operator=(const Asset_ref<t_type>& in_ref)
		{
			if (m_header)
				m_header->decrement_reference_count();

			m_header = in_ref.m_header;

			if (!m_header)
				return *this;

			m_header->increment_reference_count();

			return *this;
		}

		Asset_ref& operator=(Asset_ref&& in_ref)
		{
			if (m_header)
			{
				m_header->decrement_reference_count();
				m_header = nullptr;
			}

			m_header = std::move(in_ref.m_header);
			in_ref.m_header = nullptr;
			return *this;
		}

		t_type* get() const { return reinterpret_cast<t_type*>(m_header->m_loaded_asset.get()); }
		Asset_header* get_header() const { return m_header; }

		bool is_valid() const { return m_header != nullptr; }
		Asset_load_state get_load_state() const { return m_header->m_load_state; }

	private:
		Asset_header* m_header = nullptr;
	};

	struct Asset
	{
		virtual ~Asset() = default;

		virtual bool has_post_load() { return false; }
		virtual bool has_pre_unload() { return false; }

		virtual void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) = 0;
		virtual void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const = 0;
	};
}
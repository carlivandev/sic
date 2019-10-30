#pragma once
#include "impuls/serializer.h"
#include "impuls/defines.h"

#include "crossguid/guid.hpp"

#include <string>

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
		xg::Guid m_id;
		std::string m_name;
		std::string m_import_path;
		std::string m_asset_path;

		std::unique_ptr<byte> m_loaded_asset;
		e_asset_load_state m_load_state = e_asset_load_state::not_loaded;
	};

	template <typename t_type>
	struct asset_ref
	{
		friend state_assetsystem;

		asset_ref() = default;
		asset_ref(asset_header* in_header) : m_header(in_header) {}

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
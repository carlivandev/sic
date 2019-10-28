#pragma once
#include "impuls/system.h"
#include "impuls/world_context.h"
#include "impuls/system_file.h"
#include "impuls/asset_management.h"
#include "impuls/file_management.h"

#include "crossguid/guid.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include <unordered_map>
#include <mutex>
#include <vector>
#include <typeindex>

namespace impuls
{
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
		friend struct state_assetsystem;

		asset_ref() = default;
		asset_ref(asset_header* in_header) : m_header(in_header) {}

		t_type* get() const { return reinterpret_cast<t_type*>(m_header->m_loaded_asset.get()); }

		bool is_valid() const { return m_header != nullptr; }
		e_asset_load_state get_load_state() const { return m_header->m_load_state; }

	private:
		asset_header* m_header = nullptr;
	};

	struct state_assetsystem : i_state
	{
		friend struct system_asset;

		template <typename t_type>
		asset_ref<t_type> find_asset(const xg::Guid& in_id) const
		{
			auto header_it = m_id_to_header.find(in_id);
			return header_it != m_id_to_header.end() ? asset_ref<t_type>(header_it->second) : asset_ref<t_type>();
		}

		template <typename t_type>
		void load_batch(world_context in_context, const std::vector<asset_ref<t_type>>& in_batch_to_load)
		{
			std::scoped_lock lock(m_mutex);
			load_requests.reserve(in_batch_to_load.size());

			for (auto& ref : in_batch_to_load)
			{
				asset_header* header = ref.m_header;

				if (header->m_load_state != e_asset_load_state::not_loaded)
					continue;

				header->m_load_state = e_asset_load_state::loading;

				load_requests.push_back
				(
					file_load_request
					(
						std::string(header->m_asset_path),
						[header, this](std::string&& in_loaded_data)
						{
							asset_management::load_asset<t_type>(std::move(in_loaded_data), header->m_loaded_asset);
							if (t_type::has_post_load())
							{
								std::scoped_lock post_load_lock(m_post_load_mutex);

								std::unique_ptr<std::vector<asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_type))];

								if (!post_load_assets)
									post_load_assets = std::make_unique<std::vector<asset_header*>>();

								post_load_assets->push_back(header);
							}
							else
							{
								header->m_load_state = e_asset_load_state::loaded;
							}
						}
					)
				);

				in_context.get_state<state_filesystem>()->request_load(std::move(load_requests));
			}
		}

		template <typename t_asset_type>
		inline void do_post_load(std::function<void(asset_ref<t_asset_type>&&)>&& in_callback)
		{
			std::scoped_lock post_load_lock(m_post_load_mutex);

			std::unique_ptr<std::vector<asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_asset_type))];

			if (!post_load_assets)
				return;

			for (asset_header* header : *post_load_assets.get())
			{
				in_callback(std::move(asset_ref<t_asset_type>(header)));
				header->m_load_state = e_asset_load_state::loaded;
			}

			post_load_assets->clear();
		}

		template <typename t_asset_type>
		inline asset_header* create_asset(const std::string& in_asset_name, const std::string& in_asset_directory)
		{
			return create_asset_internal(in_asset_name, in_asset_directory, typeid(t_asset_type).name());
		}

		template <typename t_asset_type>
		inline void import_asset(const std::string& in_import_path, asset_header& out_header)
		{
			out_header.m_import_path = in_import_path;
			out_header.m_loaded_asset = asset_management::process_asset<t_asset_type>(file_management::load_file(in_import_path));
		}

		asset_header* create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename);

		template <typename t_asset_type>
		inline void save_asset(const asset_header& in_header)
		{
			save_asset_header(in_header, typeid(t_asset_type).name());
			asset_management::save_asset<t_asset_type>(in_header.m_loaded_asset, in_header.m_asset_path);
		}

		void save_asset_header(const asset_header& in_header, const std::string& in_typename);

	private:
		std::vector<std::unique_ptr<asset_header>> m_asset_headers;
		std::unordered_map<xg::Guid, asset_header*> m_id_to_header;
		std::vector<file_load_request> load_requests;

		std::unordered_map<std::type_index, std::unique_ptr<std::vector<asset_header*>>> m_typeindex_to_post_load_assets;

		std::mutex m_mutex;
		std::mutex m_post_load_mutex;
	};

	struct system_asset : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
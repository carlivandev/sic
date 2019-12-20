#pragma once
#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/level_context.h"
#include "impuls/system_file.h"
#include "impuls/file_management.h"
#include "impuls/asset.h"
#include "impuls/leavable_queue.h"
#include "impuls/logger.h"

#include "crossguid/guid.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include <unordered_map>
#include <mutex>
#include <vector>
#include <typeindex>

namespace sic
{
	struct State_assetsystem : State
	{
		friend struct System_asset;

		template <typename t_type>
		Asset_ref<t_type> find_asset(const xg::Guid& in_id) const
		{
			auto header_it = m_id_to_header.find(in_id);
			return header_it != m_id_to_header.end() ? Asset_ref<t_type>(header_it->second) : Asset_ref<t_type>();
		}

		template <typename t_type>
		void load_batch(Engine_context in_context, std::vector<Asset_ref<t_type>>&& in_batch_to_load)
		{
			std::scoped_lock lock(m_mutex);
			load_requests.reserve(in_batch_to_load.size());

			for (auto& ref : in_batch_to_load)
			{
				Asset_header* header = ref.m_header;

				if (header->m_load_state != Asset_load_state::not_loaded)
					continue;

				header->m_load_state = Asset_load_state::loading;
				header->increment_reference_count();

				load_requests.push_back
				(
					File_load_request
					(
						std::string(header->m_asset_path),
						[header, this](std::string&& in_loaded_data)
						{
							load_asset_internal<t_type>(std::move(in_loaded_data), header->m_loaded_asset);
							if (header->m_loaded_asset.get()->has_post_load())
							{
								std::scoped_lock post_load_lock(m_post_load_mutex);

								std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_type))];

								if (!post_load_assets)
									post_load_assets = std::make_unique<std::vector<Asset_header*>>();

								post_load_assets->push_back(header);
							}
							else
							{
								IMPULS_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", header->m_name.c_str());
								header->m_load_state = Asset_load_state::loaded;
								header->decrement_reference_count();
							}
						}
					)
				);

				in_context.get_state<State_filesystem>()->request_load(std::move(load_requests));
			}
		}

		template <typename t_asset_type>
		inline void do_post_load(std::function<void(t_asset_type&)>&& in_callback)
		{
			std::scoped_lock post_load_lock(m_post_load_mutex);

			std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_asset_type))];

			if (!post_load_assets)
				return;

			for (Asset_header* header : *post_load_assets.get())
			{
				IMPULS_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", header->m_name.c_str());

				in_callback(*reinterpret_cast<t_asset_type*>(header->m_loaded_asset.get()));
				header->m_load_state = Asset_load_state::loaded;
				header->decrement_reference_count();
			}

			post_load_assets->clear();
		}

		template <typename t_asset_type>
		inline void do_pre_unload(std::function<void(t_asset_type&)>&& in_callback)
		{
			std::scoped_lock pre_unload_lock(m_pre_unload_mutex);

			std::unique_ptr<std::vector<Asset_header*>>& pre_unload_assets = m_typename_to_pre_unload_headers[typeid(t_asset_type).name()];

			if (!pre_unload_assets)
				return;

			for (Asset_header* header : *pre_unload_assets.get())
			{
				IMPULS_LOG(g_log_asset_verbose, "unloaded asset: \"{0}\"", header->m_name.c_str());

				in_callback(*reinterpret_cast<t_asset_type*>(header->m_loaded_asset.get()));
				header->m_load_state = Asset_load_state::not_loaded;
				header->m_loaded_asset.reset();
			}

			pre_unload_assets->clear();
		}

		template <typename t_asset_type>
		inline Asset_ref<t_asset_type> create_asset(const std::string& in_asset_name, const std::string& in_asset_directory)
		{
			Asset_header* new_header = create_asset_internal(in_asset_name, in_asset_directory, typeid(t_asset_type).name());
			new_header->m_loaded_asset = std::unique_ptr<Asset>(reinterpret_cast<Asset*>(new t_asset_type()));

			new_header->increment_reference_count();

			IMPULS_LOG(g_log_asset_verbose, "Created asset: \"{0}\"", new_header->m_name.c_str());

			if (new_header->m_loaded_asset.get()->has_post_load())
			{
				std::scoped_lock post_load_lock(m_post_load_mutex);
				new_header->m_load_state = Asset_load_state::loading;

				std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_asset_type))];

				if (!post_load_assets)
					post_load_assets = std::make_unique<std::vector<Asset_header*>>();

				post_load_assets->push_back(new_header);
			}
			else
			{
				IMPULS_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", new_header->m_name.c_str());
				new_header->m_load_state = Asset_load_state::loaded;
			}

			return Asset_ref<t_asset_type>(new_header);
		}

		template <typename t_asset_type>
		inline void save_asset(const Asset_header& in_header)
		{
			save_asset_header(in_header, typeid(t_asset_type).name());
			save_asset_internal<t_asset_type>(in_header.m_loaded_asset, in_header.m_asset_path);
		}

		void leave_unload_queue(const Asset_header& in_header);
		void join_unload_queue(Asset_header& in_out_header);

		void force_unload_unreferenced_assets();

	private:
		Asset_header* create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename);
		void save_asset_header(const Asset_header& in_header, const std::string& in_typename);

		template <typename t_asset_type>
		inline void load_asset_internal(std::string&& in_asset_data, std::unique_ptr<Asset>& out)
		{
			if (in_asset_data.empty())
			{
				IMPULS_LOG_E(g_log_asset_verbose, "Failed to load material!");
				return;
			}

			t_asset_type* new_asset = new t_asset_type();

			Deserialize_stream stream(std::move(in_asset_data));
			new_asset->load(*this, stream);

			out = std::move(std::unique_ptr<Asset>(reinterpret_cast<Asset*>(new_asset)));
		}

		template <typename t_asset_type>
		inline void save_asset_internal(const std::unique_ptr<Asset>& in_loaded_data, const std::string& in_asset_path)
		{
			const t_asset_type* asset = reinterpret_cast<const t_asset_type*>(in_loaded_data.get());

			if (!asset)
				return;

			Serialize_stream stream;
			asset->save(*this, stream);

			File_management::save_file(in_asset_path, stream.m_bytes);
		}

	private:
		void unload_next_asset();

		std::vector<std::unique_ptr<Asset_header>> m_asset_headers;
		std::unordered_map<xg::Guid, Asset_header*> m_id_to_header;

		std::vector<File_load_request> load_requests;
		Leavable_queue<Asset_header*> m_unload_queue;

		std::unordered_map<std::type_index, std::unique_ptr<std::vector<Asset_header*>>> m_typeindex_to_post_load_assets;
		std::unordered_map<std::string, std::unique_ptr<std::vector<Asset_header*>>> m_typename_to_pre_unload_headers;

		std::mutex m_mutex;
		std::mutex m_post_load_mutex;
		std::mutex m_pre_unload_mutex;
	};

	struct System_asset : System
	{
		virtual void on_created(Engine_context&& in_context) override;
	};
}
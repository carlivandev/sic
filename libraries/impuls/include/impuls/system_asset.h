#pragma once
#include "impuls/system.h"
#include "impuls/world_context.h"
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

namespace impuls
{
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
							load_asset_internal<t_type>(std::move(in_loaded_data), header->m_loaded_asset);
							if (header->m_loaded_asset.get()->has_post_load())
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
		inline void do_pre_unload(std::function<void(asset_ref<t_asset_type>&&)>&& in_callback)
		{
			std::scoped_lock pre_unload_lock(m_pre_unload_mutex);

			std::unique_ptr<std::vector<asset_header*>>& pre_unload_assets = m_typename_to_pre_unload_headers[typeid(t_asset_type).name()];

			if (!pre_unload_assets)
				return;

			for (asset_header* header : *pre_unload_assets.get())
			{
				in_callback(std::move(asset_ref<t_asset_type>(header)));
				header->m_load_state = e_asset_load_state::not_loaded;
				header->m_loaded_asset.reset();
			}

			pre_unload_assets->clear();
		}

		template <typename t_asset_type>
		inline asset_ref<t_asset_type> create_asset(const std::string& in_asset_name, const std::string& in_asset_directory)
		{
			asset_header* new_header = create_asset_internal(in_asset_name, in_asset_directory, typeid(t_asset_type).name());
			new_header->m_loaded_asset = std::unique_ptr<i_asset>(reinterpret_cast<i_asset*>(new t_asset_type()));

			if (new_header->m_loaded_asset.get()->has_post_load())
			{
				std::scoped_lock post_load_lock(m_post_load_mutex);
				new_header->m_load_state = e_asset_load_state::loading;

				std::unique_ptr<std::vector<asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(t_asset_type))];

				if (!post_load_assets)
					post_load_assets = std::make_unique<std::vector<asset_header*>>();

				post_load_assets->push_back(new_header);
			}
			else
			{
				new_header->m_load_state = e_asset_load_state::loaded;
			}
			

			return asset_ref<t_asset_type>(new_header);
		}

		template <typename t_asset_type>
		inline void save_asset(const asset_header& in_header)
		{
			save_asset_header(in_header, typeid(t_asset_type).name());
			save_asset_internal<t_asset_type>(in_header.m_loaded_asset, in_header.m_asset_path);
		}

		void leave_unload_queue(const asset_header& in_header);
		void join_unload_queue(asset_header& in_out_header);

	private:
		asset_header* create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename);
		void save_asset_header(const asset_header& in_header, const std::string& in_typename);

		template <typename t_asset_type>
		inline void load_asset_internal(std::string&& in_asset_data, std::unique_ptr<i_asset>& out)
		{
			if (in_asset_data.empty())
			{
				IMPULS_LOG_E(g_log_asset, "Failed to load material!");
				return;
			}

			t_asset_type* new_asset = new t_asset_type();

			deserialize_stream stream(std::move(in_asset_data));
			new_asset->load(*this, stream);

			out = std::move(std::unique_ptr<i_asset>(reinterpret_cast<i_asset*>(new_asset)));
		}

		template <typename t_asset_type>
		inline void save_asset_internal(const std::unique_ptr<i_asset>& in_loaded_data, const std::string& in_asset_path)
		{
			const t_asset_type* asset = reinterpret_cast<const t_asset_type*>(in_loaded_data.get());

			if (!asset)
				return;

			serialize_stream stream;
			asset->save(*this, stream);

			file_management::save_file(in_asset_path, stream.m_bytes);
		}

	private:
		std::vector<std::unique_ptr<asset_header>> m_asset_headers;
		std::unordered_map<xg::Guid, asset_header*> m_id_to_header;

		std::vector<file_load_request> load_requests;
		leavable_queue<asset_header*> m_unload_queue;

		std::unordered_map<std::type_index, std::unique_ptr<std::vector<asset_header*>>> m_typeindex_to_post_load_assets;
		std::unordered_map<std::string, std::unique_ptr<std::vector<asset_header*>>> m_typename_to_pre_unload_headers;

		std::mutex m_mutex;
		std::mutex m_post_load_mutex;
		std::mutex m_pre_unload_mutex;
	};

	struct system_asset : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
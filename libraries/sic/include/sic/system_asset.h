#pragma once
#include "sic/system.h"
#include "sic/engine_context.h"
#include "sic/level_context.h"
#include "sic/system_file.h"
#include "sic/file_management.h"
#include "sic/asset.h"
#include "sic/leavable_queue.h"
#include "sic/logger.h"
#include "sic/delegate.h"

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
		friend Asset_header;

		template <typename T_type>
		Asset_ref<T_type> find_asset(const xg::Guid& in_id) const
		{
			auto header_it = m_id_to_header.find(in_id);
			return header_it != m_id_to_header.end() ? Asset_ref<T_type>(header_it->second) : Asset_ref<T_type>();
		}

		template <typename T_asset_type>
		void load_asset(Asset_header& in_header)
		{
			std::scoped_lock lock(m_mutex);

			if (in_header.m_load_state != Asset_load_state::not_loaded)
				return;

			in_header.m_load_state = Asset_load_state::loading;
			in_header.increment_reference_count();

			m_filesystem_state->request_load
			(
				File_load_request
				(
					std::string(in_header.m_asset_path),
					[&in_header, this](std::string&& in_loaded_data)
					{
						load_asset_internal<T_asset_type>(std::move(in_loaded_data), in_header.m_loaded_asset);
						if (in_header.m_loaded_asset.get()->has_post_load())
						{
							std::scoped_lock post_load_lock(m_post_load_mutex);

							std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(T_asset_type))];

							if (!post_load_assets)
								post_load_assets = std::make_unique<std::vector<Asset_header*>>();

							post_load_assets->push_back(&in_header);
						}
						else
						{
							m_headers_to_mark_as_loaded.push_back(&in_header);
						}
					}
				)
			);

		}

		template <typename T_asset_type>
		inline void do_post_load(std::function<void(T_asset_type&)>&& in_callback)
		{
			std::scoped_lock lock(m_mutex);
			std::scoped_lock post_load_lock(m_post_load_mutex);

			std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(T_asset_type))];

			if (!post_load_assets)
				return;

			for (Asset_header* header : *post_load_assets.get())
			{
				in_callback(*reinterpret_cast<T_asset_type*>(header->m_loaded_asset.get()));
				m_headers_to_mark_as_loaded.push_back(header);
			}

			post_load_assets->clear();
		}

		template <typename T_asset_type>
		inline void do_pre_unload(std::function<void(T_asset_type&)>&& in_callback)
		{
			std::scoped_lock pre_unload_lock(m_pre_unload_mutex);

			std::unique_ptr<std::vector<Asset_header*>>& pre_unload_assets = m_typename_to_pre_unload_headers[typeid(T_asset_type).name()];

			if (!pre_unload_assets)
				return;

			for (Asset_header* header : *pre_unload_assets.get())
			{
				SIC_LOG(g_log_asset_verbose, "unloaded asset: \"{0}\"", header->m_name.c_str());

				in_callback(*reinterpret_cast<T_asset_type*>(header->m_loaded_asset.get()));
				header->m_load_state = Asset_load_state::not_loaded;
				header->m_loaded_asset.reset();
			}

			pre_unload_assets->clear();
		}

		template <typename T_asset_type>
		inline void for_each(std::function<void(T_asset_type&)>&& in_callback)
		{
			std::scoped_lock lock(m_mutex);

			for (auto& header : m_asset_headers)
			{
				if (header->m_load_state != Asset_load_state::loaded)
					continue;

				if (header->m_typename == typeid(T_asset_type).name())
					in_callback(*reinterpret_cast<T_asset_type*>(header->m_loaded_asset.get()));
			}
		}

		template <typename T_asset_type>
		inline Asset_ref<T_asset_type> create_asset(const std::string& in_asset_name, const std::string& in_asset_directory)
		{
			Asset_header* new_header = create_asset_internal(in_asset_name, in_asset_directory, typeid(T_asset_type).name());
			new_header->m_loaded_asset = std::unique_ptr<Asset>(reinterpret_cast<Asset*>(new T_asset_type()));

			new_header->increment_reference_count();

			SIC_LOG(g_log_asset_verbose, "Created asset: \"{0}\"", new_header->m_name.c_str());

			if (new_header->m_loaded_asset.get()->has_post_load())
			{
				std::scoped_lock post_load_lock(m_post_load_mutex);
				new_header->m_load_state = Asset_load_state::loading;

				std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typeindex_to_post_load_assets[std::type_index(typeid(T_asset_type))];

				if (!post_load_assets)
					post_load_assets = std::make_unique<std::vector<Asset_header*>>();

				post_load_assets->push_back(new_header);
			}
			else
			{
				SIC_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", new_header->m_name.c_str());
				new_header->m_load_state = Asset_load_state::loaded;
			}

			return Asset_ref<T_asset_type>(new_header);
		}

		template <typename T_asset_type>
		inline void save_asset(const Asset_header& in_header)
		{
			save_asset_header(in_header, typeid(T_asset_type).name());
			save_asset_internal<T_asset_type>(in_header.m_loaded_asset, in_header.m_asset_path);
		}


		void force_unload_unreferenced_assets();

	private:
		void leave_unload_queue(const Asset_header& in_header);
		void join_unload_queue(Asset_header& in_out_header);

		Asset_header* create_asset_internal(const std::string& in_asset_name, const std::string& in_asset_directory, const std::string& in_typename);
		void save_asset_header(const Asset_header& in_header, const std::string& in_typename);

		template <typename T_asset_type>
		inline void load_asset_internal(std::string&& in_asset_data, std::unique_ptr<Asset>& out)
		{
			if (in_asset_data.empty())
			{
				SIC_LOG_E(g_log_asset_verbose, "Failed to load material!");
				return;
			}

			T_asset_type* new_asset = new T_asset_type();

			Deserialize_stream stream(std::move(in_asset_data));
			new_asset->load(*this, stream);

			out = std::move(std::unique_ptr<Asset>(reinterpret_cast<Asset*>(new_asset)));
		}

		template <typename T_asset_type>
		inline void save_asset_internal(const std::unique_ptr<Asset>& in_loaded_data, const std::string& in_asset_path)
		{
			const T_asset_type* asset = reinterpret_cast<const T_asset_type*>(in_loaded_data.get());

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

		std::vector<File_load_request> m_load_requests;
		Leavable_queue<Asset_header*> m_unload_queue;

		std::unordered_map<std::type_index, std::unique_ptr<std::vector<Asset_header*>>> m_typeindex_to_post_load_assets;
		std::unordered_map<std::string, std::unique_ptr<std::vector<Asset_header*>>> m_typename_to_pre_unload_headers;

		std::vector<Asset_header*> m_headers_to_mark_as_loaded;

		std::mutex m_mutex;
		std::mutex m_post_load_mutex;
		std::mutex m_pre_unload_mutex;

		State_filesystem* m_filesystem_state = nullptr;
	};

	struct System_asset : System
	{
		virtual void on_created(Engine_context in_context) override;
		void on_engine_finalized(Engine_context in_context) const override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
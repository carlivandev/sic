#pragma once
#include "sic/system_file.h"
#include "sic/file_management.h"
#include "sic/asset.h"

#include "sic/core/system.h"
#include "sic/core/engine_context.h"
#include "sic/core/scene_context.h"
#include "sic/core/leavable_queue.h"
#include "sic/core/logger.h"
#include "sic/core/delegate.h"

#include "crossguid/guid.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include <unordered_map>
#include <mutex>
#include <vector>
#include <typeindex>
#include <functional>

namespace sic
{
	struct State_assetsystem : State
	{
		friend struct System_asset;
		friend Asset_header;

		template <typename T_asset_type>
		void register_type()
		{
			m_typename_to_load_function[typeid(T_asset_type).name()] =
			[this](std::string&& in_asset_data, std::unique_ptr<Asset>& out)
			{
				load_asset_internal<T_asset_type>(std::move(in_asset_data), out);
			};
		}

		template <typename T_type>
		Asset_ref<T_type> find_asset(const xg::Guid& in_id) const
		{
			auto header_it = m_id_to_header.find(in_id);
			return header_it != m_id_to_header.end() ? Asset_ref<T_type>(header_it->second) : Asset_ref<T_type>();
		}

		void load_asset(Asset_header& in_header)
		{
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
						auto load_func_it = m_typename_to_load_function.find(in_header.m_typename);
						assert(load_func_it != m_typename_to_load_function.end() && "Asset type was not registered with the asset system!");
						load_func_it->second(std::move(in_loaded_data), in_header.m_loaded_asset);

						in_header.m_loaded_asset->m_header = &in_header;

						if (in_header.m_loaded_asset.get()->has_post_load())
						{
							std::scoped_lock post_load_lock(m_post_load_mutex);

							std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typename_to_post_load_assets[in_header.m_typename];

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
			std::scoped_lock post_load_lock(m_post_load_mutex);

			std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typename_to_post_load_assets[typeid(T_asset_type).name()];

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
				SIC_LOG(g_log_asset, "Unloaded asset: \"{0}\"", header->m_name.c_str());

				in_callback(*reinterpret_cast<T_asset_type*>(header->m_loaded_asset.get()));
				header->m_load_state = Asset_load_state::not_loaded;
				header->m_loaded_asset.reset();

				//this asset has not been saved, remove header completely
				if (header->m_asset_path.empty())
				{
					m_id_to_header.erase(header->m_id);

					m_free_header_indices.push_back(header->m_index.value());
					m_asset_headers[header->m_index.value()].reset();
				}
			}

			pre_unload_assets->clear();
		}

		template <typename T_asset_type>
		inline void for_each(std::function<void(T_asset_type&)>&& in_callback)
		{
			for (auto& header : m_asset_headers)
			{
				if (header->m_load_state != Asset_load_state::loaded)
					continue;

				if (header->m_typename == typeid(T_asset_type).name())
					in_callback(*reinterpret_cast<T_asset_type*>(header->m_loaded_asset.get()));
			}
		}

		template <typename T_asset_type>
		inline Asset_ref<T_asset_type> create_asset(const std::string& in_asset_name)
		{
			Asset_header* new_header = create_asset_internal(in_asset_name, typeid(T_asset_type).name());
			new_header->m_loaded_asset = std::unique_ptr<Asset>(reinterpret_cast<Asset*>(new T_asset_type()));
			new_header->m_loaded_asset->m_header = new_header;

			SIC_LOG(g_log_asset_verbose, "Created asset: \"{0}\"", new_header->m_name.c_str());

			if (new_header->m_loaded_asset.get()->has_post_load())
			{
				std::scoped_lock post_load_lock(m_post_load_mutex);

				new_header->increment_reference_count();
				new_header->m_load_state = Asset_load_state::loading;

				std::unique_ptr<std::vector<Asset_header*>>& post_load_assets = m_typename_to_post_load_assets[typeid(T_asset_type).name()];

				if (!post_load_assets)
					post_load_assets = std::make_unique<std::vector<Asset_header*>>();

				post_load_assets->push_back(new_header);
			}
			else
			{
				SIC_LOG(g_log_asset, "Loaded asset: \"{0}\"", new_header->m_name.c_str());
				new_header->m_load_state = Asset_load_state::loaded;
			}

			return Asset_ref<T_asset_type>(new_header);
		}

		template <typename T_asset_type>
		inline void save_asset(const Asset_ref<T_asset_type>& in_ref, const std::string& in_asset_directory)
		{
			Asset_header* header = in_ref.get_header();

			assert(header && "Asset_ref was not valid!");
			assert(header->m_loaded_asset && "Asset was not yet loaded!");
			header->m_asset_path = fmt::format("{0}/{1}.asset", in_asset_directory, header->m_name);

			save_asset_header(*header, typeid(T_asset_type).name());
			save_asset_internal<T_asset_type>(header->m_loaded_asset, header->m_asset_path);
		}

		template <typename T_asset_type>
		inline void modify_asset(const Asset_ref<T_asset_type>& in_asset_ref, std::function<void(T_asset_type*)> in_modification_functor)
		{
			std::scoped_lock lock(m_modification_mutex);
			in_modification_functor(in_asset_ref.get_mutable());
		}

		std::mutex& get_modification_mutex()
		{
			return m_modification_mutex;
		}

		void force_unload_unreferenced_assets();

	private:
		void leave_unload_queue(const Asset_header& in_header);
		void join_unload_queue(Asset_header& in_out_header);

		Asset_header* create_asset_internal(const std::string& in_asset_name, const std::string& in_typename);
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
		std::vector<size_t> m_free_header_indices;

		std::unordered_map<xg::Guid, Asset_header*> m_id_to_header;

		std::vector<File_load_request> m_load_requests;
		Leavable_queue<Asset_header*> m_unload_queue;

		std::unordered_map<std::string, std::unique_ptr<std::vector<Asset_header*>>> m_typename_to_post_load_assets;
		std::unordered_map<std::string, std::unique_ptr<std::vector<Asset_header*>>> m_typename_to_pre_unload_headers;
		std::unordered_map<std::string, std::function<void(std::string&& in_asset_data, std::unique_ptr<Asset>& out)>> m_typename_to_load_function;

		std::vector<Asset_header*> m_headers_to_mark_as_loaded;

		std::mutex m_post_load_mutex;
		std::mutex m_pre_unload_mutex;
		std::mutex m_modification_mutex;

		State_filesystem* m_filesystem_state = nullptr;
	};

	struct System_asset : System
	{
		virtual void on_created(Engine_context in_context) override;
		void on_engine_finalized(Engine_context in_context) const override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

		static void update_assetsystem(Engine_processor<Processor_flag_write<State_assetsystem>> in_processor);
	};
}
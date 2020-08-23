#pragma once
#include "sic/serializer.h"
#include "sic/defines.h"
#include "sic/leavable_queue.h"
#include "sic/type_restrictions.h"
#include "sic/delegate.h"
#include "sic/logger.h"

#include "crossguid/guid.hpp"

#include <string>
#include <mutex>

namespace sic
{
	struct State_assetsystem;
	struct Asset;

	enum struct Asset_load_state
	{
		not_loaded,
		loading,
		loaded,
		unloading
	};

	struct On_asset_loaded : Delegate<Asset*> {};

	struct Asset_header
	{
		template <typename T_type>
		friend struct Asset_ref;

		friend struct Asset;

		friend State_assetsystem;

		Asset_header(State_assetsystem& in_assetsystem_state) : m_assetsystem_state(&in_assetsystem_state) {}

		void increment_reference_count();
		void decrement_reference_count();

		ui32 get_reference_count() const { return m_reference_count; }

		xg::Guid m_id;
		std::string m_name;
		std::string m_typename;
		std::string m_asset_path;

		std::unique_ptr<Asset> m_loaded_asset;
		Asset_load_state m_load_state = Asset_load_state::not_loaded;
		On_asset_loaded m_on_loaded_delegate;

	private:
		State_assetsystem* m_assetsystem_state = nullptr;

		std::mutex m_reference_mutex;
		ui32 m_reference_count = 0;
		Leavable_queue<Asset_header*>::Queue_ticket m_unload_ticket;
	};

	struct Asset_ref_base
	{
		template <typename T_type>
		friend struct Asset_ref;

		friend State_assetsystem;

	private:
		Asset_header* m_header = nullptr;
		On_asset_loaded::Handle m_on_loaded_handle;
	};

	template <typename T_type>
	struct Asset_ref : Asset_ref_base
	{
		friend State_assetsystem;

		Asset_ref() = default;
		Asset_ref(Asset_header* in_header)
		{
			m_header = in_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		Asset_ref(T_type* in_loaded_asset)
		{
			m_header = in_loaded_asset->m_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		template <typename T_other_type>
		Asset_ref(const Asset_ref<T_other_type>& in_ref)
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only assign if both derive from same parent/child hierarchy");

			m_header = in_ref.m_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		template <typename T_other_type>
		Asset_ref(Asset_ref<T_other_type>&& in_ref) noexcept
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only assign if both derive from same parent/child hierarchy");

			m_header = std::move(in_ref.m_header);
			m_on_loaded_handle = std::move(in_ref.m_on_loaded_handle);
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

		template <typename T_other_type>
		Asset_ref<T_type>& operator=(const Asset_ref<T_other_type>& in_ref)
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only assign if both derive from same parent/child hierarchy");

			if (m_header)
				m_header->decrement_reference_count();

			m_header = in_ref.m_header;

			if (!m_header)
				return *this;

			increment_header_and_load();

			return *this;
		}

		template <typename T_other_type>
		Asset_ref& operator=(Asset_ref<T_other_type>&& in_ref) noexcept
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only assign if both derive from same parent/child hierarchy");

			if (m_header)
			{
				m_header->decrement_reference_count();
				m_header = nullptr;
			}

			m_header = std::move(in_ref.m_header);
			m_on_loaded_handle = std::move(in_ref.m_on_loaded_handle);

			in_ref.m_header = nullptr;
			return *this;
		}

		template <typename T_other_type>
		bool operator==(const Asset_ref<T_other_type>& in_other) const
		{
			return m_header == in_other.m_header;
		}

		void bind_on_loaded(On_asset_loaded::Signature in_callback)
		{
			m_on_loaded_handle.set_callback(in_callback);

			if (m_header && get_load_state() != Asset_load_state::loaded)
				m_header->m_on_loaded_delegate.try_bind(m_on_loaded_handle);
		}

		const T_type* get() const { return m_header ? reinterpret_cast<const T_type*>(m_header->m_loaded_asset.get()) : nullptr; }
		//Carl: unsafe use assetsystem_state->modify_asset for threadsafe modification
		T_type* get_mutable() const { return m_header ? reinterpret_cast<T_type*>(m_header->m_loaded_asset.get()) : nullptr; }
		Asset_header* get_header() const { return m_header; }

		bool is_valid() const { return m_header != nullptr; }
		Asset_load_state get_load_state() const { return m_header->m_load_state; }

		void deserialize(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
		{
			std::string asset_guid;
			in_stream.read(asset_guid);

			if (!asset_guid.empty())
				operator=(in_assetsystem_state.find_asset<T_type>(xg::Guid(asset_guid)));
		}

		void serialize(Serialize_stream& out_stream) const
		{
			out_stream.write(is_valid() ? get_header()->m_id.str() : std::string(""));
		}

	private:

		void increment_header_and_load()
		{
			if (!m_header)
				return;

			m_header->increment_reference_count();

			if (m_header->m_reference_count == 1)
				m_header->m_assetsystem_state->load_asset(*m_header);
		}
	};

	struct Asset : Noncopyable
	{
		friend struct Asset_dependency_gatherer;
		friend State_assetsystem;

		template <typename T>
		friend struct Asset_ref;

		virtual ~Asset() = default;

		virtual bool has_post_load() { return false; }
		virtual bool has_pre_unload() { return false; }

		virtual void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) = 0;
		virtual void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const = 0;

		virtual void get_dependencies(Asset_dependency_gatherer& out_dependencies) { out_dependencies; }

		const Asset_header& get_header() const { assert(m_header); return *m_header; }

	protected:
		State_assetsystem* get_assetsystem() const { return m_header ? m_header->m_assetsystem_state : nullptr; }

	private:
		std::vector<On_asset_loaded::Handle> m_dependency_loaded_handles;
		Asset_header* m_header = nullptr;
		ui32 m_dependencies_loading = 0;
	};

	struct Asset_dependency_gatherer
	{
		Asset_dependency_gatherer(State_assetsystem& inout_assetsystem, Asset_header& inout_header) :
			m_assetsystem(inout_assetsystem), m_header(inout_header){}

		template <typename T_asset_type>
		__forceinline void add_dependency(Asset& inout_asset, const Asset_ref<T_asset_type>& in_ref)
		{
			Asset_header* header = in_ref.get_header();

			if (!header)
				return;

			if (in_ref.get_load_state() == Asset_load_state::loaded)
				return;

			auto& handle = inout_asset.m_dependency_loaded_handles.emplace_back();
			handle.set_callback
			(
				[&dependencies_loading = inout_asset.m_dependencies_loading, &this_header = m_header](Asset*)
				{
					--dependencies_loading;
					if (dependencies_loading == 0)
					{
						SIC_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", this_header.m_name.c_str());
						this_header.m_load_state = Asset_load_state::loaded;
						this_header.m_on_loaded_delegate.invoke(this_header.m_loaded_asset.get());
						this_header.decrement_reference_count();
					}
				}
			);

			header->m_on_loaded_delegate.try_bind(handle);

			++inout_asset.m_dependencies_loading;
			m_dependencies_loading = inout_asset.m_dependencies_loading;
		}

		bool has_dependencies_to_load() const { return m_dependencies_loading > 0; }

		State_assetsystem& m_assetsystem;
		Asset_header& m_header;

		ui32 m_dependencies_loading = 0;
	};
}
#pragma once
#include "sic/core/serializer.h"
#include "sic/core/defines.h"
#include "sic/core/leavable_queue.h"
#include "sic/core/type_restrictions.h"
#include "sic/core/delegate.h"
#include "sic/core/logger.h"
#include "sic/core/scene_context.h"

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

	using Asset_loaded_processor = Engine_processor<Processor_flag_write<State_assetsystem>>;
	struct On_asset_loaded : Delegate<Asset_loaded_processor, Asset*> {};

	struct Asset_header
	{
		template <typename T_type>
		friend struct Asset_ref;
		friend struct Asset_ref_base;

		friend struct Asset;
		friend struct System_asset;
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
		std::optional<Leavable_queue<Asset_header*>::Queue_ticket> m_unload_ticket;

		std::optional<size_t> m_index;
	};

	struct Asset_ref_base
	{
		template <typename T_type>
		friend struct Asset_ref;

		friend State_assetsystem;

	protected:
		void increment_header_and_load();

	private:
		Asset_header* m_header = nullptr;
		On_asset_loaded::Handle m_on_loaded_handle;
		bool m_was_incremented = false;
	};

	template <typename T_type>
	struct Asset_ref : Asset_ref_base
	{
		friend State_assetsystem;

		Asset_ref() = default;
		explicit Asset_ref(Asset_header* in_header)
		{
			m_header = in_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		explicit Asset_ref(T_type* in_loaded_asset)
		{
			m_header = in_loaded_asset->m_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		Asset_ref(const Asset_ref<T_type>& in_ref)
		{
			m_header = in_ref.m_header;

			if (!m_header)
				return;

			increment_header_and_load();
		}

		Asset_ref(Asset_ref<T_type>&& in_ref) noexcept
		{
			m_header = std::move(in_ref.m_header);
			m_on_loaded_handle = std::move(in_ref.m_on_loaded_handle);
			m_was_incremented = std::move(in_ref.m_was_incremented);

			in_ref.m_was_incremented = false;
			in_ref.m_header = nullptr;
		}

		Asset_ref& operator=(const Asset_ref<T_type>& in_ref)
		{
			if (m_header)
			{
				assert(m_was_incremented && "should not be possible to be here without incrementing!!");
				m_header->decrement_reference_count();
			}

			m_header = in_ref.m_header;

			if (!m_header)
				return *this;

			increment_header_and_load();

			return *this;
		}

		Asset_ref& operator=(Asset_ref<T_type>&& in_ref) noexcept
		{
			if (m_header)
			{
				assert(m_was_incremented && "should not be possible to be here without incrementing!!");
				m_header->decrement_reference_count();
				m_header = nullptr;
			}

			m_header = std::move(in_ref.m_header);
			m_on_loaded_handle = std::move(in_ref.m_on_loaded_handle);
			m_was_incremented = std::move(in_ref.m_was_incremented);

			in_ref.m_was_incremented = false;
			in_ref.m_header = nullptr;
			return *this;
		}

		template <typename T_other_type>
		Asset_ref<T_other_type>& as()
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only cast if both derive from same parent/child hierarchy");

			return *reinterpret_cast<Asset_ref<T_other_type>*>(this);
		}

		template <typename T_other_type>
		const Asset_ref<T_other_type>& as() const
		{
			static_assert(std::is_base_of<T_other_type, T_type>::value || std::is_base_of<T_type, T_other_type>::value, "Can only cast if both derive from same parent/child hierarchy");

			return *reinterpret_cast<const Asset_ref<T_other_type>*>(this);
		}

		~Asset_ref()
		{
			if (m_header)
			{
				assert(m_was_incremented && "should not be possible to be here without incrementing!!");
				m_header->decrement_reference_count();
				m_header = nullptr;
			}
		}

		bool operator==(const Asset_ref<T_type>& in_other) const
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
				[&dependencies_loading = inout_asset.m_dependencies_loading, &this_header = m_header](Asset_loaded_processor in_processor, Asset*)
				{
					--dependencies_loading;
					if (dependencies_loading == 0)
					{
						SIC_LOG(g_log_asset_verbose, "Loaded asset: \"{0}\"", this_header.m_name.c_str());
						this_header.m_load_state = Asset_load_state::loaded;
						this_header.m_on_loaded_delegate.invoke({ in_processor, this_header.m_loaded_asset.get() });
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
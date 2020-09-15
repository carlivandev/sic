#pragma once
#include "object_base.h"
#include "engine.h"
#include "component.h"
#include "object.h"
#include "event.h"
#include "level_context.h"

namespace sic
{
	/*
		T_subtype = CRTP
	*/
	template <typename T_subtype, typename ...T_component>
	struct Object : public Object_base
	{
		friend struct Object_storage;
		friend struct Engine_context;
		friend struct Scene_context;

		template <typename T_to_check_with>
		bool set_if_matching(i32 in_type_idx, byte*& out_result)
		{
			if (Type_index<Component_base>::get<T_to_check_with>() == in_type_idx)
			{
				auto it = std::get<T_to_check_with*>(m_components);
				out_result = reinterpret_cast<byte*>(it);
				return true;
			}

			return false;
		}

		template <typename T_to_check_with>
		bool set_if_matching(i32 in_type_idx, const byte*& out_result) const
		{
			if (Type_index<Component_base>::get<T_to_check_with>() == in_type_idx)
			{
				auto it = std::get<T_to_check_with*>(m_components);
				out_result = reinterpret_cast<const byte*>(&(*it));
				return true;
			}

			return false;
		}

		byte* find_internal(i32 in_type_idx) override final
		{
			byte* result = nullptr;

			(set_if_matching<T_component>(in_type_idx, result) || ...);

			return result;
		}

		const byte* find_internal(i32 in_type_idx) const override final
		{
			const byte* result = nullptr;

			(set_if_matching<T_component>(in_type_idx, result) || ...);

			return result;
		}

		template<typename T_to_create>
		constexpr void create_component(Scene_context& inout_level)
		{
			std::get<T_to_create*>(m_components) = &inout_level.m_level.create_component<T_to_create>(*this);
		}

		template<typename T_to_invoke_on>
		constexpr void invoke_post_creation_event(Scene_context& inout_level)
		{
			inout_level.m_engine.invoke<event_post_created<T_to_invoke_on>>(*std::get<T_to_invoke_on*>(m_components));
		}

		template<typename T_to_destroy>
		constexpr void destroy_component(Scene_context& inout_level)
		{
			auto& destroy_it = std::get<T_to_destroy*>(m_components);
			inout_level.m_level.destroy_component<T_to_destroy>(*destroy_it);
			destroy_it = nullptr;
		}

	private:
		template<typename T_to_get>
		__forceinline constexpr T_to_get& get()
		{
			auto it = std::get<T_to_get*>(m_components);
			return *it;
		}

		template<typename T_to_get>
		__forceinline constexpr const T_to_get& get() const
		{
			const auto it = std::get<T_to_get*>(m_components);
			return *it;
		}

		constexpr void make_instance(Scene_context& inout_level)
		{
			m_level_id = inout_level.get_level_id();
			m_outermost_level_id = inout_level.get_outermost_level_id();

			(create_component<T_component>(inout_level), ...);
			(invoke_post_creation_event<T_component>(inout_level), ...);
		}

		void destroy_instance(Scene_context& inout_level) override
		{
			static_assert(std::is_base_of_v<Object_base, T_subtype>, "did you forget T_subtype?");
			inout_level.m_engine.invoke<event_destroyed<T_subtype>>(*reinterpret_cast<T_subtype*>(this));

			(destroy_component<T_component>(inout_level), ...);
		}

		std::tuple<T_component*...> m_components;
	};

	struct Object_storage : public Object_storage_base
	{
		void initialize_with_typesize(ui32 in_initial_capacity, ui32 in_bucket_capacity, ui32 in_typesize)
		{
			m_instances.allocate_with_typesize(in_initial_capacity, in_bucket_capacity, in_typesize);
		}

		template <typename T_object>
		constexpr T_object& make_instance(Scene_context& inout_level)
		{
			if (m_free_object_locations.empty())
			{
				T_object& new_instance = m_instances.emplace_back<T_object>();
				new_instance.m_type_index = Type_index<Object_base>::get<T_object>();
				new_instance.m_pending_destroy = false;
				new (&new_instance.m_children) std::vector<Object_base*>();

				new_instance.make_instance(inout_level);

				return new_instance;
			}

			byte* new_pos = m_free_object_locations.back();
			m_free_object_locations.pop_back();

			T_object& new_instance = *reinterpret_cast<T_object*>(new_pos);
			new_instance.m_type_index = Type_index<Object_base>::get<T_object>();
			new (&new_instance.m_children) std::vector<Object_base*>();

			new_instance.make_instance(inout_level);

			return new_instance;
		}

		void destroy_instance(Scene_context& inout_level, Object_base& in_object_to_destroy)
		{
			in_object_to_destroy.destroy_instance(inout_level);

			in_object_to_destroy.m_parent = nullptr;
			in_object_to_destroy.m_children.~vector();

			m_free_object_locations.push_back(reinterpret_cast<byte*>(&in_object_to_destroy));

			in_object_to_destroy.m_type_index = -1;
		}

		std::vector<byte*> m_free_object_locations;
	};
}
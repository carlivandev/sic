#pragma once
#include "sic/core/object_base.h"
#include "sic/core/engine.h"
#include "sic/core/component.h"
#include "sic/core/object.h"
#include "sic/core/event.h"
#include "sic/core/scene_context.h"

namespace sic
{
	template<typename ...T, size_t... I>
	auto tuple_of_pointers_to_references_helper(std::tuple<T...>& t, std::index_sequence<I...>)
	{
		return std::tie(*std::get<I>(t)...);
	}

	template<typename ...T>
	auto tuple_of_pointers_to_references(std::tuple<T...>& t) {
		return tuple_of_pointers_to_references_helper<T...>(t, std::make_index_sequence<sizeof...(T)>{});
	}

	template<typename ...T, size_t... I>
	auto tuple_of_pointers_to_const_references_helper(const std::tuple<T...>& t, std::index_sequence<I...>)
	{
		return std::tie(*std::get<I>(t)...);
	}

	template<typename ...T>
	auto tuple_of_pointers_to_const_references(const std::tuple<T...>& t) {
		return tuple_of_pointers_to_const_references_helper<T...>(t, std::make_index_sequence<sizeof...(T)>{});
	}

	/*
		T_subtype = CRTP
	*/
	template <typename T_subtype, typename ...T_component>
	struct Object : public Object_base
	{
		friend struct Object_storage;
		friend struct Engine_context;
		friend struct Scene_context;
		friend struct Scene;

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
		constexpr void create_component(Scene_context& inout_scene)
		{
			std::get<T_to_create*>(m_components) = &inout_scene.m_scene.create_component<T_to_create>(*this);
		}

		template<typename T_to_invoke_on>
		constexpr void invoke_post_creation_event(Scene_context& inout_scene)
		{
			inout_scene.m_engine.invoke<Event_post_created<T_to_invoke_on>>(std::reference_wrapper<T_to_invoke_on>(*std::get<T_to_invoke_on*>(m_components)));
		}

		template<typename T_to_destroy>
		constexpr void destroy_component(Scene_context& inout_scene)
		{
			auto& destroy_it = std::get<T_to_destroy*>(m_components);
			inout_scene.m_scene.destroy_component<T_to_destroy>(*destroy_it);
			destroy_it = nullptr;
		}

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

		std::tuple<T_component&...> components()
		{
			return tuple_of_pointers_to_references(m_components);
		}

		std::tuple<const T_component&...> components() const
		{
			return tuple_of_pointers_to_const_references(m_components);
		}

	private:
		constexpr void make_instance(Scene_context& inout_scene)
		{
			m_scene_id = inout_scene.get_scene_id();
			m_outermost_scene_id = inout_scene.get_outermost_scene_id();

			(create_component<T_component>(inout_scene), ...);
		}

		constexpr void invoke_post_creation_events(Scene_context& inout_scene)
		{
			(invoke_post_creation_event<T_component>(inout_scene), ...);
		}

		void destroy_instance(Scene_context& inout_scene) override
		{
			static_assert(std::is_base_of_v<Object_base, T_subtype>, "did you forget T_subtype?");
			inout_scene.m_engine.invoke<Event_destroyed<T_subtype>>(reinterpret_cast<T_subtype*>(this));

			(destroy_component<T_component>(inout_scene), ...);
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
		constexpr T_object& make_instance(Scene_context& inout_scene)
		{
			if (m_free_object_locations.empty())
			{
				T_object& new_instance = m_instances.emplace_back<T_object>();
				new_instance.m_type_index = Type_index<Object_base>::get<T_object>();
				new_instance.m_pending_destroy = false;
				new (&new_instance.m_children) std::vector<Object_base*>();

				new_instance.make_instance(inout_scene);

				return new_instance;
			}

			byte* new_pos = m_free_object_locations.back();
			m_free_object_locations.pop_back();

			T_object& new_instance = *reinterpret_cast<T_object*>(new_pos);
			new_instance.m_type_index = Type_index<Object_base>::get<T_object>();
			new (&new_instance.m_children) std::vector<Object_base*>();

			new_instance.make_instance(inout_scene);

			return new_instance;
		}

		void destroy_instance(Scene_context& inout_scene, Object_base& in_object_to_destroy)
		{
			in_object_to_destroy.destroy_instance(inout_scene);

			in_object_to_destroy.m_parent = nullptr;
			in_object_to_destroy.m_children.~vector();

			m_free_object_locations.push_back(reinterpret_cast<byte*>(&in_object_to_destroy));

			in_object_to_destroy.m_type_index = -1;
		}

		std::vector<byte*> m_free_object_locations;
	};
}
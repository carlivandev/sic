#pragma once
#include "object_base.h"
#include "world.h"
#include "component.h"
#include "object.h"
#include "event.h"

namespace impuls
{
	template <typename ...t_component>
	struct i_object : public i_object_base
	{
		friend struct object_storage;
		friend struct world_context;

		template <typename t_to_check_with>
		bool set_if_matching(i32 in_type_idx, byte*& out_result)
		{
			if (type_index<i_component_base>::get<t_to_check_with>() == in_type_idx)
			{
				out_result = reinterpret_cast<byte*>(std::get<t_to_check_with*>(m_components));
				return true;
			}

			return false;
		}

		template <typename t_to_check_with>
		bool set_if_matching(i32 in_type_idx, const byte*& out_result) const
		{
			if (type_index<i_component_base>::get<t_to_check_with>() == in_type_idx)
			{
				out_result = reinterpret_cast<const byte*>(std::get<t_to_check_with*>(m_components));
				return true;
			}

			return false;
		}

		byte* find_internal(i32 in_type_idx) override final
		{
			byte* result = nullptr;

			(set_if_matching<t_component>(in_type_idx, result) || ...);

			return result;
		}

		const byte* find_internal(i32 in_type_idx) const override final
		{
			const byte* result = nullptr;

			(set_if_matching<t_component>(in_type_idx, result) || ...);

			return result;
		}

		template<typename t_to_get>
		__forceinline constexpr t_to_get& get()
		{
			t_to_get* to_return = std::get<t_to_get*>(m_components);
			assert(to_return && "should always be valid");
			return *to_return;
		}

		template<typename t_to_get>
		__forceinline constexpr const t_to_get& get() const
		{
			const t_to_get* to_return = std::get<t_to_get*>(m_components);
			assert(to_return && "should always be valid");
			return *to_return;
		}

		template<typename t_to_create>
		constexpr void create_component(world& in_world)
		{
			std::get<t_to_create*>(m_components) = &in_world.create_component<t_to_create>(*this);
		}

		template<typename t_to_invoke_on>
		constexpr void invoke_post_creation_event(world& in_world)
		{
			in_world.invoke<event_post_created<t_to_invoke_on>>(*std::get<t_to_invoke_on*>(m_components));
		}

		template<typename t_to_destroy>
		constexpr void destroy_component(world& in_world)
		{
			in_world.destroy_component(*std::get<t_to_destroy*>(m_components));
			std::get<t_to_destroy*>(m_components) = nullptr;
		}

		private:
			constexpr void make_instance(world& in_world)
			{
				(create_component<t_component>(in_world), ...);
				(invoke_post_creation_event<t_component>(in_world), ...);
			}

			void destroy_instance(world& in_world) override
			{
				(destroy_component<t_component>(in_world), ...);
			}

			std::tuple<t_component*...> m_components;
	};

	struct object_storage : public i_object_storage_base
	{
		void initialize_with_typesize(ui32 in_initial_capacity, ui32 in_bucket_capacity, ui32 in_typesize)
		{
			m_instances.allocate_with_typesize(in_initial_capacity, in_bucket_capacity, in_typesize);
		}

		template <typename t_object>
		constexpr t_object& make_instance(world& in_world)
		{
			if (m_free_object_locations.empty())
			{
				t_object& new_instance = m_instances.emplace_back<t_object>();
				new_instance.m_type_index = type_index<i_object_base>::get<t_object>();
				new (&new_instance.m_children) std::vector<i_object_base*>();

				new_instance.make_instance(in_world);

				return new_instance;
			}

			byte* new_pos = m_free_object_locations.back();
			m_free_object_locations.pop_back();

			t_object& new_instance = *reinterpret_cast<t_object*>(new_pos);
			new_instance.m_type_index = type_index<i_object_base>::get<t_object>();
			new (&new_instance.m_children) std::vector<i_object_base*>();

			new_instance.make_instance(in_world);

			return new_instance;
		}

		void destroy_instance(world& in_world, i_object_base& in_object_to_destroy)
		{
			in_object_to_destroy.destroy_instance(in_world);

			in_object_to_destroy.m_parent = nullptr;
			in_object_to_destroy.m_children.~vector();

			m_free_object_locations.push_back(reinterpret_cast<byte*>(&in_object_to_destroy));

			in_object_to_destroy.m_type_index = -1;
		}

		bucket_allocator m_instances;
		std::vector<byte*> m_free_object_locations;
	};
}
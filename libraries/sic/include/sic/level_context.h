#pragma once
#include "sic/level.h"
#include "sic/bucket_allocator_view.h"
#include "sic/object.h"
#include "sic/engine_context.h"

namespace sic
{
	template <typename T>
	struct Processor_flag_read {};

	template <typename T>
	struct Processor_flag_write {};

	template <typename ...T_processor_flags>
	struct Processor
	{
		Processor(Scene_context& in_context) : m_context(in_context) {}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			static_assert((std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value, ...), "Processor does not have write flag for T_type");
			m_context.for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, Processor_flag_read<T_type>>::value, ...) ||
				(std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value, ...)
				, "Processor does not have read/write flag for T_type"
			);
			m_context.for_each_r<T_type>(in_func);
		}

	private:
		Scene_context& m_context;
	};

	struct Object_base;
	
	struct Scene_context
	{
		template <typename T_subtype, typename ...T_component>
		friend struct Object;

		template <typename ...T_processor_flags>
		friend struct Processor;

		Scene_context(Engine& inout_engine, Scene& inout_level) : m_engine(inout_engine), m_level(inout_level){}

		template <typename T_object>
		__forceinline constexpr T_object& create_object()
		{
			return m_level.create_object<T_object>();
		}

		void destroy_object(Object_base& in_object_to_destroy)
		{
			m_level.destroy_object(in_object_to_destroy);
		}

		template <typename ...T_processor_flags>
		Processor<T_processor_flags...> process()
		{
			return Processor<T_processor_flags...>(*this);
		}

		Engine_context get_engine_context() { return m_engine; }
		const Engine_context get_engine_context() const { return m_engine; }
		
		i32 get_outermost_level_id() const { return m_level.m_outermost_level != nullptr ? m_level.m_outermost_level->m_level_id : get_level_id(); }
		i32 get_level_id() const { return m_level.m_level_id; }
		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_level) const { return m_level.get_does_object_exist(in_id, in_only_in_this_level); }

	private:
		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			m_level.for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			m_level.for_each_w<T_type>(in_func);
		}

		Engine& m_engine;
		Scene& m_level;
	};
}


#pragma once
#include "sic/state_render_scene.h"
#include "sic/system_window.h"
#include "sic/input.h"
#include "sic/easing.h"

#include "sic/core/system.h"
#include "sic/core/delegate.h"

#include "glm/vec2.hpp"

#include <optional>

namespace sic
{
	extern Log g_log_ui;
	extern Log g_log_ui_verbose;

	struct State_ui;
	struct Ui_slot_tooltip;

	using Processor_ui = Engine_processor<Processor_flag_write<State_ui>, Processor_flag_read<State_input>, Processor_flag_read<State_window>>;

	struct Ui_anchors
	{
		Ui_anchors() = default;

		Ui_anchors(float in_uniform_anchors)
		{
			set(in_uniform_anchors);
		}

		Ui_anchors(float in_horizontal_anchors, float in_vertical_anchors)
		{
			set(in_horizontal_anchors, in_vertical_anchors);
		}

		Ui_anchors(float in_min_x, float in_min_y, float in_max_x, float in_max_y)
		{
			set(in_min_x, in_min_y, in_max_x, in_max_y);
		}

		void set(float in_uniform_anchors)
		{
			m_min = { in_uniform_anchors, in_uniform_anchors };
			m_max = { in_uniform_anchors, in_uniform_anchors };
		}

		void set(float in_horizontal_anchors, float in_vertical_anchors)
		{
			m_min = { in_horizontal_anchors, in_vertical_anchors };
			m_max = { in_horizontal_anchors, in_vertical_anchors };
		}

		void set(float in_min_x, float in_min_y, float in_max_x, float in_max_y)
		{
			m_min = { in_min_x, in_min_y };
			m_max = { in_max_x, in_max_y };
		}

		const glm::vec2& get_min() const { return m_min; }
		const glm::vec2& get_max() const { return m_max; }

	private:
		glm::vec2 m_min = { 0.5f, 0.5f };
		glm::vec2 m_max = { 0.5f, 0.5f };
	};

	struct Ui_padding
	{
		Ui_padding() = default;

		Ui_padding(float in_uniform_padding)
		{
			set(in_uniform_padding);
		}

		Ui_padding(float in_horizontal_padding, float in_vertical_padding)
		{
			set(in_horizontal_padding, in_vertical_padding);
		}

		Ui_padding(float in_left, float in_top, float in_right, float in_bottom)
		{
			set(in_left, in_top, in_right, in_bottom);
		}

		void set(float in_uniform_padding)
		{
			m_left = m_top = m_right = m_bottom = in_uniform_padding;
		}

		void set(float in_horizontal_padding, float in_vertical_padding)
		{
			m_left = m_right = in_horizontal_padding;
			m_top = m_bottom = in_vertical_padding;
		}

		void set(float in_left, float in_top, float in_right, float in_bottom)
		{
			m_left = in_left;
			m_top = in_top;
			m_right = in_right;
			m_bottom = in_bottom;
		}

		float get_left() const { return m_left; }
		float get_top() const { return m_top; }
		float get_right() const { return m_right; }
		float get_bottom() const { return m_bottom; }

	private:
		float m_left = 0.0f;
		float m_top = 0.0f;
		float m_right = 0.0f;
		float m_bottom = 0.0f;
	};

	enum struct Ui_h_alignment
	{
		left,
		center,
		right,
		fill
	};

	enum struct Ui_v_alignment
	{
		top,
		center,
		bottom,
		fill
	};

	void align_horizontal(Ui_h_alignment in_alignment, float in_max_size, float& inout_current_x, float& inout_current_size);
	void align_vertical(Ui_v_alignment in_alignment, float in_max_size, float& inout_current_y, float& inout_current_size);

	struct Ui_widget_base;
	struct State_ui;

	enum struct Interaction_consume
	{
		consume,
		fall_through,
		disabled
	};

	enum struct Ui_interaction_state
	{
		idle,
		hovered,
		pressed,
		pressed_not_hovered,
	};

	enum struct Ui_visibility
	{
		visible,
		invisible,
		collapsed
	};

	enum struct Ui_widget_update : ui8
	{
		none = 0,
		layout = 1,
		appearance = 2,
		layout_and_appearance = 3,
		all = 3
	};

	template <typename T_widget_type>
	struct Ui_ease_schedule_data
	{
		Ui_ease_schedule_data(const std::string& in_key, sic::Ease<float> in_ease, std::function<void(T_widget_type&, float)> in_callback, std::function<bool(Engine_context)> in_unschedule_callback, float in_delay_until_start = 0.0f, bool in_run_on_main_thread = false) :
			m_key(in_key), m_ease(in_ease), m_callback(in_callback), m_unschedule_callback(in_unschedule_callback), m_delay_until_start(in_delay_until_start), m_run_on_main_thread(in_run_on_main_thread) {}

		std::function<void(sic::Engine_processor<sic::Processor_flag_write<sic::State_ui>>)> get_callback()
		{
			return[ease = m_ease, callback = m_callback, key = m_key](sic::Engine_processor<sic::Processor_flag_write<sic::State_ui>> in_processor) mutable
			{
				if (auto* widget = in_processor.get_state_checked_w<sic::State_ui>().find<T_widget_type>(key))
					callback(*widget, ease.tick(in_processor.get_time_delta()));
			};
		}

		float get_duration() const { return m_ease.m_duration; }
		float get_delay_until_start() const { return m_delay_until_start; }
		bool get_run_on_main_thread() const { return m_run_on_main_thread; }
		std::function<bool(Engine_context)> get_unschedule_callback() const { return m_unschedule_callback; }

	private:
		std::string m_key;
		sic::Ease<float> m_ease;
		std::function<void(T_widget_type&, float)> m_callback;
		std::function<bool(Engine_context)> m_unschedule_callback;
		float m_delay_until_start = 0.0f;
		bool m_run_on_main_thread = false;
	};

	template <typename ...T_args>
	struct Ui_delegate : Delegate<Engine_processor<>, T_args...> {};

	struct Ui_widget_base
	{
		struct On_pressed : Ui_delegate<> {};
		struct On_released : Ui_delegate<> {};
		struct On_clicked : Ui_delegate<> {};
		struct On_hover_begin : Ui_delegate<> {};
		struct On_hover_end : Ui_delegate<> {};
		struct On_cursor_move_over : Ui_delegate<> {};
		struct On_drag : Ui_delegate<const glm::vec2&, const glm::vec2&> {};/*in_cursor_pos, in_cursor_movement*/
		struct On_character_input : Ui_delegate<unsigned int> {};
		struct On_focus_begin : Ui_delegate<> {};
		struct On_focus_end : Ui_delegate<> {};

		friend struct State_ui;
		friend struct System_ui;

		template <typename T_subtype, typename T_slot_type>
		friend struct Ui_parent_widget;

		template <typename T_subtype>
		friend struct Ui_widget;

		virtual void calculate_render_transform(const glm::vec2& in_window_size);
		virtual void calculate_content_size(const glm::vec2& in_window_size, bool in_slot_determines_size_x, bool in_slot_determines_size_y);

		virtual glm::vec2 get_content_size(const glm::vec2& in_window_size) const { in_window_size;  return glm::vec2(0.0f, 0.0f); }

		Ui_widget_base* get_outermost_parent();
		const Ui_widget_base* get_outermost_parent() const;

		Ui_widget_base* get_parent() { return m_parent; }
		const Ui_widget_base* get_parent() const { return m_parent; }

		const std::string& get_key() const { return m_key; }

		i32 get_slot_index() const { return m_slot_index; }

		virtual void gather_dependencies(std::vector<Asset_header*>& out_assets) const { out_assets; }

		void get_dependencies_not_loaded(std::vector<Asset_header*>& out_assets) const;

		virtual bool get_ready_to_be_shown() const;

		//interaction events begin, return bool 

		virtual void on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement);
		virtual void on_hover_begin(const glm::vec2& in_cursor_pos);
		virtual void on_hover_end(const glm::vec2& in_cursor_pos);
		virtual void on_pressed(Mousebutton in_button, const glm::vec2& in_cursor_pos);
		virtual void on_released(Mousebutton in_button, const glm::vec2& in_cursor_pos);
		virtual void on_clicked(Mousebutton in_button, const glm::vec2& in_cursor_pos);
		virtual void on_character_input(unsigned int in_character);
		virtual void on_focus_begin();
		virtual void on_focus_end();

		virtual std::optional<std::string> get_clipboard_string() { return {}; }
		virtual void on_interaction_state_changed() {}
		//interaction events end
		virtual void gather_widgets_over_point(const glm::vec2& in_point, std::vector<Ui_widget_base*>& out_widgets) { if (is_point_inside(in_point)) out_widgets.push_back(this); }

		Ui_interaction_state get_interaction_state() const { return m_interaction_state; }

		bool is_point_inside(const glm::vec2& in_point) const
		{
			return in_point.x >= m_render_translation.x &&
				in_point.x <= m_render_translation.x + m_global_render_size.x &&
				in_point.y >= m_render_translation.y &&
				in_point.y <= m_render_translation.y + m_global_render_size.y;
		}

		template <typename T_delegate_type, typename ...T_args>
		__forceinline void invoke(T_args... in_args)
		{
			m_ui_state->invoke<T_delegate_type>(m_key, in_args...);
		}

		i32 m_widget_index = -1;
		bool m_allow_dependency_streaming = false;

		glm::vec2 m_render_translation;
		glm::vec2 m_global_render_size;
		float m_render_rotation;

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor)
		{ in_final_translation; in_final_size; in_final_rotation; in_window_size; in_window_id; inout_processor; ((ui8&)m_update_requested) &= ~(ui8)Ui_widget_update::appearance; }

		void request_update(Ui_widget_update in_update) { ((ui8&)m_update_requested) |= (ui8)in_update; }
		
		virtual void destroy(Processor_ui& inout_processor);

		void destroy_render_object(Processor_ui& inout_processor, std::optional<size_t>& inout_index);
		void update_render_object(Processor_ui& inout_processor, std::optional<size_t>& inout_index, std::function<void(Render_object_ui&)> in_callback);

		Ui_widget_base() = default;
		virtual ~Ui_widget_base() = default;

		virtual void destroy_child(size_t in_slot_index, Processor_ui& inout_processor) { in_slot_index; inout_processor; }
		virtual void unchild(size_t in_slot_index) { in_slot_index; }
		virtual Ui_widget_update* find_slot_update_requested(size_t in_slot_index) { in_slot_index; return nullptr; }

		virtual bool should_propagate_dirtyness_to_parent() const { return true; }
		virtual void propagate_request_to_children(Ui_widget_update in_request) { in_request; }

		virtual std::pair<bool, bool> get_slot_overrides_content_size(size_t in_slot_index) const { in_slot_index; return { false, false }; }

		Ui_widget_base* get_previous_widget_render_order() const { if (!m_parent) return nullptr; return m_parent->get_child_previous_widget_render_order(m_slot_index); }
		virtual Ui_widget_base* get_child_previous_widget_render_order(i32 in_slot_index) { return nullptr; }
		virtual Ui_widget_base* get_leaf_widget_render_order() { return this; }

		virtual Ui_widget_base* get_next_widget_render_order() { if (!m_parent) return nullptr; return m_parent->get_child_next_widget_render_order(m_slot_index); }
		virtual Ui_widget_base* get_child_next_widget_render_order(i32 in_slot_index) { return nullptr; }

		void set_visibility(Ui_visibility in_new_state)
		{
			if (m_visibility_state == in_new_state)
				return;

			if (in_new_state == Ui_visibility::collapsed)
				request_update(Ui_widget_update::layout_and_appearance);
			else if (m_visibility_state == Ui_visibility::collapsed)
				request_update(Ui_widget_update::layout_and_appearance);
			else
				request_update(Ui_widget_update::appearance);

			m_visibility_state = in_new_state;
		}

		virtual void set_window_id(const Update_list_id<Render_object_window>& in_window_id);

		Update_list_id<Render_object_window> m_window_id;

		void Update_image_common(Processor_ui& inout_processor, Ui_widget_base& inout_owner, std::optional<size_t>& inout_ro_id, const Asset_ref<Asset_material>& in_material, const glm::vec4& in_tint, const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id);

	private:
		std::string m_key;
		Ui_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		On_asset_loaded::Handle m_dependencies_loaded_handle;
		State_ui* m_ui_state = nullptr;

		glm::vec2 m_local_scale = { 1.0f, 1.0f };
		glm::vec2 m_local_translation = { 0.0f, 0.0f };
		float m_local_rotation = 0.0f;

		bool m_correctly_added = false;
		Ui_interaction_state m_interaction_state = Ui_interaction_state::idle;
		Interaction_consume m_interaction_consume_type = Interaction_consume::consume;
		Ui_widget_update m_update_requested = Ui_widget_update::all;

		bool m_render_transform_was_updated_this_frame = false;
		bool m_render_state_was_updated_this_frame = false;
		Ui_visibility m_visibility_state = Ui_visibility::visible;

		std::function<std::pair<Ui_widget_base&, Ui_slot_tooltip>(State_ui&, const glm::vec2&)> m_on_create_tooltip_callback;

		struct Ro_id_item
		{
			Update_list_id<Render_object_ui> m_ro_id;

			Ui_widget_base* m_next_widget = nullptr;
			std::optional<size_t> m_next_ro_index;

			Ui_widget_base* m_previous_widget = nullptr;
			std::optional<size_t> m_previous_ro_index;
		};

		std::vector<Ro_id_item> m_ro_ids;
		std::vector<size_t> m_free_ro_id_indices;
	};

	template <typename T_subtype>
	struct Ui_widget : Ui_widget_base
	{
		virtual ~Ui_widget() = default;

		T_subtype& interaction_consume(Interaction_consume in_interaction_consume_type) { m_interaction_consume_type = in_interaction_consume_type; request_update(Ui_widget_update::layout_and_appearance); return *reinterpret_cast<T_subtype*>(this); }
		T_subtype& local_scale(const glm::vec2& in_scale) { m_local_scale = in_scale; request_update(Ui_widget_update::appearance); return *reinterpret_cast<T_subtype*>(this); }
		T_subtype& local_translation(const glm::vec2& in_translation) { m_local_translation = in_translation; request_update(Ui_widget_update::appearance); return *reinterpret_cast<T_subtype*>(this); }
		T_subtype& local_rotation(float in_rotation) { m_local_rotation = in_rotation; request_update(Ui_widget_update::appearance); return *reinterpret_cast<T_subtype*>(this); }

		template <typename T_widget_type>
		__forceinline T_subtype& tooltip(std::function<void(State_ui& inout_ui_state, T_widget_type& inout_widget, Ui_slot_tooltip& inout_slot, const glm::vec2& in_window_size)> in_create_callback)
		{
			m_on_create_tooltip_callback = [in_create_callback](State_ui& inout_ui_state, const glm::vec2& in_window_size)
			{
				auto& new_widget = inout_ui_state.create<T_widget_type>({});
				Ui_slot_tooltip slot;
				in_create_callback(inout_ui_state, new_widget, slot, in_window_size);
				return std::pair<Ui_widget_base&, Ui_slot_tooltip>{ new_widget, slot };
			};

			return *reinterpret_cast<T_subtype*>(this);
		}
	};

	struct State_ui : State
	{
		struct Window_info
		{
			glm::vec2 m_size;
			Update_list_id<Render_object_window> m_id;
			std::string m_tooltip_canvas_key;
		};

		friend Ui_widget_base;
		friend struct System_ui;

		template <typename T_subtype, typename T_slot_type>
		friend struct Ui_parent_widget;

		template<typename T_widget_type>
		T_widget_type& create(std::optional<std::string> in_key)
		{
			const std::string key = in_key.has_value() ? in_key.value() : xg::newGuid().str();

			assert(m_widget_lut.find(key) == m_widget_lut.end() && "Can not have two widgets with same key!");

			size_t new_idx;

			if (m_free_widget_indices.empty())
			{
				new_idx = m_widgets.size();
				m_widgets.push_back(std::make_unique<T_widget_type>());
			}
			else
			{
				new_idx = m_free_widget_indices.back();
				m_free_widget_indices.pop_back();
				m_widgets[new_idx] = std::make_unique<T_widget_type>();
			}

			auto& widget = m_widget_lut[key] = m_widgets[new_idx].get();
			widget->m_key = key;
			widget->m_widget_index = static_cast<i32>(new_idx);
			widget->m_correctly_added = true;

			T_widget_type& ret_val = *reinterpret_cast<T_widget_type*>(widget);

			ret_val.m_ui_state = this;

			return ret_val;
		}

		template<typename T_widget_type>
		T_widget_type& create_get_key(std::string& out_key)
		{
			auto& ret_val = create<T_widget_type>({});
			out_key = ret_val.m_key;
			return ret_val;
		}

		void destroy(Processor_ui& inout_processor, const std::string& in_key)
		{
			auto it = m_widget_lut.find(in_key);

			if (it == m_widget_lut.end())
				return;

			if (it->second)
				destroy(inout_processor , *it->second);
		}

		void destroy(Processor_ui& inout_processor, Ui_widget_base& inout_widget)
		{
			auto it = m_widget_lut.find(inout_widget.m_key);

			if (it == m_widget_lut.end())
				return;

			inout_widget.destroy(inout_processor);

			if (auto* parent = it->second->get_parent())
			{
				parent->request_update(Ui_widget_update::layout_and_appearance);
				parent->unchild(it->second->m_slot_index);
			}

			if (it->second == m_focused_widget)
			{
				m_focused_widget->on_focus_end();
				m_focused_widget = nullptr;
			}

			m_window_key_to_root_key_lut.erase(inout_widget.m_key);
			m_root_to_window_info_lut.erase(inout_widget.m_key);
			m_event_delegate_lut.erase(inout_widget.m_key);

			m_free_widget_indices.push_back(it->second->m_widget_index);
			m_widgets[it->second->m_widget_index].reset();

			m_widget_lut.erase(it);
		}

		template<typename T_widget_type>
		T_widget_type* find(const std::string& in_key)
		{
			auto it = m_widget_lut.find(in_key);
			if (it == m_widget_lut.end())
				return nullptr;

			return reinterpret_cast<T_widget_type*>(it->second);
		}

		template<typename T_widget_type>
		const T_widget_type* find(const std::string& in_key) const
		{
			auto it = m_widget_lut.find(in_key);
			if (it == m_widget_lut.end())
				return nullptr;

			return reinterpret_cast<T_widget_type*>(it->second);
		}

		template <typename T_delegate_type>
		void bind(const std::string& in_widget_key, T_delegate_type::template Signature in_callback)
		{
			auto& events_for_key = m_event_delegate_lut[in_widget_key][Type_index<Ui_delegate<>>::get<T_delegate_type>()];

			events_for_key.push_back(*reinterpret_cast<std::function<void()>*>(&in_callback));
		}
		
		template <typename T_delegate_type, typename ...T_args>
		__forceinline void invoke(const std::string& in_key, T_args... in_args)
		{
			auto event_lists_it = m_event_delegate_lut.find(in_key);

			if (event_lists_it == m_event_delegate_lut.end())
				return;

			auto event_list_it = event_lists_it->second.find(Type_index<Ui_delegate<>>::get<T_delegate_type>());

			if (event_list_it == event_lists_it->second.end())
				return;

			if (event_list_it->second.empty())
				return;

			m_events_to_invoke.push_back
			(
				[listeners = event_list_it->second, in_args...](Engine_processor<> in_processor) mutable
				{
					for (auto&& listener : listeners)
						(*reinterpret_cast<T_delegate_type::template Signature*>(&listener))(in_processor, in_args...);
				}
			);
		}

		void set_window_info(const std::string& in_window_widget_key, const glm::vec2& in_size);
		void create_window_info(const std::string& in_window_widget_key, const glm::vec2& in_size, Update_list_id<Render_object_window> in_id);

	private:
		std::vector<std::unique_ptr<Ui_widget_base>> m_widgets;
		std::vector<size_t> m_free_widget_indices;
		std::unordered_map<std::string, Ui_widget_base*> m_widget_lut;
		std::unordered_set<Ui_widget_base*> m_dirty_root_widgets;

		std::unordered_map<std::string, std::string> m_window_key_to_root_key_lut;
		std::unordered_map<std::string, Window_info> m_root_to_window_info_lut;

		std::vector<Ui_widget_base*> m_widgets_to_redraw;
		Ui_widget_base* m_focused_widget = nullptr;

		std::unordered_map<std::string, std::unordered_map<i32, std::vector<std::function<void()>>>> m_event_delegate_lut;

		i32 m_ro_id_ticker = 0;
		std::vector<i32> m_free_ro_ids;
		std::vector<i32> m_free_ro_ids_this_frame;

		Double_buffer<std::vector<std::function<void(Engine_processor<Processor_flag_write<State_render_scene>>)>>> m_updates;
		std::vector<std::function<void(Engine_processor<>)>> m_events_to_invoke;
	};

	struct System_ui : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

		static void update_ui(Processor_ui in_processor);
		static void update_ui_interactions(Processor_ui in_processor);
	};

	struct Ui_slot
	{
		friend System_ui;

		template <typename T_subtype, typename T_slot_type>
		friend struct Ui_parent_widget;


		virtual std::pair<bool, bool> get_overrides_content_size() const { return { false, false }; }


		void request_update(Ui_widget_update in_update) { ((ui8&)m_update_requested) |= (ui8)in_update; }

	private:
		Ui_widget_update m_update_requested = Ui_widget_update::layout_and_appearance;
	};

	template <typename T_subtype, typename T_slot_type>
	struct Ui_parent_widget : Ui_widget<T_subtype>
	{
		friend struct State_ui;

		using Slot = T_slot_type;

		virtual ~Ui_parent_widget() = default;

		T_subtype& add_child(const Slot& in_slot_data, Ui_widget_base& inout_widget)
		{
			assert(inout_widget.m_correctly_added && "Widget was not created properly, please use Ui_context::create_widget");
			assert(!inout_widget.m_parent && "Widget has already been added to some parent.");
			m_children.emplace_back(in_slot_data, inout_widget);
			inout_widget.m_parent = this;
			inout_widget.m_slot_index = static_cast<i32>(m_children.size() - 1);

			inout_widget.set_window_id(m_window_id);

			if (m_visibility_state != Ui_visibility::visible)
				inout_widget.m_visibility_state = m_visibility_state;

			request_update(Ui_widget_update::layout_and_appearance);

			return *reinterpret_cast<T_subtype*>(this);
		}

		virtual void destroy_child(size_t in_slot_index, Processor_ui& inout_processor) override
		{
			if (in_slot_index >= m_children.size())
				return;

			inout_processor.get_state_checked_w<State_ui>().destroy(inout_processor, m_children[in_slot_index].second.get());

			request_update(Ui_widget_update::layout_and_appearance);
		}

		virtual void unchild(size_t in_slot_index) override
		{
			if (in_slot_index >= m_children.size())
				return;

			for (size_t i = in_slot_index + 1; i < m_children.size(); i++)
				--m_children[i].second.get().m_slot_index;

			m_children[in_slot_index].second.get().m_parent = nullptr;
			m_children[in_slot_index].second.get().m_slot_index = -1;
			m_children.erase(m_children.begin() + in_slot_index);
		}

		virtual Ui_widget_update* find_slot_update_requested(size_t in_slot_index) override
		{
			return &get_slot(in_slot_index).m_update_requested;
		}

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			if (m_render_transform_was_updated_this_frame)
				return;

			m_render_transform_was_updated_this_frame = true;

			Ui_widget<T_subtype>::calculate_render_transform(in_window_size);

			for (auto&& child : m_children)
			{
				if (child.second.get().m_render_transform_was_updated_this_frame)
					continue;

				if (should_propagate_dirtyness_to_parent() || (ui8)m_update_requested & (ui8)Ui_widget_update::layout)
				{
					child.second.get().calculate_render_transform(in_window_size);
					child.second.get().m_render_transform_was_updated_this_frame = true;
				}
			}
		}

		virtual void gather_dependencies(std::vector<Asset_header*>& out_assets) const override
		{
			Ui_widget<T_subtype>::gather_dependencies(out_assets);
			for (auto && child : m_children)
				child.second.get().gather_dependencies(out_assets);
		}

		const Slot& get_slot(size_t in_slot_index) const { return m_children[in_slot_index].first; }
		Slot& get_slot(size_t in_slot_index) { return m_children[in_slot_index].first; }

		const Ui_widget_base& get_widget(size_t in_slot_index) const { return m_children[in_slot_index].second.get(); }
		Ui_widget_base& get_widget(size_t in_slot_index) { return m_children[in_slot_index].second.get(); }

		const std::pair<Slot, std::reference_wrapper<Ui_widget_base>>& get_child(size_t in_slot_index) const { return m_children[in_slot_index]; }
		std::pair<Slot, std::reference_wrapper<Ui_widget_base>>& get_child(size_t in_slot_index) { return m_children[in_slot_index]; }

		virtual void gather_widgets_over_point(const glm::vec2& in_point, std::vector<Ui_widget_base*>& out_widgets) override
		{
			//reverse so the widget rendered last gets the interaction first
			for (i32 i = (i32)m_children.size() - 1; i >= 0; i--)
				m_children[i].second.get().gather_widgets_over_point(in_point, out_widgets);

			Ui_widget<T_subtype>::gather_widgets_over_point(in_point, out_widgets);
		}

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Ui_widget<T_subtype>::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			if (m_render_state_was_updated_this_frame)
				return;

			for (auto&& child : m_children)
			{
				auto& child_widget = child.second.get();
				if (child_widget.m_render_state_was_updated_this_frame)
					continue;

				if (should_propagate_dirtyness_to_parent() || (ui8)m_update_requested & (ui8)Ui_widget_update::appearance)
				{
					child_widget.update_render_scene(child_widget.m_render_translation, child_widget.m_global_render_size, child_widget.m_render_rotation, in_window_size, in_window_id, inout_processor);
					child_widget.m_render_state_was_updated_this_frame = true;
				}
			}
		}

		virtual void destroy(Processor_ui& inout_processor) override
		{
			destroy_children(inout_processor);

			Ui_widget<T_subtype>::destroy(inout_processor);
		}

		virtual void destroy_children(Processor_ui& inout_processor)
		{
			for (sic::i32 i = static_cast<sic::i32>(get_child_count()) - 1; i >= 0; --i)
				destroy_child(static_cast<size_t>(i), inout_processor);
		}

		size_t get_child_count() const { return m_children.size(); }

		virtual void calculate_content_size(const glm::vec2& in_window_size, bool in_slot_determines_size_x, bool in_slot_determines_size_y) override
		{
			for (auto&& child : m_children)
			{
				auto determine_flags = child.first.get_overrides_content_size();
				child.second.get().calculate_content_size(in_window_size, determine_flags.first, determine_flags.second);
			}

			Ui_widget<T_subtype>::calculate_content_size(in_window_size, in_slot_determines_size_x, in_slot_determines_size_y);
		}

		virtual void propagate_request_to_children(Ui_widget_update in_request) override
		{
			for (auto&& child : m_children)
			{
				if ((ui8)child.second.get().m_update_requested & (ui8)in_request && child.second.get().m_visibility_state == m_visibility_state)
					continue;
				
				((ui8&)child.second.get().m_update_requested) |= (ui8)in_request;

				if (m_visibility_state != Ui_visibility::visible)
					child.second.get().m_visibility_state = m_visibility_state;

				child.second.get().propagate_request_to_children(in_request);
			}
		}

		virtual std::pair<bool, bool> get_slot_overrides_content_size(size_t in_slot_index) const { return get_slot(in_slot_index).get_overrides_content_size(); }

		virtual void set_window_id(const Update_list_id<Render_object_window>& in_window_id) override
		{
			Ui_widget<T_subtype>::set_window_id(in_window_id);

			for (auto&& child : m_children)
			{
				if (child.second.get().m_window_id.is_valid())
					continue;

				child.second.get().set_window_id(in_window_id);
			}
		}

		virtual Ui_widget_base* get_child_previous_widget_render_order(i32 in_slot_index) override
		{
			if (in_slot_index == 0)
				return this;

			Ui_widget_base& prev_widget = (m_children[(size_t)(in_slot_index - 1)].second.get());
			return prev_widget.get_leaf_widget_render_order();
		}

		virtual Ui_widget_base* get_next_widget_render_order()
		{
			if (m_children.empty())
				return Ui_widget_base::get_next_widget_render_order();

			return &m_children[0].second.get();
		}

		virtual Ui_widget_base* get_child_next_widget_render_order(i32 in_slot_index) override
		{
			if (in_slot_index + 1 >= m_children.size())
			{
				if (!m_parent)
					return nullptr;

				return m_parent->get_child_next_widget_render_order(m_slot_index);
			}

			return &(m_children[(size_t)(in_slot_index + 1)].second.get());
		}

		virtual Ui_widget_base* get_leaf_widget_render_order() override
		{
			if (m_children.empty())
				return this;

			return m_children.back().second.get().get_leaf_widget_render_order();
		}

	private:
		std::vector<std::pair<Slot, std::reference_wrapper<Ui_widget_base>>> m_children;
	};

	struct Ui_slot_tooltip : Ui_slot
	{
		friend System_ui;
		friend struct Ui_widget_tooltip;

		Ui_slot_tooltip& pivot(const glm::vec2& in_pivot) { m_pivot = in_pivot; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		//if not set, use cursor x
		Ui_slot_tooltip& attachment_pivot_x(std::optional<float> in_pivot) { m_attachment_pivot_x = in_pivot; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		//if not set, use cursor y
		Ui_slot_tooltip& attachment_pivot_y(std::optional<float> in_pivot) { m_attachment_pivot_y = in_pivot; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		//in pixels
		Ui_slot_tooltip& min(const glm::vec2& in_min) { m_min = in_min; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		//in pixels
		Ui_slot_tooltip& max(const glm::vec2& in_max) { m_max = in_max; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_slot_tooltip& time_until_destroy(float in_time_until_destroy) { m_time_until_destroy = in_time_until_destroy; request_update(Ui_widget_update::appearance); return *this; }

		glm::vec2 m_pivot = { 0.0f, 0.0f };
		
		//if not set, use cursor x
		std::optional<float> m_attachment_pivot_x;
		//if not set, use cursor y
		std::optional<float> m_attachment_pivot_y;

		std::optional<glm::vec2> m_min;
		std::optional<glm::vec2> m_max;

		float m_time_until_destroy = 0.0f;
	};

	struct Ui_widget_tooltip : Ui_parent_widget<Ui_widget_tooltip, Ui_slot_tooltip>
	{
		Ui_widget_tooltip& cursor_translation(const glm::vec2& in_cursor_translation) { m_cursor_translation = in_cursor_translation; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_tooltip, Ui_slot_tooltip>::calculate_render_transform(in_window_size);

			const glm::vec2 hovered_widget_top_left = m_render_translation;
			const glm::vec2 hovered_widget_bottom_right = hovered_widget_top_left + (m_global_render_size);
			const glm::vec2 cursor_loc = m_cursor_translation / in_window_size;

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child_ref] = get_child(i);
				auto& child = child_ref.get();

				if (slot.m_attachment_pivot_x.has_value())
					child.m_render_translation.x = glm::mix(hovered_widget_top_left.x, hovered_widget_bottom_right.x, slot.m_attachment_pivot_x.value());
				else
					child.m_render_translation.x = cursor_loc.x;

				if (slot.m_attachment_pivot_y.has_value())
					child.m_render_translation.y = glm::mix(hovered_widget_top_left.y, hovered_widget_bottom_right.y, slot.m_attachment_pivot_y.value());
				else
					child.m_render_translation.y = cursor_loc.y;

				child.m_render_rotation = m_render_rotation;

				const glm::vec2 render_size = child.get_content_size(in_window_size);

				child.m_global_render_size = render_size;
				child.m_render_translation -= child.m_global_render_size * slot.m_pivot;

				child.m_render_translation = glm::max(child.m_render_translation, slot.m_min.value_or(glm::vec2{ 0.0f, 0.0f }) / in_window_size);

				const glm::vec2 exceeding_over_max = child.m_render_translation + child.m_global_render_size - (slot.m_max.value_or(in_window_size) / in_window_size);

				if (exceeding_over_max.x > 0.0f)
					child.m_render_translation.x -= exceeding_over_max.x;
				if (exceeding_over_max.y > 0.0f)
					child.m_render_translation.y -= exceeding_over_max.y;

				child.calculate_render_transform(in_window_size);
			}
		}

		virtual bool should_propagate_dirtyness_to_parent() const { return false; }

		glm::vec2 m_cursor_translation = { 0.0f, 0.0f };
	};

	struct Ui_widget_spacer : Ui_widget<Ui_widget_spacer>
	{
		Ui_widget_spacer& size(const glm::vec2& in_size) { m_size = in_size; request_update(Ui_widget_update::layout); return *this; }
		Ui_widget_spacer& x(float in_size) { m_size.x = in_size; request_update(Ui_widget_update::layout); return *this; }
		Ui_widget_spacer& y(float in_size) { m_size.y = in_size; request_update(Ui_widget_update::layout); return *this; }

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override { return m_size / in_window_size; };

	private:
		glm::vec2 m_size = { 0.0f, 0.0f };
	};


	struct Ui_slot_canvas : Ui_slot
	{
		friend struct Ui_widget_canvas;

		Ui_slot_canvas& anchors(const Ui_anchors& in_anchors) { m_anchors = in_anchors; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& translation(const glm::vec2& in_translation) { m_translation = in_translation; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& size(const glm::vec2& in_size) { m_size = in_size; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& pivot(const glm::vec2& in_pivot) { m_pivot = in_pivot; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& rotation(float in_rotation) { m_rotation = in_rotation; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& autosize(bool in_autosize) { m_autosize = in_autosize; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_canvas& z_order(ui32 in_z_order) { m_z_order = in_z_order; request_update(Ui_widget_update::appearance); return *this; }

		const glm::vec2& get_translation() const { return m_translation; }

		virtual std::pair<bool, bool> get_overrides_content_size() const override { return { !m_autosize, !m_autosize }; }

	private:
		Ui_anchors m_anchors;

		glm::vec2 m_translation = { 0.0f, 0.0f };
		glm::vec2 m_size = { 0.0f, 0.0f };
		glm::vec2 m_pivot = { 0.0f, 0.0f };
		float m_rotation = 0;
		bool m_autosize = false;

		ui32 m_z_order = 0;
	};

	struct Ui_widget_canvas : Ui_parent_widget<Ui_widget_canvas, Ui_slot_canvas>
	{
		Ui_widget_canvas& reference_dimensions(const glm::vec2& in_reference_dimensions) { m_reference_dimensions = in_reference_dimensions; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_canvas, Ui_slot_canvas>::calculate_render_transform(in_window_size);

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);

				child.get().m_render_translation = m_render_translation;
				child.get().m_render_rotation = m_render_rotation;

				glm::vec2 anchor_vec = slot.m_anchors.get_max() - slot.m_anchors.get_min();
				glm::vec2 anchor_scale = anchor_vec;

				//if both anchors are 0 on all, translation should be relative to top left
				//if both anchors are 1 on all, translation should be relative to bottom right

				//if both anchors are not the same, translation should be relative to center of both
				glm::vec2 translation = (slot.m_translation / m_reference_dimensions) * m_global_render_size;
				translation += (slot.m_anchors.get_min() + (anchor_vec * 0.5f)) * m_global_render_size;

				glm::vec2 slot_size = slot.m_size;

				if (slot.m_anchors.get_min().x < slot.m_anchors.get_max().x)
					slot_size.x = (slot.m_anchors.get_max().x - slot.m_anchors.get_min().x) * in_window_size.x;

				if (slot.m_anchors.get_min().y < slot.m_anchors.get_max().y)
					slot_size.y = (slot.m_anchors.get_max().y - slot.m_anchors.get_min().y) * in_window_size.y;

				const glm::vec2 render_size = slot.m_autosize ? child.get().get_content_size(in_window_size) : ((slot_size / m_reference_dimensions) * m_global_render_size);

				child.get().m_global_render_size = render_size;
				child.get().m_render_translation += translation;
				child.get().m_render_translation -= child.get().m_global_render_size * slot.m_pivot;
				child.get().m_render_rotation += slot.m_rotation;

				child.get().calculate_render_transform(in_window_size);
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			return m_reference_dimensions / in_window_size;
		}

		virtual bool should_propagate_dirtyness_to_parent() const { return false; }

		glm::vec2 m_reference_dimensions = { 100.0f, 100.0f };
	};

	struct Ui_slot_vertical_box : Ui_slot
	{
		Ui_slot_vertical_box& padding(const Ui_padding& in_padding) { m_padding = in_padding;; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_vertical_box& h_align(Ui_h_alignment in_horizontal_alignment) { m_horizontal_alignment = in_horizontal_alignment;; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_slot_vertical_box& size_auto() { m_size.reset(); request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_vertical_box& size_fill(float in_fill_scale) { m_size = in_fill_scale; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_padding m_padding;
		Ui_h_alignment m_horizontal_alignment = Ui_h_alignment::fill;
		std::optional<float> m_size;
	};

	struct Ui_widget_vertical_box : Ui_parent_widget<Ui_widget_vertical_box, Ui_slot_vertical_box>
	{
		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_vertical_box, Ui_slot_vertical_box>::calculate_render_transform(in_window_size);

			float total_fill_value = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
				if (get_slot(i).m_size.has_value())
					total_fill_value += get_slot(i).m_size.has_value();

			float prev_y = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
			{
				const i32 prev_slot_index = static_cast<i32>(i) - 1;

				auto&& [slot, child] = get_child(i);

				child.get().m_render_translation = m_render_translation;
				child.get().m_render_rotation = m_render_rotation;

				child.get().m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;

				child.get().m_render_translation.y += prev_y;
				child.get().m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

				if (prev_slot_index >= 0)
				{
					const Ui_slot_vertical_box& prev_slot = get_slot(prev_slot_index);
					child.get().m_render_translation.y += prev_slot.m_padding.get_bottom() / in_window_size.y;
				}

				const float new_render_x = m_global_render_size.x - ((slot.m_padding.get_right() + slot.m_padding.get_left()) / in_window_size.x);

				align_horizontal(slot.m_horizontal_alignment, new_render_x, child.get().m_render_translation.x, child.get().m_global_render_size.x);

				if (slot.m_size.has_value())
					child.get().m_global_render_size.y = total_fill_value > 0.0f ? (m_global_render_size.y * (slot.m_size.value() / total_fill_value)) : 0.0f;

				child.get().calculate_render_transform(in_window_size);

				prev_y = child.get().m_render_translation.y - m_render_translation.y + child.get().m_global_render_size.y;
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);
				size.y += child.get().m_global_render_size.y;
				size.y += (slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y;

				const float child_content_x = child.get().m_global_render_size.x - ((slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x);

				size.x = glm::max(size.x, child_content_x);
			}

			return size;
		}
	};

	struct Ui_slot_horizontal_box : Ui_slot
	{
		Ui_slot_horizontal_box& padding(const Ui_padding& in_padding) { m_padding = in_padding;; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_horizontal_box& v_align(Ui_v_alignment in_alignment) { m_alignment = in_alignment;; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		
		Ui_slot_horizontal_box& size_auto() { m_size.reset(); request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_horizontal_box& size_fill(float in_fill_scale) { m_size = in_fill_scale; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_padding m_padding;
		Ui_v_alignment m_alignment = Ui_v_alignment::fill;
		std::optional<float> m_size;
	};

	struct Ui_widget_horizontal_box : Ui_parent_widget<Ui_widget_horizontal_box, Ui_slot_horizontal_box>
	{
		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_horizontal_box, Ui_slot_horizontal_box>::calculate_render_transform(in_window_size);

			float total_fill_value = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
				if (get_slot(i).m_size.has_value())
					total_fill_value += get_slot(i).m_size.has_value();

			float prev_x = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
			{
				const i32 prev_slot_index = static_cast<i32>(i) - 1;

				auto&& [slot, child] = get_child(i);

				child.get().m_render_translation = m_render_translation;
				child.get().m_render_rotation = m_render_rotation;

				child.get().m_render_translation.x += prev_x;
				child.get().m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;

				child.get().m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

				if (prev_slot_index >= 0)
				{
					const Ui_slot_horizontal_box& prev_slot = get_slot(prev_slot_index);
					child.get().m_render_translation.x += prev_slot.m_padding.get_left() / in_window_size.y;
				}

				const float new_render_y = m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

				align_vertical(slot.m_alignment, new_render_y, child.get().m_render_translation.y, child.get().m_global_render_size.y);

				if (slot.m_size.has_value())
					child.get().m_global_render_size.x = total_fill_value > 0.0f ? (m_global_render_size.x * (slot.m_size.value() / total_fill_value)) : 0.0f;

				child.get().calculate_render_transform(in_window_size);

				prev_x = child.get().m_render_translation.x - m_render_translation.x + child.get().m_global_render_size.x + (slot.m_padding.get_right() / in_window_size.x);
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);
				size.x += child.get().m_global_render_size.x;
				size.x += (slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x;

				const float child_content_y = child.get().m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

				size.y = glm::max(size.y, child_content_y);
			}

			return size;
		};
	};

	struct Ui_slot_uniform_grid_panel : Ui_slot
	{
		Ui_slot_uniform_grid_panel& h_align(Ui_h_alignment in_alignment) { m_horizontal_alignment = in_alignment; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_uniform_grid_panel& v_align(Ui_v_alignment in_alignment) { m_vertical_alignment = in_alignment; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		
		Ui_slot_uniform_grid_panel& row(size_t in_row) { m_row = in_row; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_uniform_grid_panel& column(size_t in_column) { m_column = in_column; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_h_alignment m_horizontal_alignment = Ui_h_alignment::left;
		Ui_v_alignment m_vertical_alignment = Ui_v_alignment::top;
		size_t m_row = 0;
		size_t m_column = 0;
	};

	struct Ui_widget_uniform_grid_panel : Ui_parent_widget<Ui_widget_uniform_grid_panel, Ui_slot_uniform_grid_panel>
	{
		Ui_widget_uniform_grid_panel& slot_padding(const Ui_padding& in_slot_padding) { m_slot_padding = in_slot_padding; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_uniform_grid_panel, Ui_slot_uniform_grid_panel>::calculate_render_transform(in_window_size);

			size_t rows = 0, columns = 0;

			const size_t child_count = get_child_count();

			for (size_t i = 0; i < child_count; i++)
			{
				auto&& [slot, child] = get_child(i);

				rows = glm::max(rows, slot.m_row + 1);
				columns = glm::max(columns, slot.m_column + 1);
			}

			const glm::vec2 total_slot_size_before_padding = { m_global_render_size.x / static_cast<float>(columns), m_global_render_size.y / static_cast<float>(rows) };

			for (size_t i = 0; i < child_count; i++)
			{
				auto&& [slot, child] = get_child(i);

				child.get().m_render_translation = m_render_translation;
				child.get().m_render_rotation = m_render_rotation;

				const float x_start = (total_slot_size_before_padding.x * static_cast<float>(slot.m_column)) + (m_slot_padding.get_left() / in_window_size.x);
				const float x_end = x_start + total_slot_size_before_padding.x - ((m_slot_padding.get_left() + m_slot_padding.get_right()) / in_window_size.x);

				const float y_start = (total_slot_size_before_padding.y * static_cast<float>(slot.m_row)) + (m_slot_padding.get_top() / in_window_size.y);
				const float y_end = y_start + total_slot_size_before_padding.y - ((m_slot_padding.get_top() + m_slot_padding.get_bottom()) / in_window_size.y);

				const float width = x_end - x_start;
				const float height = y_end - y_start;

				child.get().m_render_translation += glm::vec2(x_start, y_start);

				align_horizontal(slot.m_horizontal_alignment, width, child.get().m_render_translation.x, child.get().m_global_render_size.x);
				align_vertical(slot.m_vertical_alignment, height, child.get().m_render_translation.y, child.get().m_global_render_size.y);

				child.get().calculate_render_transform(in_window_size);
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };
			size_t rows = 0, columns = 0;

			const size_t child_count = get_child_count();

			for (size_t i = 0; i < child_count; i++)
			{
				auto&& [slot, child] = get_child(i);

				size.x = glm::max(size.x, child.get().m_global_render_size.x);
				size.y = glm::max(size.y, child.get().m_global_render_size.y);

				rows = glm::max(rows, slot.m_row + 1);
				columns = glm::max(columns, slot.m_column + 1);
			}

			size.x *= columns;
			size.y *= rows;

			size.x += columns * ((m_slot_padding.get_left() + m_slot_padding.get_right()) / in_window_size.x);
			size.y += rows * ((m_slot_padding.get_top() + m_slot_padding.get_bottom()) / in_window_size.y);

			return size;
		};

	private:
		Ui_padding m_slot_padding;
	};

	struct Ui_slot_panel : Ui_slot
	{
		Ui_slot_panel& h_align(Ui_h_alignment in_alignment) { m_horizontal_alignment = in_alignment; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_panel& v_align(Ui_v_alignment in_alignment) { m_vertical_alignment = in_alignment; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_slot_panel& padding(const Ui_padding& in_padding) { m_padding = in_padding; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_h_alignment m_horizontal_alignment = Ui_h_alignment::left;
		Ui_v_alignment m_vertical_alignment = Ui_v_alignment::top;
		Ui_padding m_padding;
	};

	struct Ui_widget_panel : Ui_parent_widget<Ui_widget_panel, Ui_slot_panel>
	{
		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_panel, Ui_slot_panel>::calculate_render_transform(in_window_size);

			const size_t child_count = get_child_count();

			for (size_t i = 0; i < child_count; i++)
			{
				auto&& [slot, child] = get_child(i);

				const float x_start = slot.m_padding.get_left() / in_window_size.x;
				const float x_end = x_start + m_global_render_size.x - ((slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x);

				const float y_start = slot.m_padding.get_top() / in_window_size.y;
				const float y_end = y_start + m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

				const float max_width = x_end - x_start;
				const float max_height = y_end - y_start;

				child.get().m_render_translation = m_render_translation;
				child.get().m_render_rotation = m_render_rotation;

				child.get().m_render_translation += glm::vec2(x_start, y_start);

				align_horizontal(slot.m_horizontal_alignment, max_width, child.get().m_render_translation.x, child.get().m_global_render_size.x);
				align_vertical(slot.m_vertical_alignment, max_height, child.get().m_render_translation.y, child.get().m_global_render_size.y);

				child.get().calculate_render_transform(in_window_size);
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };
			const size_t child_count = get_child_count();

			for (size_t i = 0; i < child_count; i++)
			{
				auto&& [slot, child] = get_child(i);

				const float padding_size_x = (slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x;
				const float padding_size_y = (slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y;

				size.x = glm::max(size.x, child.get().m_global_render_size.x + padding_size_x);
				size.y = glm::max(size.y, child.get().m_global_render_size.y + padding_size_y);
			}

			return size;
		};
	};

	struct Ui_widget_image : Ui_widget<Ui_widget_image>
	{
		Ui_widget_image& material(const Asset_ref<Asset_material>& in_asset_ref) { m_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_image& image_size(const glm::vec2& in_size) { m_image_size = in_size; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_image& tint(const glm::vec4& in_tint) { m_tint = in_tint; request_update(Ui_widget_update::appearance); return *this; }

		const Asset_ref<Asset_material>& get_material() const { return m_material; }
		const glm::vec2& get_image_size() const { return m_image_size; }
		const glm::vec4& get_tint() const { return m_tint; }

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override {  return m_image_size / in_window_size; };

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Ui_widget<Ui_widget_image>::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);
			Update_image_common(inout_processor, *this, m_ro_id, m_material, m_tint, in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		void destroy(Processor_ui& inout_processor) override
		{
			destroy_render_object(inout_processor, m_ro_id);
			Ui_widget<Ui_widget_image>::destroy(inout_processor);
		}

		std::optional<size_t> m_ro_id;

	private:
		Asset_ref<Asset_material> m_material;
		glm::vec2 m_image_size = { 64.0f, 64.0f };
		glm::vec4 m_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct Ui_slot_button : Ui_slot
	{
		Ui_slot_button& padding(const Ui_padding& in_padding) { m_padding = in_padding; return *this; }
		Ui_slot_button& h_align(Ui_h_alignment in_horizontal_alignment) { m_horizontal_alignment = in_horizontal_alignment; return *this; }
		Ui_slot_button& v_align(Ui_v_alignment in_vertical_alignment) { m_vertical_alignment = in_vertical_alignment; return *this; }

		Ui_padding m_padding;
		Ui_h_alignment m_horizontal_alignment = Ui_h_alignment::center;
		Ui_v_alignment m_vertical_alignment = Ui_v_alignment::center;
	};

	struct Ui_widget_button : Ui_parent_widget<Ui_widget_button, Ui_slot_button>
	{
		struct On_toggled : Ui_delegate<bool> {};

		//assigns all materials that have not yet been assigned
		Ui_widget_button& base_material(const Asset_ref<Asset_material>& in_asset_ref)
		{
			if (!m_idle_material.is_valid())
				m_idle_material = in_asset_ref;

			if (!m_hovered_material.is_valid())
				m_hovered_material = in_asset_ref;

			if (!m_pressed_material.is_valid())
				m_pressed_material = in_asset_ref;

			request_update(Ui_widget_update::appearance);

			return *this;
		}

		Ui_widget_button& idle_material(const Asset_ref<Asset_material>& in_asset_ref) { m_idle_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_button& hovered_material(const Asset_ref<Asset_material>& in_asset_ref) { m_hovered_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_button& pressed_material(const Asset_ref<Asset_material>& in_asset_ref) { m_pressed_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }

		Ui_widget_button& tints(const glm::vec4& in_tint_idle, const glm::vec4& in_tint_hovered, const glm::vec4& in_tint_pressed)
		{
			m_tint_idle = in_tint_idle;
			m_tint_hovered = in_tint_hovered;
			m_tint_pressed = in_tint_pressed;

			request_update(Ui_widget_update::appearance);

			return *this;
		}

		Ui_widget_button& tint_idle(const glm::vec4& in_tint_idle) { m_tint_idle = in_tint_idle; request_update(Ui_widget_update::appearance); return *this;	}
		Ui_widget_button& tint_hovered(const glm::vec4& in_tint_hovered) { m_tint_hovered = in_tint_hovered; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_button& tint_pressed(const glm::vec4& in_tint_pressed) { m_tint_pressed = in_tint_pressed; request_update(Ui_widget_update::appearance); return *this; }

		Ui_widget_button& size(const glm::vec2& in_size) { m_size = in_size; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_widget_button& toggled(bool in_is_toggled) { m_toggled = in_is_toggled; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_button& is_toggle(bool in_is_toggle) { m_is_toggle = in_is_toggle; request_update(Ui_widget_update::appearance); return *this; }


		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override {  return m_size / in_window_size; };

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Asset_ref<Asset_material>* mat = nullptr;
			glm::vec4 tint = {};
			switch (get_interaction_state())
			{
			case sic::Ui_interaction_state::idle:
				mat = &m_idle_material;
				tint = m_tint_idle;
				break;
			case sic::Ui_interaction_state::hovered:
				mat = &m_hovered_material;
				tint = m_tint_hovered;
				break;
			case sic::Ui_interaction_state::pressed:
				mat = &m_pressed_material;
				tint = m_tint_pressed;
				break;
			case sic::Ui_interaction_state::pressed_not_hovered:
				mat = &m_hovered_material;
				tint = m_tint_hovered;
				break;
			default:
				break;
			}

			Update_image_common(inout_processor, *this, m_ro_id, *mat, tint, in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id);
			Ui_parent_widget<Ui_widget_button, Ui_slot_button>::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);
		}

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_parent_widget<Ui_widget_button, Ui_slot_button>::calculate_render_transform(in_window_size);

			if (get_child_count() == 0)
				return;

			auto&& [slot, child] = get_child(0);

			child.get().m_render_translation = m_render_translation;
			child.get().m_render_rotation = m_render_rotation;

			child.get().m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;
			child.get().m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

			const float new_render_x = m_global_render_size.x - ((slot.m_padding.get_right() + slot.m_padding.get_left()) / in_window_size.x);
			const float new_render_y = m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

			align_horizontal(slot.m_horizontal_alignment, new_render_x, child.get().m_render_translation.x, child.get().m_global_render_size.x);
			align_vertical(slot.m_vertical_alignment, new_render_y, child.get().m_render_translation.y, child.get().m_global_render_size.y);

			child.get().calculate_render_transform(in_window_size);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override
		{
			Ui_parent_widget<Ui_widget_button, Ui_slot_button>::gather_dependencies(out_assets);

			out_assets.push_back(m_idle_material.get_header());
			out_assets.push_back(m_hovered_material.get_header());
			out_assets.push_back(m_pressed_material.get_header());
		}

		virtual void on_interaction_state_changed() { request_update(Ui_widget_update::appearance); }

		virtual void on_clicked(Mousebutton in_button, const glm::vec2& in_cursor_pos) override
		{
			Ui_parent_widget<Ui_widget_button, Ui_slot_button>::on_clicked(in_button, in_cursor_pos);

			if (m_is_toggle)
			{
				m_toggled = !m_toggled;
				invoke<On_toggled>(m_toggled);
			}
		}

		void destroy(Processor_ui& inout_processor) override
		{
			destroy_render_object(inout_processor, m_ro_id);
			Ui_parent_widget<Ui_widget_button, Ui_slot_button>::destroy(inout_processor);
		}

		glm::vec2 m_size = { 64.0f, 64.0f };

		Asset_ref<Asset_material> m_idle_material;
		Asset_ref<Asset_material> m_hovered_material;
		Asset_ref<Asset_material> m_pressed_material;

		glm::vec4 m_tint_idle = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_hovered = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_pressed = { 1.0f, 1.0f, 1.0f, 1.0f };

		std::optional<size_t> m_ro_id;
		bool m_toggled = false;
		bool m_is_toggle = false;
	};

	struct Ui_widget_text : Ui_widget<Ui_widget_text>
	{
		Ui_widget_text& text(const std::string in_text) { if (m_text == in_text)  return *this; m_text = in_text; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& font(const Asset_ref<Asset_font>& in_asset_ref) { m_font = in_asset_ref; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& material(const Asset_ref<Asset_material>& in_asset_ref) { m_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& px(float in_px) { m_px = in_px; request_update(Ui_widget_update::layout_and_appearance); return *this; }

		Ui_widget_text& line_height_percentage(float in_line_height_percentage) { m_line_height_percentage = in_line_height_percentage; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& align(Ui_h_alignment in_alignment) { m_alignment = in_alignment; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& autowrap(bool in_autowrap) { m_autowrap = in_autowrap; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& wrap_text_at(std::optional<float> in_wrap_text_at) { m_wrap_text_at = in_wrap_text_at; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_text& foreground_color(const glm::vec4& in_foreground_color) { m_foreground_color = in_foreground_color; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& background_color(const glm::vec4& in_background_color) { m_background_color = in_background_color; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& selected_foreground_color(const glm::vec4& in_foreground_color) { m_selected_foreground_color = in_foreground_color; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& selected_background_color(const glm::vec4& in_background_color) { m_selected_background_color = in_background_color; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& selectable(bool in_selectable) { m_selectable = in_selectable; if (!m_selectable) { m_selection_start.reset(); m_selection_end.reset(); request_update(Ui_widget_update::appearance); } return *this; }

		Ui_widget_text& selection_box_material(Asset_ref<Asset_material> in_material) { m_selection_box_material = in_material; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& selection_box_image_size(const::glm::vec2& in_size) { m_selection_box_image_size = in_size; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_text& selection_box_tint(const::glm::vec4& in_tint) { m_selection_box_tint = in_tint; request_update(Ui_widget_update::appearance); return *this; }

		const std::string& get_text() const { return m_text; }
		const glm::vec4& get_foreground_color() const { return m_foreground_color; }

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			const Asset_font* font = m_font.get();
			const float scale_ratio = m_px / font->m_em_size;

			float longest_width = 0.0f;
			float height = 0.0f;

			if (m_text.empty())
				return { 0.0f, 0.0f };

			float cur_line_advance = 0.0f;

			float width_until_last_word = 0.0f;
			i32 last_word_end = 0;

			for (size_t i = 0; i < m_text.size(); i++)
			{
				const auto c = m_text[i];
				const auto& glyph = font->m_glyphs[c];

				if (c == '\n')
				{
					longest_width = (glm::max)(longest_width, cur_line_advance);

					if (i + 1 >= m_text.size())
						break;

					height += (font->m_max_glyph_height * m_line_height_percentage);

					last_word_end = -1;
					width_until_last_word = 0.0f;
					cur_line_advance = 0.0f;

					continue;
				}
				else if (c == ' ')
				{
					last_word_end = (i32)i - 1;
					width_until_last_word = cur_line_advance;
				}

				float kerning = 0.0f;
				if (cur_line_advance > 0.0f)
					kerning = glyph.m_kerning_values[m_text[i - 1]];

				cur_line_advance += glyph.m_pixel_advance + kerning;

				longest_width = (glm::max)(longest_width, cur_line_advance);

				if ((m_wrap_text_at.has_value() && cur_line_advance * scale_ratio > m_wrap_text_at.value()))
				{
					if (last_word_end + 2 >= m_text.size())
						break;

					height += (font->m_max_glyph_height * m_line_height_percentage);
					longest_width = (glm::max)(longest_width, width_until_last_word);

					last_word_end = (i32)i - 1;
					width_until_last_word = 0.0f;
					cur_line_advance = 0.0f;
				}
			}

			const float x = longest_width / in_window_size.x;
			const float y = (height + (font->m_max_glyph_height * m_line_height_percentage)) / in_window_size.y;

			return { x * scale_ratio, y * scale_ratio };
		}

		virtual void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_widget<Ui_widget_text>::calculate_render_transform(in_window_size);

			if (!m_autowrap)
				return;

			const Asset_font* font = m_font.get();

			if (!font)
				return;

			float new_size_y = font->m_max_glyph_height;
			float cur_size_x = 0.0f;

			for (size_t i = 0; i < m_text.size(); i++)
			{
				const auto c = m_text[i];

				if (c == '\n' ||
					(m_autowrap && cur_size_x + font->m_glyphs[c].m_pixel_advance > m_global_render_size.x * in_window_size.x))
				{
					new_size_y += font->m_max_glyph_height;

					cur_size_x = 0.0f;
					continue;
				}

				cur_size_x += font->m_glyphs[c].m_pixel_advance;

				if (i > 0)
					cur_size_x += font->m_glyphs[c].m_kerning_values[m_text[i - 1]];
			}

			const float scale_ratio = m_px / font->m_em_size;
			new_size_y *= scale_ratio;

			m_global_render_size.y = new_size_y / in_window_size.y;
		}

		virtual void on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement) override
		{
			Ui_widget<Ui_widget_text>::on_cursor_move_over(in_cursor_pos, in_cursor_movement);

			if (!m_selectable)
				return;

			if (!m_is_selecting)
				return;

			if (m_selection_end.value() != in_cursor_movement)
				request_update(Ui_widget_update::appearance);

			m_selection_end = in_cursor_pos;
		}
		virtual void on_pressed(Mousebutton in_button, const glm::vec2& in_cursor_pos) override
		{
			Ui_widget<Ui_widget_text>::on_pressed(in_button, in_cursor_pos);
			
			if (!m_selectable)
				return;

			if (in_button != Mousebutton::left)
				return;

			if (m_is_selecting)
				return;

			m_selection_start_index.reset();
			m_selection_end_index.reset();

			m_selection_start = m_selection_end = in_cursor_pos;
			request_update(Ui_widget_update::appearance);
			m_is_selecting = true;
		}

		virtual void on_released(Mousebutton in_button, const glm::vec2& in_cursor_pos) override
		{
			Ui_widget<Ui_widget_text>::on_released(in_button, in_cursor_pos);

			if (!m_selectable)
				return;

			if (in_button != Mousebutton::left || !m_selection_start.has_value())
				return;

			m_selection_end = in_cursor_pos;
			request_update(Ui_widget_update::appearance);
			m_is_selecting = false;
		}

		virtual void on_focus_end() override
		{
			Ui_widget<Ui_widget_text>::on_focus_end();

			if (!m_selectable)
				return;

			if (!m_selection_start.has_value())
				return;

			m_selection_start.reset();
			m_selection_end.reset();
			request_update(Ui_widget_update::appearance);
			m_is_selecting = false;
		}

		virtual std::optional<std::string> get_clipboard_string() override
		{
			if (!m_selection_start_index.has_value() || !m_selection_end_index.has_value())
				return {};

			return m_text.substr(m_selection_start_index.value(), m_selection_end_index.value() - m_selection_start_index.value());
		}

		Asset_ref<Asset_material> m_material;
		Asset_ref<Asset_font> m_font;
		std::string m_text;
		Ui_h_alignment m_alignment = Ui_h_alignment::left;
		float m_px = 16.0f;
		float m_line_height_percentage = 1.0f;
		std::optional<float> m_wrap_text_at;
		bool m_autowrap = false;
		glm::vec4 m_foreground_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		glm::vec4 m_background_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		glm::vec4 m_selected_foreground_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 m_selected_background_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		bool m_selectable = false;
		std::optional<glm::vec2> m_selection_start;
		std::optional<glm::vec2> m_selection_end;
		std::optional<size_t> m_selection_start_index;
		std::optional<size_t> m_selection_end_index;

		Asset_ref<Asset_material> m_selection_box_material;
		glm::vec2 m_selection_box_image_size = { 64.0f, 64.0f };
		glm::vec4 m_selection_box_tint = { 1.0f, 1.0f, 1.0f, 1.0f };

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Ui_widget<Ui_widget_text>::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			const Asset_font* font = m_font.get();

			if (!font)
				return;

			const float scale_ratio = m_px / font->m_em_size;

			for (size_t remove_glyph_idx = m_text.size(); remove_glyph_idx < m_glyph_instances.size(); ++remove_glyph_idx)
				destroy_render_object(inout_processor, m_glyph_instances[remove_glyph_idx].m_ro_id);

			m_glyph_instances.resize(m_text.size());

			generate_lines(in_final_size.x * in_window_size.x);

			size_t prev_line_end = 0;

			for (size_t i = m_lines.size(); i < m_selection_box_images.size(); i++)
				destroy_render_object(inout_processor, m_selection_box_images[i]);
			
			m_selection_box_images.resize(m_lines.size());

			for (size_t line_idx = 0; line_idx < m_lines.size(); ++line_idx)
			{
				const Text_line& line = m_lines[line_idx];

				for (size_t remove_glyph_idx = prev_line_end; remove_glyph_idx < line.m_start_index; ++remove_glyph_idx)
					destroy_render_object(inout_processor, m_glyph_instances[remove_glyph_idx].m_ro_id);

				prev_line_end = line.m_end_index + 1;

				float cur_advance = 0.0f;

				if (m_alignment == Ui_h_alignment::right)
					cur_advance = (in_final_size.x * in_window_size.x) - line.m_total_width;
				else if (m_alignment == Ui_h_alignment::center)
					cur_advance = (in_final_size.x * in_window_size.x * 0.5f) - (line.m_total_width * 0.5f);

				auto& selection_box_image = m_selection_box_images[line_idx];

				for (size_t i = line.m_start_index; i <= line.m_end_index; i++)
				{
					auto c = m_text[i];
					auto& glyph_instance = m_glyph_instances[i];
					const auto& glyph = font->m_glyphs[c];

					const glm::vec2 top_left_start = in_final_translation + ((glm::vec2(cur_advance, font->m_em_size + line.m_y) * scale_ratio) / in_window_size);
					glm::vec2 render_translation = top_left_start + ((glyph.m_offset_to_glyph * scale_ratio) / in_window_size);

					float kerning = 0.0f;
					if (i > line.m_start_index)
					{
						kerning = glyph.m_kerning_values[m_text[i - 1]];
						render_translation.x += (kerning * scale_ratio) / in_window_size.x;
					}

					cur_advance += glyph.m_pixel_advance + kerning;

					const glm::vec2 render_size = ((glyph.m_atlas_pixel_size * scale_ratio) / in_window_size);

					const glm::vec2 atlas_offset = (glyph.m_atlas_pixel_position / glm::vec2(font->m_width, font->m_height));
					const glm::vec2 atlas_glyph_size = (glyph.m_atlas_pixel_size / glm::vec2(font->m_width, font->m_height));

					glyph_instance.m_top_left = render_translation;
					if (render_size.x == 0.0f)
						glyph_instance.m_bottom_right = render_translation + render_size + glm::vec2{ (glyph.m_pixel_advance + kerning) / in_window_size.x, 0.0f };
					else
						glyph_instance.m_bottom_right = render_translation + render_size;
					const glm::vec2 center = glyph_instance.m_top_left + (render_size * 0.5f);

					//only cull edges if we are not auto-wrapping
					if (!m_autowrap)
					{
						const float width_overflow_right = (glyph_instance.m_bottom_right.x - (top_left_start.x - render_translation.x)) - (in_final_translation.x + in_final_size.x);

						//cull only if it overflows by half the character or more
						if (width_overflow_right >= in_final_size.x * 0.5f)
						{
							destroy_render_object(inout_processor, glyph_instance.m_ro_id);
							continue;
						}
						else if (top_left_start.x < in_final_translation.x - (in_final_size.x * 0.5f))
						{
							destroy_render_object(inout_processor, glyph_instance.m_ro_id);
							continue;
						}
					}

					glm::vec4 fg_color = m_foreground_color;
					glm::vec4 bg_color = m_background_color;

					glyph_instance.m_is_selected = false;
					if (m_selectable && m_selection_start.has_value() && m_selection_end.has_value())
					{
						const float y_min = in_final_translation.y + ((line.m_y * scale_ratio) / in_window_size.y);
						const float y_max = in_final_translation.y + (((font->m_max_glyph_height + line.m_y) * scale_ratio) / in_window_size.y);

						glm::vec2 selection_start = { m_selection_start.value().x, glm::min(m_selection_start.value().y, m_selection_end.value().y) };
						glm::vec2 selection_end = { m_selection_end.value().x, glm::max(m_selection_start.value().y, m_selection_end.value().y) };

						bool selected = false;

						const bool start_is_above_line = selection_start.y < y_min;
						const bool end_is_below_line = selection_end.y > y_max;

						const bool start_is_within_line = selection_start.y >= y_min && selection_start.y <= y_max;
						const bool end_is_within_line = selection_end.y >= y_min && selection_end.y <= y_max;

						const bool selecting_above_onto_current = start_is_above_line && end_is_within_line && center.x <= selection_end.x;
						if (selecting_above_onto_current)
							selected = true;

						const bool selecting_below_onto_current = end_is_below_line && start_is_within_line && center.x >= selection_start.x;
						if (selecting_below_onto_current)
							selected = true;

						if (start_is_above_line && end_is_below_line)
							selected = true;

						if (start_is_within_line && end_is_within_line)
						{
							if (center.x >= selection_start.x && center.x <= selection_end.x)
								selected = true;
						}

						if (selected)
						{
							fg_color = m_selected_foreground_color;
							bg_color = m_selected_background_color;

							if (!m_selection_start_index.has_value() || m_selection_start_index.value() > i)
								m_selection_start_index = i;

							if (!m_selection_end_index.has_value() || m_selection_end_index.value() < i + 1)
								m_selection_end_index = i + 1;

							glyph_instance.m_is_selected = true;
						}
					}

					if (c != ' ' && c != '\n')
					{
						auto update_lambda =
						[font_ref = m_font, top_left = glyph_instance.m_top_left, bottom_right = glyph_instance.m_bottom_right, in_final_rotation, mat = m_material, in_window_size, atlas_offset, atlas_glyph_size, fg_color, bg_color](Render_object_ui& inout_object)
						{
							if (inout_object.m_material != mat.get_mutable())
							{
								if (inout_object.m_material)
									inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index);

								inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
								inout_object.m_material = mat.get_mutable();
							}

							const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

							mat.get_mutable()->set_parameter_on_instance("lefttop_rightbottom_packed", lefttop_rightbottom_packed, inout_object.m_instance_data_index);
							mat.get_mutable()->set_parameter_on_instance("msdf", font_ref.as<Asset_texture_base>(), inout_object.m_instance_data_index);
							mat.get_mutable()->set_parameter_on_instance("offset_and_size", glm::vec4(atlas_offset, atlas_glyph_size), inout_object.m_instance_data_index);
							mat.get_mutable()->set_parameter_on_instance("fg_color", fg_color, inout_object.m_instance_data_index);
							mat.get_mutable()->set_parameter_on_instance("bg_color", bg_color, inout_object.m_instance_data_index);
						};

						update_render_object(inout_processor, glyph_instance.m_ro_id, update_lambda);
					}
					else
					{
						destroy_render_object(inout_processor, glyph_instance.m_ro_id);
					}
				}

				if (m_selection_start.has_value() && m_selection_end.has_value())
				{
					std::optional<glm::vec2> selection_start, selection_end;
					for (size_t i = line.m_start_index; i <= line.m_end_index; i++)
					{
						auto& glyph_instance = m_glyph_instances[i];
						if (glyph_instance.m_is_selected)
						{
							if (!selection_start.has_value())
								selection_start = glyph_instance.m_top_left;

							selection_end = glyph_instance.m_bottom_right;
						}
					}

					if (selection_start.has_value() && selection_end.has_value())
					{
						const float x_min = selection_start.value().x;
						const float x_max = selection_end.value().x;
						const float y_min = in_final_translation.y;
						const float y_max = in_final_translation.y + (((font->m_max_glyph_height + line.m_y) * scale_ratio) / in_window_size.y);

						Update_image_common(inout_processor, *this, selection_box_image, m_selection_box_material, m_selection_box_tint, { x_min, y_min }, { x_max - x_min, y_max - y_min }, 0.0f, in_window_size, in_window_id);
					}
					else
					{
						Update_image_common(inout_processor, *this, selection_box_image, m_selection_box_material, { 0.0f, 0.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 0.0f, in_window_size, in_window_id);
					}
				}
				else
				{
					Update_image_common(inout_processor, *this, selection_box_image, m_selection_box_material, { 0.0f, 0.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 0.0f, in_window_size, in_window_id);
				}
			}

			for (size_t remove_glyph_idx = prev_line_end; remove_glyph_idx < m_text.size(); ++remove_glyph_idx)
				destroy_render_object(inout_processor, m_glyph_instances[remove_glyph_idx].m_ro_id);
		}

		void destroy(Processor_ui& inout_processor) override
		{
			for (auto&& glyph_instance : m_glyph_instances)
				destroy_render_object(inout_processor, glyph_instance.m_ro_id);

			for (auto& image : m_selection_box_images)
				destroy_render_object(inout_processor, image);

			m_glyph_instances.clear();
			m_selection_box_images.clear();

			Ui_widget<Ui_widget_text>::destroy(inout_processor);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		struct Glyph
		{
			std::optional<size_t> m_ro_id;
			glm::vec2 m_top_left;
			glm::vec2 m_bottom_right;
			bool m_is_selected = false;
		};

		std::vector<Glyph> m_glyph_instances;
		std::vector<std::optional<size_t>> m_selection_box_images;

	private:
		void generate_lines(float in_max_line_width)
		{
			m_lines.clear();

			const Asset_font* font = m_font.get();
			const float scale_ratio = m_px / font->m_em_size;
			Text_line* cur_line = nullptr;

			if (!m_text.empty())
			{
				m_lines.push_back(Text_line());
				cur_line = &m_lines.back();
			}
			else
			{
				return;
			}

			float cur_line_advance = 0.0f;

			float width_until_last_word = 0.0f;
			i32 last_word_end = 0;

			for (size_t i = 0; i < m_text.size(); i++)
			{
				const auto c = m_text[i];
				const auto& glyph = font->m_glyphs[c];
				auto& glyph_instance = m_glyph_instances[i];

				if (c == '\n')
				{
					cur_line->m_end_index = i - 1;
					cur_line->m_total_width = cur_line_advance;

					if (i + 1 >= m_text.size())
						break;

					Text_line new_line;
					new_line.m_start_index = i + 1;
					new_line.m_end_index = i + 1;
					new_line.m_y = cur_line->m_y + (font->m_max_glyph_height * m_line_height_percentage);
					m_lines.push_back(new_line);
					cur_line = &m_lines.back();

					last_word_end = -1;
					width_until_last_word = 0.0f;
					cur_line_advance = cur_line->m_total_width;

					continue;
				}
				else if (c == ' ')
				{
					last_word_end = (i32)i - 1;
					width_until_last_word = cur_line_advance;
				}

				float kerning = 0.0f;
				if (i > cur_line->m_start_index)
					kerning = glyph.m_kerning_values[m_text[i - 1]];

				cur_line_advance += glyph.m_pixel_advance + kerning;

				cur_line->m_end_index = i;
				cur_line->m_total_width = cur_line_advance;

				if ((m_wrap_text_at.has_value() && cur_line->m_total_width * scale_ratio > m_wrap_text_at.value()) ||
					(m_autowrap && cur_line->m_total_width * scale_ratio > in_max_line_width))
				{
					if (last_word_end + 2 >= m_text.size())
						break;
					
					Text_line new_line;
					new_line.m_start_index = last_word_end + 2;
					new_line.m_end_index = glm::max((i32)cur_line->m_end_index, last_word_end + 2);
					new_line.m_y = cur_line->m_y + (font->m_max_glyph_height * m_line_height_percentage);
					new_line.m_total_width = cur_line->m_total_width - width_until_last_word - font->m_glyphs[' '].m_pixel_advance;

					cur_line->m_end_index = glm::max(0, last_word_end);
					cur_line->m_total_width = width_until_last_word;

					m_lines.push_back(new_line);

					cur_line = &m_lines.back();

					last_word_end = (i32)i - 1;
					width_until_last_word = 0.0f;
					cur_line_advance = cur_line->m_total_width;
				}
			}
		}

		struct Text_line
		{
			size_t m_start_index = 0;
			size_t m_end_index = 0;

			float m_total_width = 0.0f;
			float m_y = 0.0f;
		};

		std::vector<Text_line> m_lines;
		bool m_is_selecting = false;
	};

}
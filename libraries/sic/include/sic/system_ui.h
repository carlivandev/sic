#pragma once
#include "sic/state_render_scene.h"
#include "sic/system_window.h"
#include "sic/input.h"

#include "sic/core/system.h"
#include "sic/core/delegate.h"

#include "glm/vec2.hpp"

#include <optional>

namespace sic
{
	extern Log g_log_ui;
	extern Log g_log_ui_verbose;

	struct State_ui;
	using Processor_ui = Processor<Processor_flag_write<State_ui>, Processor_flag_deferred_write<State_render_scene>, Processor_flag_read<State_input>, Processor_flag_read<State_window>>;

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

	struct Ui_parent_widget_base;
	struct State_ui;

	enum struct Interaction_consume
	{
		consume,
		fall_through
	};

	enum struct Ui_interaction_state
	{
		idle,
		hovered,
		pressed,
		pressed_not_hovered,
	};

	enum struct Ui_widget_update : ui8
	{
		none = 0,
		layout = 1,
		appearance = 2,
		layout_and_appearance = 3
	};

	struct Ui_widget : Noncopyable
	{
		struct On_pressed : Delegate<> {};
		struct On_released : Delegate<> {};
		struct On_clicked : Delegate<> {};
		struct On_hover_begin : Delegate<> {};
		struct On_hover_end : Delegate<> {};
		struct On_cursor_move_over : Delegate<> {};
		struct On_drag : Delegate<const glm::vec2&, const glm::vec2&> {};
		struct On_character_input : Delegate<unsigned int> {};
		struct On_focus_begin : Delegate<> {};
		struct On_focus_end : Delegate<> {};

		friend struct State_ui;
		friend struct System_ui;

		template <typename T_slot_type>
		friend struct Ui_parent_widget;

		Ui_widget& interaction_consume(Interaction_consume in_interaction_consume_type) { m_interaction_consume_type = in_interaction_consume_type; return *this; }

		virtual void calculate_render_transform(const glm::vec2& in_window_size);
		virtual void calculate_content_size(const glm::vec2& in_window_size) { m_global_render_size = get_content_size(in_window_size); }
		virtual void calculate_sort_priority(ui32& inout_current_sort_priority) { m_sort_priority = ++inout_current_sort_priority; }

		virtual glm::vec2 get_content_size(const glm::vec2& in_window_size) const { in_window_size; return glm::vec2(0.0f, 0.0f); }

		Ui_widget* get_outermost_parent();
		const Ui_parent_widget_base* get_parent() const { return m_parent; }

		ui32 get_slot_index() const { return m_slot_index; }

		//this overrides all sorting behaviour of UI!
		void set_sort_priority(ui32 in_sort_priority) { m_sort_priority = in_sort_priority; }
		ui32 get_sort_priority() const { return m_sort_priority; }

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
		virtual void gather_widgets_over_point(const glm::vec2& in_point, std::vector<Ui_widget*>& out_widgets) { if (is_point_inside(in_point)) out_widgets.push_back(this); }

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

		glm::vec2 m_local_scale = { 1.0f, 1.0f };
		glm::vec2 m_local_translation = { 0.0f, 0.0f };
		float m_local_rotation = 0.0f;

		i32 m_widget_index = -1;
		bool m_allow_dependency_streaming = false;

		glm::vec2 m_render_translation;
		glm::vec2 m_global_render_size;
		float m_render_rotation;

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor)
		{ in_final_translation; in_final_size; in_final_rotation; in_window_size; in_window_id; inout_processor; ((ui8&)m_update_requested) &= ~(ui8)Ui_widget_update::appearance; }

		void request_update(Ui_widget_update in_update) { ((ui8&)m_update_requested) |= (ui8)in_update; }
		
		virtual void destroy(State_ui& inout_ui_state);

		void destroy_render_object(State_ui& inout_ui_state, Update_list_id<Render_object_ui> in_ro_id) const;

		Ui_widget() = default;

	private:
		std::string m_key;
		Ui_parent_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		ui32 m_sort_priority = 0;
		On_asset_loaded::Handle m_dependencies_loaded_handle;
		State_ui* m_ui_state = nullptr;

		bool m_correctly_added = false;
		Ui_interaction_state m_interaction_state = Ui_interaction_state::idle;
		Interaction_consume m_interaction_consume_type = Interaction_consume::consume;
		Ui_widget_update m_update_requested = Ui_widget_update::layout_and_appearance;
	};

	struct State_ui : State
	{
		struct Window_info
		{
			glm::vec2 m_size;
			Update_list_id<Render_object_window> m_id;
		};

		friend Ui_widget;
		friend struct System_ui;

		template <typename T_slot_type>
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

			m_dirty_root_widgets.insert(widget->get_outermost_parent());

			return ret_val;
		}

		void destroy(const std::string& in_key)
		{
			auto it = m_widget_lut.find(in_key);

			if (it == m_widget_lut.end())
				return;

			if (it->second != it->second->get_outermost_parent())
				m_dirty_root_widgets.insert(it->second->get_outermost_parent());

			if (it->second == m_focused_widget)
			{
				m_focused_widget->on_focus_end();
				m_focused_widget = nullptr;
			}

			it->second->destroy(*this);

			m_root_to_window_size_lut.erase(in_key);
			m_event_delegate_lut.erase(in_key);
		}

		//finding it with write access causes it to redraw!
		template<typename T_widget_type>
		T_widget_type* find(const std::string& in_key)
		{
			auto it = m_widget_lut.find(in_key);
			if (it == m_widget_lut.end())
				return nullptr;

			m_dirty_root_widgets.insert(it->second->get_outermost_parent());

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
			auto& events_for_key = m_event_delegate_lut[in_widget_key][Type_index<Delegate<>>::get<T_delegate_type>()];

			events_for_key.push_back(*reinterpret_cast<std::function<void()>*>(&in_callback));
		}
		
		template <typename T_delegate_type, typename ...T_args>
		__forceinline void invoke(const std::string& in_key, T_args... in_args)
		{
			auto event_lists_it = m_event_delegate_lut.find(in_key);

			if (event_lists_it == m_event_delegate_lut.end())
				return;

			auto event_list_it = event_lists_it->second.find(Type_index<Delegate<>>::get<T_delegate_type>());

			if (event_list_it == event_lists_it->second.end())
				return;

			this_thread().update_deferred
			(
				[listeners = event_list_it->second, in_args...](Engine_context) mutable
				{
					for (auto&& listener : listeners)
						(*reinterpret_cast<T_delegate_type::template Signature*>(&listener))(in_args...);
				}
			);
		}

		void set_window_info(const std::string& in_root_widget_key, const glm::vec2& in_size, Update_list_id<Render_object_window> in_id)
		{
			auto&& it = m_root_to_window_size_lut[in_root_widget_key];
			it.m_size = in_size;
			it.m_id = in_id;
		}

	private:
		std::vector<std::unique_ptr<Ui_widget>> m_widgets;
		std::vector<size_t> m_free_widget_indices;
		std::unordered_map<std::string, Ui_widget*> m_widget_lut;
		std::unordered_set<Ui_widget*> m_dirty_root_widgets;
		std::unordered_map<std::string, Window_info> m_root_to_window_size_lut;

		std::vector<Ui_widget*> m_widgets_to_redraw;
		Ui_widget* m_focused_widget = nullptr;

		std::unordered_map<std::string, std::unordered_map<i32, std::vector<std::function<void()>>>> m_event_delegate_lut;
		std::vector<Update_list_id<Render_object_ui>> m_ro_ids_to_destroy;
	};

	struct System_ui : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

		static void update_ui(Processor_ui in_processor);
		static void update_ui_interactions(Processor_ui in_processor);
	};

	struct Ui_parent_widget_base : Ui_widget
	{
	};

	struct Ui_slot
	{
	};

	template <typename T_slot_type>
	struct Ui_parent_widget : Ui_parent_widget_base
	{
		friend struct State_ui;

		using Slot_type = T_slot_type;

		Ui_parent_widget& add_child(const Slot_type& in_slot_data, Ui_widget& inout_widget)
		{
			assert(inout_widget.m_correctly_added && "Widget was not created properly, please use Ui_context::create_widget");
			assert(!inout_widget.m_parent && "Widget has already been added to some parent.");
			m_children.emplace_back(in_slot_data, inout_widget);
			inout_widget.m_parent = this;
			inout_widget.m_slot_index = static_cast<i32>(m_children.size() - 1);
			
			m_ui_state->m_dirty_root_widgets.erase(&inout_widget);
			m_ui_state->m_dirty_root_widgets.insert(get_outermost_parent());

			return *this;
		}

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			for (auto&& child : m_children)
				child.second.calculate_render_transform(in_window_size);

			Ui_widget::calculate_render_transform(in_window_size);
		}

		virtual void gather_dependencies(std::vector<Asset_header*>& out_assets) const override
		{
			Ui_widget::gather_dependencies(out_assets);
			for (auto && child : m_children)
				child.second.gather_dependencies(out_assets);
		}

		const Slot_type& get_slot(size_t in_slot_index) const { return m_children[in_slot_index].first; }
		Slot_type& get_slot(size_t in_slot_index) { return m_children[in_slot_index].first; }

		const Ui_widget& get_widget(size_t in_slot_index) const { return m_children[in_slot_index].second; }
		Ui_widget& get_widget(size_t in_slot_index) { return m_children[in_slot_index].second; }

		const std::pair<Slot_type, Ui_widget&>& get_child(size_t in_slot_index) const { return m_children[in_slot_index]; }
		std::pair<Slot_type, Ui_widget&>& get_child(size_t in_slot_index) { return m_children[in_slot_index]; }

		virtual void gather_widgets_over_point(const glm::vec2& in_point, std::vector<Ui_widget*>& out_widgets) override
		{
			//reverse so the widget rendered last gets the interaction first
			for (i32 i = (i32)m_children.size() - 1; i >= 0; i--)
				m_children[i].second.gather_widgets_over_point(in_point, out_widgets);

			Ui_widget::gather_widgets_over_point(in_point, out_widgets);
		}

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Ui_widget::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			in_final_translation; in_final_size; in_final_rotation;
			for (auto&& child : m_children)
				child.second.update_render_scene(child.second.m_render_translation, child.second.m_global_render_size, child.second.m_render_rotation, in_window_size, in_window_id, inout_processor);
		}

		virtual void destroy(State_ui& inout_ui_state) override
		{
			for (auto&& child : m_children)
				child.second.destroy(inout_ui_state);

			Ui_widget::destroy(inout_ui_state);
		}

		size_t get_child_count() const { return m_children.size(); }

		virtual void calculate_content_size(const glm::vec2& in_window_size)
		{
			for (auto&& child : m_children)
				child.second.calculate_content_size(in_window_size);

			Ui_widget::calculate_content_size(in_window_size);
		}

		virtual void calculate_sort_priority(ui32& inout_current_sort_priority)
		{
			m_sort_priority = ++inout_current_sort_priority;

			for (auto&& child : m_children)
				child.second.calculate_sort_priority(inout_current_sort_priority);
		}

	private:
		std::vector<std::pair<Slot_type, Ui_widget&>> m_children;
	};

	struct Ui_slot_canvas : Ui_slot
	{
		Ui_slot_canvas& anchors(const Ui_anchors& in_anchors) { m_anchors = in_anchors; return *this; }
		Ui_slot_canvas& translation(const glm::vec2& in_translation) { m_translation = in_translation; return *this; }
		Ui_slot_canvas& size(const glm::vec2& in_size) { m_size = in_size; return *this; }
		Ui_slot_canvas& pivot(const glm::vec2& in_pivot) { m_pivot = in_pivot; return *this; }
		Ui_slot_canvas& rotation(float in_rotation) { m_rotation = in_rotation; return *this; }
		Ui_slot_canvas& autosize(bool in_autosize) { m_autosize = in_autosize; return *this; }
		Ui_slot_canvas& z_order(ui32 in_z_order) { m_z_order = in_z_order; return *this; }

		Ui_anchors m_anchors;

		glm::vec2 m_translation;
		glm::vec2 m_size;
		glm::vec2 m_pivot;
		float m_rotation = 0;
		bool m_autosize = false;

		ui32 m_z_order = 0;
	};

	struct Ui_widget_canvas : Ui_parent_widget<Ui_slot_canvas>
	{
		Ui_widget_canvas& reference_dimensions(const glm::vec2& in_reference_dimensions) { m_reference_dimensions = in_reference_dimensions; return *this; }

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_widget::calculate_render_transform(in_window_size);

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);

				child.m_render_translation = m_render_translation;
				child.m_render_rotation = m_render_rotation;

				glm::vec2 anchor_vec = slot.m_anchors.get_max() - slot.m_anchors.get_min();
				glm::vec2 anchor_scale = anchor_vec;

				//if both anchors are 0 on all, translation should be relative to top left
				//if both anchors are 1 on all, translation should be relative to bottom right

				//if both anchors are not the same, translation should be relative to center of both
				glm::vec2 translation = slot.m_translation / m_reference_dimensions;
				translation += slot.m_anchors.get_min() + (anchor_vec * 0.5f);


				glm::vec2 slot_size = slot.m_size;

				if (slot.m_anchors.get_min().x < slot.m_anchors.get_max().x)
					slot_size.x = (slot.m_anchors.get_max().x - slot.m_anchors.get_min().x) * in_window_size.x;

				if (slot.m_anchors.get_min().y < slot.m_anchors.get_max().y)
					slot_size.y = (slot.m_anchors.get_max().y - slot.m_anchors.get_min().y) * in_window_size.y;

				const glm::vec2 render_size = slot.m_autosize ? child.get_content_size(in_window_size) : slot_size / in_window_size;

				child.m_global_render_size = render_size;
				child.m_render_translation += translation;
				child.m_render_translation -= child.m_global_render_size * slot.m_pivot;
				child.m_render_rotation += slot.m_rotation;

				child.calculate_render_transform(in_window_size);
			}
		}

		glm::vec2 m_reference_dimensions;
	};

	struct Ui_slot_vertical_box : Ui_slot
	{
		Ui_slot_vertical_box& padding(const Ui_padding& in_padding) { m_padding = in_padding; return *this; }
		Ui_slot_vertical_box& h_align(Ui_h_alignment in_horizontal_alignment) { m_horizontal_alignment = in_horizontal_alignment; return *this; }

		Ui_padding m_padding;
		Ui_h_alignment m_horizontal_alignment = Ui_h_alignment::fill;
	};

	struct Ui_widget_vertical_box : Ui_parent_widget<Ui_slot_vertical_box>
	{
		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_widget::calculate_render_transform(in_window_size);

			float prev_y = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
			{
				const i32 prev_slot_index = static_cast<i32>(i) - 1;

				auto&& [slot, child] = get_child(i);

				child.m_render_translation = m_render_translation;
				child.m_render_rotation = m_render_rotation;

				child.m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;

				child.m_render_translation.y += prev_y;
				child.m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

				if (prev_slot_index >= 0)
				{
					const Ui_slot_vertical_box& prev_slot = get_slot(prev_slot_index);
					child.m_render_translation.y += prev_slot.m_padding.get_bottom() / in_window_size.y;
				}

				const float new_render_x = m_global_render_size.x - ((slot.m_padding.get_right() + slot.m_padding.get_left()) / in_window_size.x);

				switch (slot.m_horizontal_alignment)
				{
				case Ui_h_alignment::fill:
					child.m_global_render_size.x = new_render_x;
					break;
				case Ui_h_alignment::left:
					child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
					break;
				case Ui_h_alignment::right:
					child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
					child.m_render_translation.x += new_render_x - child.m_global_render_size.x;
					break;
				case Ui_h_alignment::center:
					child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
					child.m_render_translation.x += (new_render_x - child.m_global_render_size.x) * 0.5f;
					break;
				default:
					break;
				}

				child.calculate_render_transform(in_window_size);

				prev_y = child.m_render_translation.y - m_render_translation.y + child.m_global_render_size.y;
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);
				size.y += child.m_global_render_size.y;
				size.y += (slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y;

				const float child_content_x = child.m_global_render_size.x - ((slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x);

				size.x = glm::max(size.x, child_content_x);
			}

			return size;
		};
	};

	struct Ui_slot_horizontal_box : Ui_slot
	{
		Ui_slot_horizontal_box& padding(const Ui_padding& in_padding) { m_padding = in_padding; return *this; }
		Ui_slot_horizontal_box& v_align(Ui_v_alignment in_alignment) { m_alignment = in_alignment; return *this; }

		Ui_padding m_padding;
		Ui_v_alignment m_alignment = Ui_v_alignment::fill;
	};

	struct Ui_widget_horizontal_box : Ui_parent_widget<Ui_slot_horizontal_box>
	{
		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_widget::calculate_render_transform(in_window_size);

			float prev_x = 0.0f;

			for (size_t i = 0; i < get_child_count(); i++)
			{
				const i32 prev_slot_index = static_cast<i32>(i) - 1;

				auto&& [slot, child] = get_child(i);

				child.m_render_translation = m_render_translation;
				child.m_render_rotation = m_render_rotation;

				child.m_render_translation.x += prev_x;
				child.m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;

				child.m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

				if (prev_slot_index >= 0)
				{
					const Ui_slot_horizontal_box& prev_slot = get_slot(prev_slot_index);
					child.m_render_translation.x += prev_slot.m_padding.get_left() / in_window_size.y;
				}

				const float new_render_y = m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

				switch (slot.m_alignment)
				{
				case Ui_v_alignment::fill:
					child.m_global_render_size.y = new_render_y;
					break;
				case Ui_v_alignment::top:
					child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
					break;
				case Ui_v_alignment::bottom:
					child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
					child.m_render_translation.y += new_render_y - child.m_global_render_size.y;
					break;
				case Ui_v_alignment::center:
					child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
					child.m_render_translation.y += (new_render_y - child.m_global_render_size.y) * 0.5f;
					break;
				default:
					break;
				}

				child.calculate_render_transform(in_window_size);

				prev_x = child.m_render_translation.x - m_render_translation.x + child.m_global_render_size.x + (slot.m_padding.get_right() / in_window_size.x);
			}
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			glm::vec2 size = { 0.0f, 0.0f };

			for (size_t i = 0; i < get_child_count(); i++)
			{
				auto&& [slot, child] = get_child(i);
				size.x += child.m_global_render_size.x;
				size.x += (slot.m_padding.get_left() + slot.m_padding.get_right()) / in_window_size.x;

				const float child_content_y = child.m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

				size.y = glm::max(size.y, child_content_y);
			}

			return size;
		};
	};

	struct Ui_widget_image : Ui_widget
	{
		Ui_widget_image& material(const Asset_ref<Asset_material>& in_asset_ref) { m_material = in_asset_ref; request_update(Ui_widget_update::appearance); return *this; }
		Ui_widget_image& image_size(const glm::vec2& in_size) { m_image_size = in_size; request_update(Ui_widget_update::layout_and_appearance); return *this; }
		Ui_widget_image& tint(const glm::vec4& in_tint) { m_tint = in_tint; request_update(Ui_widget_update::appearance); return *this; }

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override { return m_image_size / in_window_size; };

		Asset_ref<Asset_material> m_material;
		glm::vec2 m_image_size = { 64.0f, 64.0f };
		glm::vec4 m_tint = { 1.0f, 1.0f, 1.0f, 1.0f };

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			Ui_widget::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			if (!m_material.is_valid())
				return;

			auto update_lambda =
			[in_final_translation, in_final_size, in_final_rotation, mat = m_material, id = in_window_id, in_window_size, tint = m_tint, sort_priority = get_sort_priority()](Render_object_ui& inout_object)
			{
				inout_object.m_window_id = id;
				inout_object.m_sort_priority = sort_priority;

				if (inout_object.m_material != mat.get_mutable())
				{
					if (inout_object.m_material)
						inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index);

					inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
					inout_object.m_material = mat.get_mutable();
				}

				struct Local
				{
					static glm::vec2 round_to_pixel_density(const glm::vec2& in_vec, const glm::vec2& in_pixel_density)
					{
						glm::vec2 ret_val = { in_vec.x * in_pixel_density.x, in_vec.y * in_pixel_density.y };
						ret_val = glm::round(ret_val);
						ret_val.x /= in_pixel_density.x;
						ret_val.y /= in_pixel_density.y;

						return ret_val;
					}
				};

				const glm::vec2 top_left = Local::round_to_pixel_density(in_final_translation, in_window_size);
				const glm::vec2 bottom_right = Local::round_to_pixel_density(in_final_translation + in_final_size, in_window_size);

				const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

				mat.get_mutable()->set_parameter_on_instance("lefttop_rightbottom_packed", lefttop_rightbottom_packed, inout_object.m_instance_data_index);
				mat.get_mutable()->set_parameter_on_instance("tint", tint, inout_object.m_instance_data_index);
			};

			inout_processor.update_state_deferred<State_render_scene>
			(
				[this, update_lambda](State_render_scene& inout_state)
				{
					if (m_ro_id.is_valid())
						inout_state.m_ui_elements.update_object(m_ro_id, update_lambda);
					else
						m_ro_id = inout_state.m_ui_elements.create_object(update_lambda);

				}
			);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		void destroy(State_ui& inout_ui_state) override
		{
			destroy_render_object(inout_ui_state, m_ro_id);
			Ui_widget::destroy(inout_ui_state);
		}

		Update_list_id<Render_object_ui> m_ro_id;
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

	struct Ui_widget_button : Ui_parent_widget<Ui_slot_button>
	{
		struct On_toggled : Delegate<bool> {};

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


		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override { return m_size / in_window_size; };

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			m_image.m_image_size = m_size;
			
			switch (get_interaction_state())
			{
			case sic::Ui_interaction_state::idle:
				m_image.m_material = m_idle_material;
				m_image.m_tint = m_tint_idle;
				break;
			case sic::Ui_interaction_state::hovered:
				m_image.m_material = m_hovered_material;
				m_image.m_tint = m_tint_hovered;
				break;
			case sic::Ui_interaction_state::pressed:
				m_image.m_material = m_pressed_material;
				m_image.m_tint = m_tint_pressed;
				break;
			case sic::Ui_interaction_state::pressed_not_hovered:
				m_image.m_material = m_hovered_material;
				m_image.m_tint = m_tint_hovered;
				break;
			default:
				break;
			}

			m_image.set_sort_priority(get_sort_priority());
			m_image.update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			Ui_parent_widget<Ui_slot_button>::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);
		}

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			Ui_widget::calculate_render_transform(in_window_size);

			if (get_child_count() == 0)
				return;

			auto&& [slot, child] = get_child(0);

			child.m_render_translation = m_render_translation;
			child.m_render_rotation = m_render_rotation;

			child.m_render_translation.x += slot.m_padding.get_left() / in_window_size.x;
			child.m_render_translation.y += slot.m_padding.get_top() / in_window_size.y;

			const float new_render_x = m_global_render_size.x - ((slot.m_padding.get_right() + slot.m_padding.get_left()) / in_window_size.x);

			switch (slot.m_horizontal_alignment)
			{
			case Ui_h_alignment::fill:
				child.m_global_render_size.x = new_render_x;
				break;
			case Ui_h_alignment::left:
				child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
				break;
			case Ui_h_alignment::right:
				child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
				child.m_render_translation.x += new_render_x - child.m_global_render_size.x;
				break;
			case Ui_h_alignment::center:
				child.m_global_render_size.x = glm::min(child.m_global_render_size.x, new_render_x);
				child.m_render_translation.x += (new_render_x - child.m_global_render_size.x) * 0.5f;
				break;
			default:
				break;
			}

			const float new_render_y = m_global_render_size.y - ((slot.m_padding.get_top() + slot.m_padding.get_bottom()) / in_window_size.y);

			switch (slot.m_vertical_alignment)
			{
			case Ui_v_alignment::fill:
				child.m_global_render_size.y = new_render_y;
				break;
			case Ui_v_alignment::top:
				child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
				break;
			case Ui_v_alignment::bottom:
				child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
				child.m_render_translation.y += new_render_y - child.m_global_render_size.y;
				break;
			case Ui_v_alignment::center:
				child.m_global_render_size.y = glm::min(child.m_global_render_size.y, new_render_y);
				child.m_render_translation.y += (new_render_y - child.m_global_render_size.y) * 0.5f;
				break;
			default:
				break;
			}

			child.calculate_render_transform(in_window_size);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override
		{
			Ui_parent_widget<Ui_slot_button>::gather_dependencies(out_assets);

			out_assets.push_back(m_idle_material.get_header());
			out_assets.push_back(m_hovered_material.get_header());
			out_assets.push_back(m_pressed_material.get_header());
		}

		virtual void on_interaction_state_changed() { request_update(Ui_widget_update::appearance); }

		virtual void on_clicked(Mousebutton in_button, const glm::vec2& in_cursor_pos) override
		{
			Ui_widget::on_clicked(in_button, in_cursor_pos);

			if (m_is_toggle)
			{
				m_toggled = !m_toggled;
				invoke<On_toggled>(m_toggled);
			}
		}

		void destroy(State_ui& inout_ui_state) override
		{
			m_image.destroy_render_object(inout_ui_state, m_image.m_ro_id);

			Ui_parent_widget<Ui_slot_button>::destroy(inout_ui_state);
		}

		glm::vec2 m_size = { 64.0f, 64.0f };

		Asset_ref<Asset_material> m_idle_material;
		Asset_ref<Asset_material> m_hovered_material;
		Asset_ref<Asset_material> m_pressed_material;

		glm::vec4 m_tint_idle = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_hovered = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_pressed = { 1.0f, 1.0f, 1.0f, 1.0f };

		Ui_widget_image m_image;
		bool m_toggled = false;
		bool m_is_toggle = false;
	};

	struct Ui_widget_text : Ui_widget
	{
		Ui_widget_text& text(const std::string in_text) { m_text = in_text; request_update(Ui_widget_update::layout_and_appearance); return *this; }
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

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			const Asset_font* font = m_font.get();

			if (!font)
				return glm::vec2(0.0f, 0.0f);

			glm::vec2 size = { 0.0f, 0.0f };

			if (!m_text.empty())
				size.y = font->m_max_glyph_height;

			float cur_size_x = 0.0f;

			for (size_t i = 0; i < m_text.size(); i++)
			{
				const auto c = m_text[i];

				if (c == '\n')
				{
					size.y += font->m_max_glyph_height;
					size.x = glm::max(size.x, cur_size_x);

					cur_size_x = 0.0f;
					continue;
				}

				cur_size_x += font->m_glyphs[c].m_pixel_advance;

				if (i > 0)
					cur_size_x += font->m_glyphs[c].m_kerning_values[m_text[i - 1]];
			}

			size.x = glm::max(size.x, cur_size_x);
			size.y = font->m_max_glyph_height;

			const float scale_ratio = m_px / font->m_em_size;
			size *= scale_ratio;

			size /= in_window_size;

			return size;
		};

		virtual void on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement) override
		{
			Ui_widget::on_cursor_move_over(in_cursor_pos, in_cursor_movement);

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
			Ui_widget::on_pressed(in_button, in_cursor_pos);
			
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
			Ui_widget::on_released(in_button, in_cursor_pos);

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
			Ui_widget::on_focus_end();

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
			Ui_widget::update_render_scene(in_final_translation, in_final_size, in_final_rotation, in_window_size, in_window_id, inout_processor);

			const Asset_font* font = m_font.get();

			if (!font)
				return;

			const float scale_ratio = m_px / font->m_em_size;

			std::vector<Update_list_id<Render_object_ui>> ro_ids_to_destroy;

			for (size_t i = m_text.size(); i < m_glyph_instances.size(); i++)
			{
				ro_ids_to_destroy.push_back(m_glyph_instances[i].m_ro_id);
				m_glyph_instances[i].m_ro_id.reset();
			}

			m_glyph_instances.resize(m_text.size());

			generate_lines(in_final_size.x * in_window_size.x);

			if (m_lines.empty())
			{
				for (auto& glyph_instance : m_glyph_instances)
				{
					if (!glyph_instance.m_ro_id.is_valid())
						continue;

					ro_ids_to_destroy.push_back(glyph_instance.m_ro_id);
					glyph_instance.m_ro_id.reset();
				}

				return;
			}

			for (size_t i = m_lines.size(); i < m_selection_box_images.size(); i++)
				if (m_selection_box_images[i].m_ro_id.is_valid())
					ro_ids_to_destroy.push_back(m_selection_box_images[i].m_ro_id);
			
			m_selection_box_images.resize(m_lines.size());

			for (size_t line_idx = 0; line_idx < m_lines.size(); ++line_idx)
			{
				const Text_line& line = m_lines[line_idx];
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
						const float width_overflow = (glyph_instance.m_bottom_right.x - (top_left_start.x - render_translation.x)) - (in_final_translation.x + in_final_size.x);
						//cull only if it overflows by half the character or more
						if (width_overflow >= in_final_size.x * 0.5f)
						{
							if (glyph_instance.m_ro_id.is_valid())
								ro_ids_to_destroy.push_back(glyph_instance.m_ro_id);
							glyph_instance.m_ro_id.reset();
							continue;
						}
						else if (top_left_start.x < in_final_translation.x)
						{
							if (glyph_instance.m_ro_id.is_valid())
								ro_ids_to_destroy.push_back(glyph_instance.m_ro_id);
							glyph_instance.m_ro_id.reset();
							continue;
						}
					}

					glm::vec4 fg_color = m_foreground_color;
					glm::vec4 bg_color = m_background_color;

					glyph_instance.m_is_selected = false;
					if (m_selectable && m_selection_start.has_value() && m_selection_end.has_value())
					{
						const glm::vec2 selection_top_left = { glm::min(m_selection_start.value().x, m_selection_end.value().x), glm::min(m_selection_start.value().y, m_selection_end.value().y) };
						const glm::vec2 selection_bottom_right = { glm::max(m_selection_start.value().x, m_selection_end.value().x), glm::max(m_selection_start.value().y, m_selection_end.value().y) };

						if (center.x >= selection_top_left.x && center.x <= selection_bottom_right.x)
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

					auto update_lambda =
						[font_ref = m_font, top_left = glyph_instance.m_top_left, bottom_right = glyph_instance.m_bottom_right, in_final_rotation, mat = m_material, id = in_window_id, in_window_size, atlas_offset, atlas_glyph_size, sort_priority = get_sort_priority(), fg_color, bg_color](Render_object_ui& inout_object)
					{
						inout_object.m_window_id = id;
						inout_object.m_sort_priority = sort_priority;

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

					inout_processor.update_state_deferred<State_render_scene>
					(
						[&ro_id = glyph_instance.m_ro_id, update_lambda](State_render_scene& inout_state)
						{
							if (ro_id.is_valid())
								inout_state.m_ui_elements.update_object(ro_id, update_lambda);
							else
								ro_id = inout_state.m_ui_elements.create_object(update_lambda);
						}
					);

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
						selection_box_image.m_material = m_selection_box_material;
						selection_box_image.m_image_size = m_selection_box_image_size;
						selection_box_image.m_tint = m_selection_box_tint;

						selection_box_image.set_sort_priority(get_sort_priority() - 1);
						const float x_min = selection_start.value().x;
						const float x_max = selection_end.value().x;
						const float y_min = in_final_translation.y;
						const float y_max = in_final_translation.y + (((font->m_max_glyph_height + line.m_y) * scale_ratio) / in_window_size.y);
						selection_box_image.update_render_scene({ x_min, y_min }, { x_max - x_min, y_max - y_min }, 0.0f, in_window_size, in_window_id, inout_processor);
					}
					else
					{
						selection_box_image.m_tint = { 0.0f, 0.0f, 0.0f, 0.0f };
						selection_box_image.set_sort_priority(get_sort_priority() - 1);
						selection_box_image.update_render_scene(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 0.0f, in_window_size, in_window_id, inout_processor);
					}
				}
				else
				{
					selection_box_image.m_tint = { 0.0f, 0.0f, 0.0f, 0.0f };
					selection_box_image.set_sort_priority(get_sort_priority() - 1);
					selection_box_image.update_render_scene(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 0.0f, in_window_size, in_window_id, inout_processor);
				}
			}

			inout_processor.update_state_deferred<State_render_scene>
			(
				[ro_ids_to_destroy](State_render_scene& inout_state)
				{
					for (auto&& ro_id : ro_ids_to_destroy)
						inout_state.m_ui_elements.destroy_object(ro_id);
				}
			);
		}

		void destroy(State_ui& inout_ui_state) override
		{
			for (auto&& glyph_instance : m_glyph_instances)
				destroy_render_object(inout_ui_state, glyph_instance.m_ro_id);

			for (auto& image : m_selection_box_images)
				image.destroy_render_object(inout_ui_state, image.m_ro_id);

			m_glyph_instances.clear();
			m_selection_box_images.clear();

			Ui_widget::destroy(inout_ui_state);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		struct Glyph
		{
			Update_list_id<Render_object_ui> m_ro_id;
			glm::vec2 m_top_left;
			glm::vec2 m_bottom_right;
			bool m_is_selected = false;
		};

		std::vector<Glyph> m_glyph_instances;
		std::vector<Ui_widget_image> m_selection_box_images;

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
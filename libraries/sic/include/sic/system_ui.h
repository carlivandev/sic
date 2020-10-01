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

	struct Ui_widget : Noncopyable
	{
		struct On_pressed : Delegate<> {};
		struct On_released : Delegate<> {};
		struct On_clicked : Delegate<> {};
		struct On_hover_begin : Delegate<> {};
		struct On_hover_end : Delegate<> {};
		struct On_cursor_move_over : Delegate<> {};
		struct On_drag : Delegate<const glm::vec2&, const glm::vec2&> {};

		friend struct State_ui;
		friend struct System_ui;

		template <typename T_slot_type>
		friend struct Ui_parent_widget;

		Ui_widget& interaction_consume(Interaction_consume in_interaction_consume_type) { m_interaction_consume_type = in_interaction_consume_type; return *this; }

		virtual void calculate_render_transform(const glm::vec2& in_window_size);
		virtual void calculate_content_size(const glm::vec2& in_window_size) { m_global_render_size = get_content_size(in_window_size); }

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
			if (m_key.has_value())
				m_ui_state->invoke<T_delegate_type>(m_key.value(), in_args...);
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
		{ in_final_translation; in_final_size; in_final_rotation; in_window_size; in_window_id; inout_processor; }

		void request_redraw() { m_redraw_requested = true; }

		virtual void destroy(State_ui& inout_ui_state);

		Ui_widget() = default;

	private:
		std::optional<std::string> m_key;
		Ui_parent_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		ui32 m_sort_priority = 0;
		On_asset_loaded::Handle m_dependencies_loaded_handle;
		State_ui* m_ui_state = nullptr;

		bool m_correctly_added = false;
		Ui_interaction_state m_interaction_state = Ui_interaction_state::idle;
		Interaction_consume m_interaction_consume_type = Interaction_consume::consume;
		bool m_redraw_requested = false;
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
			if (!in_key.has_value())
				in_key = xg::newGuid().str();

			assert(m_widget_lut.find(in_key.value()) == m_widget_lut.end() && "Can not have two widgets with same key!");

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

			auto& widget = m_widget_lut[in_key.value()] = m_widgets.back().get();
			widget->m_key = in_key.value();
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

		std::unordered_map<std::string, std::unordered_map<i32, std::vector<std::function<void()>>>> m_event_delegate_lut;

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

			ui32 steps_until_no_parent = 0;

			const Ui_parent_widget_base* parent = this;
			while (parent)
			{
				++steps_until_no_parent;
				parent = parent->get_parent();
			}

			inout_widget.m_sort_priority = 0;
			inout_widget.m_sort_priority = steps_until_no_parent << 16;
			inout_widget.m_sort_priority += static_cast<ui32>(inout_widget.m_slot_index);
			
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
			for (i32 i = m_children.size() - 1; i >= 0; i--)
				m_children[i].second.gather_widgets_over_point(in_point, out_widgets);

			Ui_widget::gather_widgets_over_point(in_point, out_widgets);
		}

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			in_final_translation; in_final_size; in_final_rotation;
			for (auto&& child : m_children)
				child.second.update_render_scene(child.second.m_render_translation, child.second.m_global_render_size, child.second.m_render_rotation, in_window_size, in_window_id, inout_processor);
		}

		virtual void destroy(State_ui& inout_ui_state) override final
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
		Ui_widget_image& material(const Asset_ref<Asset_material>& in_asset_ref) { m_material = in_asset_ref; return *this; }
		Ui_widget_image& image_size(const glm::vec2& in_size) { m_image_size = in_size; return *this; }
		Ui_widget_image& tint(const glm::vec4& in_tint) { m_tint = in_tint; return *this; }

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override { return m_image_size / in_window_size; };

		Asset_ref<Asset_material> m_material;
		glm::vec2 m_image_size = { 64.0f, 64.0f };
		glm::vec4 m_tint = { 1.0f, 1.0f, 1.0f, 1.0f };

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			if (!m_material.is_valid())
				return;

			auto update_lambda =
			[in_final_translation, in_final_size, in_final_rotation, mat = m_material, id = in_window_id, in_window_size, tint = m_tint, sort_priority = get_sort_priority()](Render_object_ui& inout_object)
			{
				inout_object.m_window_id = id;

				if (inout_object.m_material != mat.get_mutable())
				{
					if (inout_object.m_material)
						inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index);

					inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
					inout_object.m_material = mat.get_mutable();
					inout_object.m_sort_priority = sort_priority;
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
		//assigns all materials that have not yet been assigned
		Ui_widget_button& base_material(const Asset_ref<Asset_material>& in_asset_ref)
		{
			if (!m_idle_material.is_valid())
				m_idle_material = in_asset_ref;

			if (!m_hovered_material.is_valid())
				m_hovered_material = in_asset_ref;

			if (!m_pressed_material.is_valid())
				m_pressed_material = in_asset_ref;

			return *this;
		}

		Ui_widget_button& idle_material(const Asset_ref<Asset_material>& in_asset_ref) { m_idle_material = in_asset_ref; return *this; }
		Ui_widget_button& hovered_material(const Asset_ref<Asset_material>& in_asset_ref) { m_hovered_material = in_asset_ref; return *this; }
		Ui_widget_button& pressed_material(const Asset_ref<Asset_material>& in_asset_ref) { m_pressed_material = in_asset_ref; return *this; }

		Ui_widget_button& tints(const glm::vec4& in_tint_idle, const glm::vec4& in_tint_hovered, const glm::vec4& in_tint_pressed)
		{
			m_tint_idle = in_tint_idle;
			m_tint_hovered = in_tint_hovered;
			m_tint_pressed = in_tint_pressed;

			return *this;
		}

		Ui_widget_button& size(const glm::vec2& in_size) { m_size = in_size; return *this; }

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

		virtual void on_interaction_state_changed() { request_redraw(); }

		glm::vec2 m_size = { 64.0f, 64.0f };

		Asset_ref<Asset_material> m_idle_material;
		Asset_ref<Asset_material> m_hovered_material;
		Asset_ref<Asset_material> m_pressed_material;

		glm::vec4 m_tint_idle = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_hovered = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 m_tint_pressed = { 1.0f, 1.0f, 1.0f, 1.0f };

		Ui_widget_image m_image;
	};

	struct Ui_widget_text : Ui_widget
	{
		Ui_widget_text& text(const std::string in_text) { m_text = in_text; return *this; }
		Ui_widget_text& font(const Asset_ref<Asset_font>& in_asset_ref) { m_font = in_asset_ref; return *this; }
		Ui_widget_text& material(const Asset_ref<Asset_material>& in_asset_ref) { m_material = in_asset_ref; return *this; }
		Ui_widget_text& px(float in_px) { m_px = in_px; return *this; }

		Ui_widget_text& line_height_percentage(float in_line_height_percentage) { m_line_height_percentage = in_line_height_percentage; return *this; }
		Ui_widget_text& align(Ui_h_alignment in_alignment) { m_alignment = in_alignment; return *this; }
		Ui_widget_text& autowrap(bool in_autowrap) { m_autowrap = in_autowrap; return *this; }
		Ui_widget_text& wrap_text_at(std::optional<float> in_wrap_text_at)
		{
			m_wrap_text_at = in_wrap_text_at; return *this;
		}

		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override
		{
			const Asset_font* font = m_font.get();

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

			size.y = font->m_max_glyph_height;

			const float scale_ratio = m_px / font->m_em_size;
			size *= scale_ratio;

			size /= in_window_size;

			return size;
		};

		Asset_ref<Asset_material> m_material;
		Asset_ref<Asset_font> m_font;
		std::string m_text;
		Ui_h_alignment m_alignment = Ui_h_alignment::left;
		float m_px = 16.0f;
		float m_line_height_percentage = 1.0f;
		std::optional<float> m_wrap_text_at;
		bool m_autowrap = false;

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id, Processor_ui& inout_processor) override
		{
			const Asset_font* font = m_font.get();
			const float scale_ratio = m_px / font->m_em_size;

			std::vector<Update_list_id<Render_object_ui>> ro_ids_to_destroy;

			for (size_t i = m_text.size(); i < m_ro_ids.size(); i++)
			{
				ro_ids_to_destroy.push_back(m_ro_ids[i]);
				m_ro_ids[i].reset();
			}

			m_ro_ids.resize(m_text.size());

			generate_lines(in_final_size.x * in_window_size.x);

			if (m_lines.empty())
			{
				for (auto& m_ro_id : m_ro_ids)
				{
					if (!m_ro_id.is_valid())
						continue;

					ro_ids_to_destroy.push_back(m_ro_id);
					m_ro_id.reset();
				}

				return;
			}

			for (const Text_line& line : m_lines)
			{
				float cur_advance = 0.0f;

				if (m_alignment == Ui_h_alignment::right)
					cur_advance = (in_final_size.x * in_window_size.x) - line.m_total_width;
				else if (m_alignment == Ui_h_alignment::center)
					cur_advance = (in_final_size.x * in_window_size.x * 0.5f) - (line.m_total_width * 0.5f);

				for (size_t i = line.m_start_index; i <= line.m_end_index; i++)
				{
					auto c = m_text[i];
					auto& ro_id = m_ro_ids[i];
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

					const glm::vec2& top_left = render_translation;
					const glm::vec2 bottom_right = render_translation + render_size;

					//only cull edges if we are not auto-wrapping
					if (!m_autowrap)
					{
						if (bottom_right.x - (top_left_start.x - render_translation.x) > in_final_translation.x + in_final_size.x)
						{
							if (ro_id.is_valid())
								ro_ids_to_destroy.push_back(ro_id);
							ro_id.reset();
							continue;
						}
						else if (top_left_start.x < in_final_translation.x)
						{
							if (ro_id.is_valid())
								ro_ids_to_destroy.push_back(ro_id);
							ro_id.reset();
							continue;
						}
					}

					auto update_lambda =
						[font_ref = m_font, top_left, bottom_right, in_final_rotation, mat = m_material, id = in_window_id, in_window_size, atlas_offset, atlas_glyph_size, sort_priority = get_sort_priority()](Render_object_ui& inout_object)
					{
						inout_object.m_window_id = id;

						if (inout_object.m_material != mat.get_mutable())
						{
							if (inout_object.m_material)
								inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index);

							inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
							inout_object.m_material = mat.get_mutable();
							inout_object.m_sort_priority = sort_priority;
						}

						const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

						mat.get_mutable()->set_parameter_on_instance("lefttop_rightbottom_packed", lefttop_rightbottom_packed, inout_object.m_instance_data_index);
						mat.get_mutable()->set_parameter_on_instance("msdf", font_ref.as<Asset_texture_base>(), inout_object.m_instance_data_index);
						mat.get_mutable()->set_parameter_on_instance("offset_and_size", glm::vec4(atlas_offset, atlas_glyph_size), inout_object.m_instance_data_index);
					};

					inout_processor.update_state_deferred<State_render_scene>
					(
						[&ro_id, update_lambda](State_render_scene& inout_state)
						{
							if (ro_id.is_valid())
								inout_state.m_ui_elements.update_object(ro_id, update_lambda);
							else
								ro_id = inout_state.m_ui_elements.create_object(update_lambda);
						}
					);
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

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		std::vector<Update_list_id<Render_object_ui>> m_ro_ids;

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
	};

}
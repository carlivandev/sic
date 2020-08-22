#pragma once
#include "sic/system.h"
#include "sic/state_render_scene.h"
#include "sic/system_window.h"

#include "glm/vec2.hpp"

#include <optional>

namespace sic
{
	struct Ui_anchors
	{
		Ui_anchors() = default;

		Ui_anchors(float in_uniform_anchors)
		{
			set(in_uniform_anchors);
		}

		Ui_anchors(float in_horizontal_anchors, float in_vertical_anchors)
		{
			set(in_horizontal_anchors, in_horizontal_anchors);
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

	struct Ui_widget : Noncopyable
	{
		friend struct Ui_context;
		friend struct System_ui;

		template <typename T_slot_type>
		friend struct Ui_parent_widget;

		virtual void calculate_render_transform(const glm::vec2& in_window_size);
		virtual void calculate_content_size(const glm::vec2& in_window_size) { m_global_render_size = get_content_size(in_window_size); }

		virtual glm::vec2 get_content_size(const glm::vec2& in_window_size) const { in_window_size; return glm::vec2(0.0f, 0.0f); }

		Ui_widget* get_outermost_parent();
		const Ui_parent_widget_base* get_parent() const { return m_parent; }

		ui32 get_slot_index() const { return m_slot_index; }

		virtual void gather_dependencies(std::vector<Asset_header*>& out_assets) const { out_assets; }

		void get_dependencies_not_loaded(std::vector<Asset_header*>& out_assets) const;

		virtual bool get_ready_to_be_shown() const;

		Update_list_id<Render_object_window> m_window_id;

		glm::vec2 m_local_scale = { 1.0f, 1.0f };
		glm::vec2 m_local_translation = { 0.0f, 0.0f };
		float m_local_rotation = 0.0f;

		i32 m_widget_index = -1;
		bool m_allow_dependency_streaming = false;

		glm::vec2 m_render_translation;
		glm::vec2 m_global_render_size;
		float m_render_rotation;

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) { in_final_translation; in_final_size; in_final_rotation; inout_render_scene_state; }

		virtual void destroy(State_ui& inout_ui_state);

		Ui_widget() = default;

	private:
		std::optional<std::string> m_key;
		Ui_parent_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		On_asset_loaded::Handle m_dependencies_loaded_handle;

		bool m_correctly_added = false;
	};

	struct State_ui : State
	{
		friend Ui_widget;
		friend struct Ui_context;
		friend struct System_ui;

		void update(std::function<void(Ui_context&)> in_update_func)
		{
			std::scoped_lock lock(m_update_lock);
			m_updates.push_back(in_update_func);
		}

	private:
		std::mutex m_update_lock;
		std::vector<std::function<void(Ui_context&)>> m_updates;
		std::vector<std::unique_ptr<Ui_widget>> m_widgets;
		std::vector<size_t> m_free_widget_indices;
		std::unordered_map<std::string, Ui_widget*> m_widget_lut;
		std::unordered_set<Ui_widget*> m_dirty_parent_widgets;
		std::unordered_map<std::string, glm::vec2> m_root_to_window_size_lut;

	};

	struct System_ui : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
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
		friend struct Ui_context;

		using Slot_type = T_slot_type;

		Ui_parent_widget& add_child(const Slot_type& in_slot_data, Ui_widget& inout_widget)
		{
			assert(inout_widget.m_correctly_added && "Widget was not created properly, please use Ui_context::create_widget");
			assert(!inout_widget.m_parent && "Widget has already been added to some parent.");
			m_children.emplace_back(in_slot_data, inout_widget);
			inout_widget.m_parent = this;
			inout_widget.m_slot_index = static_cast<i32>(m_children.size() - 1);
			inout_widget.m_window_id = m_window_id;

			return *this;
		}

		void calculate_render_transform(const glm::vec2& in_window_size) override
		{
			for (auto&& child : m_children)
				child.second.calculate_render_transform(in_window_size);

			Ui_widget::calculate_render_transform(in_window_size);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final
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

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) override
		{
			Ui_widget::update_render_scene(in_final_translation, in_final_size, in_final_rotation, inout_render_scene_state);
			for (auto && child : m_children)
				child.second.update_render_scene(child.second.m_render_translation, child.second.m_global_render_size, child.second.m_render_rotation, inout_render_scene_state);
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

	struct Ui_context
	{
		Ui_context(State_ui& inout_ui_state) : m_ui_state(inout_ui_state) {}

		template<typename T_widget_type>
		T_widget_type& create_widget(std::optional<std::string> in_key)
		{
			if (!in_key.has_value())
				in_key = xg::newGuid().str();

			assert(m_ui_state.m_widget_lut.find(in_key.value()) == m_ui_state.m_widget_lut.end() && "Can not have two widgets with same key!");

			size_t new_idx;

			if (m_ui_state.m_free_widget_indices.empty())
			{
				new_idx = m_ui_state.m_widgets.size();
				m_ui_state.m_widgets.push_back(std::make_unique<T_widget_type>());
			}
			else
			{
				new_idx = m_ui_state.m_free_widget_indices.back();
				m_ui_state.m_free_widget_indices.pop_back();
				m_ui_state.m_widgets[new_idx] = std::make_unique<T_widget_type>();
			}

			auto& widget = m_ui_state.m_widget_lut[in_key.value()] = m_ui_state.m_widgets.back().get();
			widget->m_key = in_key.value();
			widget->m_widget_index = static_cast<i32>(new_idx);
			widget->m_correctly_added = true;

			m_ui_state.m_dirty_parent_widgets.insert(widget->get_outermost_parent());

			return *reinterpret_cast<T_widget_type*>(widget);
		}

		void destroy_widget(const std::string& in_key)
		{
			auto it = m_ui_state.m_widget_lut.find(in_key);

			if (it == m_ui_state.m_widget_lut.end())
				return;

			if (it->second != it->second->get_outermost_parent())
				m_ui_state.m_dirty_parent_widgets.insert(it->second->get_outermost_parent());

			it->second->destroy(m_ui_state);

			m_ui_state.m_root_to_window_size_lut.erase(in_key);
		}

		//finding it with write access causes it to redraw!
		template<typename T_widget_type>
		T_widget_type* find_widget(const std::string& in_key)
		{
			auto it = m_ui_state.m_widget_lut.find(in_key);
			if (it == m_ui_state.m_widget_lut.end())
				return nullptr;

			m_ui_state.m_dirty_parent_widgets.insert(it->second->get_outermost_parent());

			return reinterpret_cast<T_widget_type*>(it->second);
		}

		template<typename T_widget_type>
		const T_widget_type* find_widget(const std::string& in_key) const
		{
			auto it = m_ui_state.m_widget_lut.find(in_key);
			if (it == m_ui_state.m_widget_lut.end())
				return nullptr;

			return reinterpret_cast<T_widget_type*>(it->second);
		}

// 		template<typename T_parent_widget_slot_type, typename T_widget_type>
// 		void add_child(Ui_parent_widget<T_parent_widget_slot_type>& inout_parent, T_widget_type& inout_widget, const T_parent_widget_slot_type& in_slot_data) const
// 		{
// 			{
// 				auto it = m_ui_state.m_widget_lut.find(inout_widget.m_key.value());
// 				assert(it != m_ui_state.m_widget_lut.end() && "Widget was not created properly, please use Ui_context::create_widget");
// 				assert(it->second == &inout_widget && "Widget was not created properly, please use Ui_context::create_widget");
// 			}
// 
// 			{
// 				auto it = m_ui_state.m_widget_lut.find(inout_parent.m_key.value());
// 				assert(it != m_ui_state.m_widget_lut.end() && "Parent widget was not created properly, please use Ui_context::create_widget");
// 				assert(it->second == &inout_parent && "Parent widget was not created properly, please use Ui_context::create_widget");
// 			}
// 
// 			inout_parent.add_child(inout_widget, in_slot_data);
// 		}

		void set_window_size(const std::string& in_root_widget_key, const glm::vec2& in_size)
		{
			m_ui_state.m_root_to_window_size_lut[in_root_widget_key] = in_size;
		}

		State_ui& m_ui_state;
	};

	struct Ui_slot_canvas : Ui_slot
	{
		Ui_slot_canvas& anchors(const Ui_anchors& in_anchors) { m_anchors = in_anchors; return *this; }
		Ui_slot_canvas& translation(const glm::vec2& in_translation) { m_translation = in_translation; return *this; }
		Ui_slot_canvas& size(const glm::vec2& in_size) { m_size = in_size; return *this; }
		Ui_slot_canvas& pivot(const glm::vec2& in_pivot) { m_pivot = in_pivot; return *this; }
		Ui_slot_canvas& rotation(float in_rotation) { m_rotation = in_rotation; return *this; }
		Ui_slot_canvas& z_order(ui32 in_z_order) { m_z_order = in_z_order; return *this; }

		Ui_anchors m_anchors;

		glm::vec2 m_translation;
		glm::vec2 m_size;
		glm::vec2 m_pivot;
		float m_rotation = 0;

		ui32 m_z_order = 0;
	};

	struct Ui_widget_canvas : Ui_parent_widget<Ui_slot_canvas>
	{
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

				//Todo (carl): maaaybe we should support stretching with anchors, so if they are not the same we restrain the size inbetween the anchor points?

				child.m_global_render_size = slot.m_size / m_reference_dimensions;
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

				prev_x = child.m_render_translation.x - m_render_translation.x + child.m_global_render_size.x;
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


		glm::vec2 get_content_size(const glm::vec2& in_window_size) const override { return m_image_size / in_window_size; };

		Asset_ref<Asset_material> m_material;
		glm::vec2 m_image_size = { 64.0f, 64.0f };

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) override
		{
			auto update_lambda =
			[in_final_translation, in_final_size, in_final_rotation, mat = m_material, id = m_window_id](Render_object_ui& inout_object)
			{
				inout_object.m_lefttop = in_final_translation;
				inout_object.m_rightbottom = in_final_translation + in_final_size;
				inout_object.m_window_id = id;

				if (!inout_object.m_material)
				{
					inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
					inout_object.m_material = mat.get_mutable();
				}
			};

			if (m_ro_id.is_valid())
				inout_render_scene_state.m_ui_elements.update_object(m_ro_id, update_lambda);
			else
				m_ro_id = inout_render_scene_state.m_ui_elements.create_object(update_lambda);
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		Update_list_id<Render_object_ui> m_ro_id;
	};
}
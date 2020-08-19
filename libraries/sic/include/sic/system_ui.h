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

	struct Ui_widget
	{
		friend struct System_ui;

		virtual void calculate_render_transform();

		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) { in_final_translation; in_final_size; in_final_rotation; inout_render_scene_state; }

		virtual void make_dirty(std::vector<Ui_widget*>& out_dirty_widgets);

		virtual void gather_dependencies(std::vector<Asset_header*>& out_assets) { out_assets; }

		std::optional<std::string> m_key;
		struct Ui_parent_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		Update_list_id<Render_object_window> m_window_id;

		glm::vec2 m_local_scale = { 1.0f, 1.0f };
		glm::vec2 m_local_translation = { 0.0f, 0.0f };
		float m_local_rotation = 0.0f;

		glm::vec2 m_render_translation;
		glm::vec2 m_render_size;
		float m_render_rotation;
	};

	struct Ui_parent_widget_base : Ui_widget
	{
		virtual void calculate_render_transform_for_slot_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;
	};

	struct Ui_slot
	{
	};

	struct Ui_slot_canvas : Ui_slot
	{
		Ui_anchors m_anchors;

		glm::vec2 m_translation;
		glm::vec2 m_size;
		glm::vec2 m_pivot;
		float m_rotation = 0;

		ui32 m_z_order = 0;
	};

	template <typename T_slot_type>
	struct Ui_parent_widget : Ui_parent_widget_base
	{
		using Slot_type = T_slot_type;
		void calculate_render_transform() override final
		{
			Ui_widget::calculate_render_transform();

			for (auto&& child : m_child_widgets)
				child->calculate_render_transform();
		}

		void calculate_render_transform_for_slot_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) override final
		{
			assert(in_widget.m_parent == this && "Parent mismatch!");
			calculate_render_transform_for_slot(m_slots[in_widget.m_slot_index], out_translation, out_size, out_rotation);

			out_translation += m_render_size * (m_render_translation - glm::vec2(0.5f, 0.5f));
			out_size *= m_render_size;
			out_rotation += m_render_rotation;
		}

		virtual void calculate_render_transform_for_slot(const T_slot_type& in_slot, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;

		void make_dirty(std::vector<Ui_widget*>& out_dirty_widgets) final
		{
			Ui_widget::make_dirty(out_dirty_widgets);
			for (auto&& child_widget : m_child_widgets)
				child_widget->make_dirty(out_dirty_widgets);
		}

		template<typename T_child_type>
		void add_child(T_slot_type&& in_slot_data, T_child_type&& in_widget_data)
		{
			assert(!in_widget_data.m_parent && "Widget has already been added to some parent.");
			m_slots.emplace_back(in_slot_data);
			m_child_widgets.emplace_back(new T_child_type(in_widget_data));
			m_child_widgets.back()->m_parent = this;
			m_child_widgets.back()->m_slot_index = static_cast<i32>(m_slots.size() - 1);
			m_child_widgets.back()->m_window_id = m_window_id;
		}

		std::vector<T_slot_type> m_slots;
		std::vector<std::unique_ptr<Ui_widget>> m_child_widgets;
	};

	struct Ui_widget_canvas : Ui_parent_widget<Ui_slot_canvas>
	{
		void calculate_render_transform_for_slot(const Ui_slot_canvas& in_slot, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) override final
		{
			glm::vec2 anchor_vec = in_slot.m_anchors.get_max() - in_slot.m_anchors.get_min();
			glm::vec2 anchor_scale = anchor_vec;

			//if both anchors are 0 on all, translation should be relative to top left
			//if both anchors are 1 on all, translation should be relative to bottom right

			//if both anchors are not the same, translation should be relative to center of both
			glm::vec2 translation = in_slot.m_translation / m_reference_dimensions;
			translation += in_slot.m_anchors.get_min() + (anchor_vec * 0.5f);

			//Todo (carl): maaaybe we should support stretching with anchors, so if they are not the same we restrain the size inbetween the anchor points?

			out_size *= in_slot.m_size / m_reference_dimensions;
			out_translation += translation;
			out_translation -= out_size * (in_slot.m_pivot - glm::vec2(0.5f, 0.5f));
			out_rotation += in_slot.m_rotation;
		}

		glm::vec2 m_reference_dimensions;
	};

	struct Ui_widget_image : Ui_widget
	{
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) override
		{
			auto update_lambda =
				[in_final_translation, in_final_size, in_final_rotation, mat = m_material, id = m_window_id](Render_object_ui& inout_object)
			{
				const glm::vec2 half_size = in_final_size * 0.5f;
				inout_object.m_lefttop = in_final_translation - half_size;
				inout_object.m_rightbottom = in_final_translation + half_size;
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

		void gather_dependencies(std::vector<Asset_header*>& out_assets) override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }

		Asset_ref<Asset_material> m_material;
		glm::vec4 m_color_and_opacity = { 1.0f, 1.0f, 1.0f, 1.0f };

		Update_list_id<Render_object_ui> m_ro_id;
	};

	struct State_ui : State
	{
		friend struct System_ui;

		template <typename T_widget_type>
		void update_widget(const std::string& in_key, std::function<void(T_widget_type&)> in_update_func)
		{
			std::scoped_lock lock(m_update_lock);

			m_updates.push_back
			(
				[in_update_func, in_key, this]()
				{
					auto it = m_widget_lut.find(in_key);
					if (it == m_widget_lut.end())
						return;

					in_update_func(*reinterpret_cast<T_widget_type*>(it->second));
					it->second->make_dirty(m_dirty_widgets);
				}
			);
		}

		template <typename T_widget_type>
		void create_widget(const std::string& in_key, std::function<void(T_widget_type&)> in_update_func)
		{
			std::scoped_lock lock(m_update_lock);

			m_updates.push_back
			(
				[in_update_func, in_key, this]()
				{
					assert(m_widget_lut.find(in_key) == m_widget_lut.end() && "Can not have two widgets with same key!");

					m_widgets.push_back(std::make_unique<T_widget_type>());
					auto& widget = m_widget_lut[in_key] = m_widgets.back().get();

					in_update_func(*reinterpret_cast<T_widget_type*>(widget));
					widget->make_dirty(m_dirty_widgets);
				}
			);
		}

	private:
		std::mutex m_update_lock;
		std::vector<std::function<void()>> m_updates;
		std::vector<std::unique_ptr<Ui_widget>> m_widgets;
		std::unordered_map<std::string, Ui_widget*> m_widget_lut;
		std::vector<Ui_widget*> m_dirty_widgets;
	};

	struct System_ui : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
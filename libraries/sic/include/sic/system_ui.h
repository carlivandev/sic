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

	struct Ui_parent_widget_base;
	struct State_ui;

	struct Ui_widget
	{
		friend struct Ui_context;
		friend struct System_ui;

		template <typename T_slot_type>
		friend struct Ui_parent_widget;

		virtual void calculate_render_transform();

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

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) { in_final_translation; in_final_size; in_final_rotation; inout_render_scene_state; }

		virtual void destroy(State_ui& inout_ui_state);

		glm::vec2 m_render_translation;
		glm::vec2 m_render_size;
		float m_render_rotation;

	private:
		std::optional<std::string> m_key;
		Ui_parent_widget_base* m_parent = nullptr;
		i32 m_slot_index = -1;
		On_asset_loaded::Handle m_dependencies_loaded_handle;
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

	};

	struct System_ui : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};

	struct Ui_parent_widget_base : Ui_widget
	{
		virtual void calculate_render_transform_for_slot_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;
	};

	struct Ui_slot
	{
	};

	template <typename T_slot_type>
	struct Ui_parent_widget : Ui_parent_widget_base
	{
		friend struct Ui_context;

		using Slot_type = T_slot_type;

		void calculate_render_transform() override final
		{
			Ui_widget::calculate_render_transform();

			for (auto&& child : m_child_widgets)
				child->calculate_render_transform();
		}

		void calculate_render_transform_for_slot_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) override final
		{
			assert(in_widget.get_parent() == this && "Parent mismatch!");
			calculate_render_transform_for_slot(m_slots[in_widget.get_slot_index()], out_translation, out_size, out_rotation);

			out_translation += m_render_size * (m_render_translation - glm::vec2(0.5f, 0.5f));
			out_size *= m_render_size;
			out_rotation += m_render_rotation;
		}

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final
		{
			Ui_widget::gather_dependencies(out_assets);
			for (auto&& child_widget : m_child_widgets)
				child_widget->gather_dependencies(out_assets);
		}

		virtual void calculate_render_transform_for_slot(const T_slot_type& in_slot, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;

	protected:
		virtual void update_render_scene(const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, State_render_scene& inout_render_scene_state) override
		{
			Ui_widget::update_render_scene(in_final_translation, in_final_size, in_final_rotation, inout_render_scene_state);
			for (auto&& child_widget : m_child_widgets)
				child_widget->update_render_scene(child_widget->m_render_translation, child_widget->m_render_size, child_widget->m_render_rotation, inout_render_scene_state);
		}

		virtual void destroy(State_ui& inout_ui_state) override final
		{
			for (auto&& child_widget : m_child_widgets)
				child_widget->destroy(inout_ui_state);

			Ui_widget::destroy(inout_ui_state);
		}

	private:
		template<typename T_child_type>
		void add_child(T_child_type& inout_widget, T_slot_type&& in_slot_data)
		{
			assert(!inout_widget.m_parent && "Widget has already been added to some parent.");
			m_slots.emplace_back(in_slot_data);
			m_child_widgets.emplace_back(&inout_widget);
			inout_widget.m_parent = this;
			inout_widget.m_slot_index = static_cast<i32>(m_slots.size() - 1);
			inout_widget.m_window_id = m_window_id;
		}

		std::vector<T_slot_type> m_slots;
		std::vector<Ui_widget*> m_child_widgets;
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
			widget->m_widget_index = new_idx;

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

		template<typename T_parent_widget_slot_type, typename T_widget_type>
		void add_child(Ui_parent_widget<T_parent_widget_slot_type>& inout_parent, T_widget_type& inout_widget, T_parent_widget_slot_type&& in_slot_data) const
		{
			{
				auto it = m_ui_state.m_widget_lut.find(inout_widget.m_key.value());
				assert(it != m_ui_state.m_widget_lut.end() && "Widget was not created properly, please use Ui_context::create_widget");
				assert(it->second == &inout_widget && "Widget was not created properly, please use Ui_context::create_widget");
			}

			{
				auto it = m_ui_state.m_widget_lut.find(inout_parent.m_key.value());
				assert(it != m_ui_state.m_widget_lut.end() && "Parent widget was not created properly, please use Ui_context::create_widget");
				assert(it->second == &inout_parent && "Parent widget was not created properly, please use Ui_context::create_widget");
			}

			inout_parent.add_child(inout_widget, std::move(in_slot_data));
		}

		State_ui& m_ui_state;
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
		Asset_ref<Asset_material> m_material;
		glm::vec4 m_color_and_opacity = { 1.0f, 1.0f, 1.0f, 1.0f };

	protected:
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

		void gather_dependencies(std::vector<Asset_header*>& out_assets) const override final { if (m_material.is_valid()) out_assets.push_back(m_material.get_header()); }
		Update_list_id<Render_object_ui> m_ro_id;
	};
}
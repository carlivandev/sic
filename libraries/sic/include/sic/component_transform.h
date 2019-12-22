#pragma once
#include "sic/component.h"
#include "sic/delegate.h"

#include "glm/gtc/quaternion.hpp"
#include "glm/vec3.hpp"

namespace sic
{
	struct Transform
	{
		static constexpr glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		static constexpr glm::vec3 right = { 1.0f, 0.0f, 0.0f };
		static constexpr glm::vec3 forward = { 0.0f, 0.0f, -1.0f };

		void set_rotation(const glm::quat& in_rotation)
		{
			m_rotation = in_rotation;
		}

		void set_rotation(const glm::vec3& in_euler_angles)
		{
			m_rotation = glm::quat(in_euler_angles);
		}

		void set_translation(const glm::vec3& in_translation)
		{
			m_translation = in_translation;
		}

		void set_scale(const glm::vec3& in_scale)
		{
			m_scale = in_scale;
		}

		void rotate(const glm::quat& in_rotation)
		{
			m_rotation *= in_rotation;
		}

		void rotate(const glm::vec3& in_euler_angles)
		{
			rotate(glm::quat(in_euler_angles));
		}

		void rotate_around(float in_angle, const glm::vec3& in_axis)
		{
			m_rotation = glm::angleAxis(in_angle, in_axis) * m_rotation;
			m_rotation = glm::normalize(m_rotation);
		}

		void look_at(const glm::vec3& in_direction, const glm::vec3& in_up)
		{
			m_rotation = glm::quatLookAt(in_direction, in_up);
			m_rotation = glm::normalize(m_rotation);
		}

		void translate(const glm::vec3& in_translation)
		{
			m_translation += in_translation;
		}

		void pitch(float in_angle)
		{
			m_rotation = glm::rotate(m_rotation, in_angle, right);
			m_rotation = glm::normalize(m_rotation);
		}

		void yaw(float in_angle)
		{
			m_rotation = glm::rotate(m_rotation, in_angle, up);
			m_rotation = glm::normalize(m_rotation);
		}

		void roll(float in_angle)
		{
			m_rotation = glm::rotate(m_rotation, in_angle, forward);
			m_rotation = glm::normalize(m_rotation);
		}

		glm::mat4 get_matrix() const
		{
			glm::mat4 mat(1);
			mat = mat * glm::mat4_cast(m_rotation);
			mat = glm::scale(mat, m_scale);
			mat[3][0] = m_translation.x;
			mat[3][1] = m_translation.y;
			mat[3][2] = m_translation.z;

			return std::move(mat);
		}

		glm::vec3 get_forward() const
		{
			return m_rotation * forward;
		}

		glm::vec3 get_right() const
		{
			return m_rotation * right;
		}

		glm::vec3 get_up() const
		{
			return m_rotation * up;
		}

		glm::vec3 get_euler_angles() const
		{
			return glm::eulerAngles(m_rotation);
		}

		glm::quat m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 m_translation = { 0.0f, 0.0f, 0.0f };
		//local-space
		glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };
	};

	struct Component_transform : public Component
	{
		struct On_updated : Delegate<const Component_transform&> {};

		void set_rotation(const glm::quat& in_rotation)
		{
			if (in_rotation != m_transform.m_rotation)
			{
				m_transform.set_rotation(in_rotation);
				m_on_updated.invoke(*this);
			}
		}

		void set_rotation(const glm::vec3& in_euler_angles)
		{
			const glm::quat new_rot(in_euler_angles);

			if (new_rot != m_transform.m_rotation)
			{
				m_transform.set_rotation(new_rot);
				m_on_updated.invoke(*this);
			}
		}

		void set_translation(const glm::vec3& in_translation)
		{
			if (in_translation != m_transform.m_translation)
			{
				m_transform.set_translation(in_translation);
				m_on_updated.invoke(*this);
			}
		}

		void set_scale(const glm::vec3& in_scale)
		{
			if (in_scale != m_transform.m_scale)
			{
				m_transform.set_scale(in_scale);
				m_on_updated.invoke(*this);
			}
		}

		void rotate(const glm::quat& in_rotation)
		{
			m_transform.rotate(in_rotation);
			m_on_updated.invoke(*this);
		}

		void rotate(const glm::vec3& in_euler_angles)
		{
			m_transform.rotate(in_euler_angles);
			m_on_updated.invoke(*this);
		}

		void rotate_around(float in_angle, const glm::vec3& in_axis)
		{
			m_transform.rotate_around(in_angle, in_axis);
			m_on_updated.invoke(*this);
		}

		void look_at(const glm::vec3& in_direction, const glm::vec3& in_up)
		{
			m_transform.look_at(in_direction, in_up);
			m_on_updated.invoke(*this);
		}

		void translate(const glm::vec3& in_translation)
		{
			m_transform.translate(in_translation);
			m_on_updated.invoke(*this);
		}

		void pitch(float in_angle)
		{
			m_transform.pitch(in_angle);
			m_on_updated.invoke(*this);
		}

		void yaw(float in_angle)
		{
			m_transform.yaw(in_angle);
			m_on_updated.invoke(*this);
		}

		void roll(float in_angle)
		{
			m_transform.roll(in_angle);
			m_on_updated.invoke(*this);
		}

		const glm::quat& get_rotation() const
		{
			return m_transform.m_rotation;
		}

		const glm::vec3& get_translation() const
		{
			return m_transform.m_translation;
		}

		const glm::vec3& get_scale() const
		{
			return m_transform.m_scale;
		}

		glm::mat4 get_matrix() const
		{
			return m_transform.get_matrix();
		}

		glm::vec3 get_forward() const
		{
			return m_transform.get_forward();
		}

		glm::vec3 get_right() const
		{
			return m_transform.get_right();
		}

		glm::vec3 get_up() const
		{
			return m_transform.get_up();
		}

		glm::vec3 get_euler_angles() const
		{
			return m_transform.get_euler_angles();
		}

		On_updated m_on_updated;

	protected:
		Transform m_transform;
	};
}
#include "impuls/pch.h"
#include "impuls/engine_context.h"
#include "impuls/object_base.h"

void impuls::engine_context::add_child(i_object_base& in_parent, i_object_base& in_child)
{
	for (i_object_base* child : in_parent.m_children)
	{
		if (child == &in_child)
			return;
	}

	if (in_child.m_parent)
	{
		for (i32 i = 0; i < in_child.m_parent->m_children.size(); i++)
		{
			if (in_child.m_parent->m_children[i] == &in_child)
			{
				in_child.m_parent->m_children[i] = in_child.m_parent->m_children.back();
				in_child.m_parent->m_children.pop_back();
				break;
			}
		}
	}

	in_parent.m_children.push_back(&in_child);

	in_child.m_parent = &in_parent;
}

void impuls::engine_context::unchild(i_object_base& in_child)
{
	if (in_child.m_parent)
	{
		for (i32 i = 0; i < in_child.m_parent->m_children.size(); i++)
		{
			if (in_child.m_parent->m_children[i] == &in_child)
			{
				in_child.m_parent->m_children[i] = in_child.m_parent->m_children.back();
				in_child.m_parent->m_children.pop_back();
				break;
			}
		}
	}

	in_child.m_parent = nullptr;
}

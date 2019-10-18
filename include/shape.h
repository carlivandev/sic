#pragma once
#include "impuls/component.h"
#include "impuls/object.h"

struct shape_data : public impuls::i_component
{
	impuls::i32 m_pos_x = 0;
	impuls::i32 m_pos_y = 0;
	impuls::i32 shape_idx = -1;
	impuls::i32 rotation_idx = -1;
};

struct shape : public impuls::i_object<shape_data>{};
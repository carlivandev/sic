#include "pch.h"
#include "shape_renderer_system.h"
#include "shape.h"

#include "impuls/world_context.h"
#include "fmt/format.h"

namespace shapes
{
	static constexpr const wchar_t line_shape[] =
		L" x  " //0
		L" x  "
		L" x  "
		L" x  "

		L"    " //1
		L"xxxx"
		L"    "
		L"    "

		L"  x " //2
		L"  x "
		L"  x "
		L"  x "

		L"    " //3
		L"    "
		L"xxxx"
		L"    "
		;

	static constexpr const wchar_t square_shape[] =
		L"    " //0
		L" oo "
		L" oo "
		L"    "

		L"    " //1
		L" oo "
		L" oo "
		L"    "

		L"    " //2
		L" oo "
		L" oo "
		L"    "

		L"    " //3
		L" oo "
		L" oo "
		L"    "
		;

	static constexpr const wchar_t l_shape[] =
		L"ee  " //0
		L" e  "
		L" e  "
		L"    "

		L"  e " //1
		L"eee "
		L"    "
		L"    "

		L" e  " //2
		L" e  "
		L" ee "
		L"    "

		L"    " //3
		L"eee "
		L"e   "
		L"    "
		;

	static constexpr const wchar_t j_shape[] =
		L" aaa" //0
		L" a  "
		L" a  "
		L"    "

		L"    " //1
		L"aaa "
		L"  a "
		L"    "

		L" a  " //2
		L" a  "
		L"aa  "
		L"    "

		L"a   " //3
		L"aaa "
		L"    "
		L"    "
		;

	static constexpr const wchar_t tee_shape[] =
		L" u  " //0
		L"uuu "
		L"    "
		L"    "

		L" u  " //1
		L" uu "
		L" u  "
		L"    "

		L"    " //2
		L"uuu "
		L" u  "
		L"    "

		L" u  " //3
		L"uu  "
		L" u  "
		L"    "
		;

	static constexpr const wchar_t z_shape[] =
		L" z  " //0
		L"zz  "
		L"z   "
		L"    "

		L"zz  " //1
		L" zz "
		L"    "
		L"    "

		L"  z " //2
		L" zz "
		L" z  "
		L"    "

		L"    " //3
		L"zz  "
		L" zz "
		L"    "
		;

	static constexpr const wchar_t s_shape[] =
		L"s   " //0
		L"ss  "
		L" s  "
		L"    "

		L" ss " //1
		L"ss  "
		L"    "
		L"    "

		L" s  " //2
		L" ss "
		L"  s "
		L"    "

		L"    " //3
		L" ss "
		L"ss  "
		L"    "
		;

	constexpr const wchar_t* data[] =
	{
		line_shape,
		square_shape,
		l_shape,
		j_shape,
		tee_shape,
		z_shape,
		s_shape

	};
}

namespace shape_renderer_system_constants
{
	constexpr impuls::i32 screen_width = 120;
	constexpr impuls::i32 screen_height = 40;
}

void shape_renderer_system::on_created(impuls::world_context&& in_context) const
{
	in_context.register_state<state_shape_renderer>("state_shape_renderer");
	
	in_context.register_component_type<shape_data>("shape_data")
		.property("pos_x", &shape_data::m_pos_x)
		.property("pos_y", &shape_data::m_pos_y)
		.property("shape_idx", &shape_data::shape_idx)
		.property("rotation_idx", &shape_data::rotation_idx)
	;

	in_context.register_object<shape>("shape");
}

void shape_renderer_system::on_begin_simulation(impuls::world_context&& in_context) const
{
	if (auto state = in_context.get_state<state_shape_renderer>())
	{
		state->m_console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		SetConsoleActiveScreenBuffer(state->m_console_handle);

		state->m_screen_buffer.resize(shape_renderer_system_constants::screen_width * shape_renderer_system_constants::screen_height);
	}
}

void shape_renderer_system::on_tick(impuls::world_context&& in_context, float in_time_delta) const
{
	auto state = in_context.get_state<state_shape_renderer>();

	if (!state)
		return;

	memset(state->m_screen_buffer.data(), 0, sizeof(wchar_t) * state->m_screen_buffer.size());

	draw_shapes(in_context, *state);

	const std::wstring fps_counter = fmt::format(L"FPS={0:.2f}", 1.0f / in_time_delta);
	swprintf_s(state->m_screen_buffer.data(), fps_counter.size() + 1, fps_counter.c_str());


	state->m_screen_buffer[shape_renderer_system_constants::screen_width * shape_renderer_system_constants::screen_height - 1] = '\0';

	DWORD dwBytesWritten = 0;
	WriteConsoleOutputCharacterW(state->m_console_handle, state->m_screen_buffer.data(), shape_renderer_system_constants::screen_width * shape_renderer_system_constants::screen_height, { 0,0 }, &dwBytesWritten);
}

void shape_renderer_system::on_end_simulation(impuls::world_context&& in_context) const
{
	in_context;
}

void shape_renderer_system::draw_shapes(const impuls::world_context& in_context, state_shape_renderer& in_state) const
{
	constexpr impuls::i32 shape_width = 4;
	constexpr impuls::i32 shape_height = 4;

	for (auto&& cur_shape : in_context.each<shape_data>())
	{
		const wchar_t* start_of_shape = &shapes::data[cur_shape.shape_idx][cur_shape.rotation_idx * shape_width * shape_height];

		for (impuls::i32 y = 0; y < shape_height; y++)
		{
			for (impuls::i32 x = 0; x < shape_width; x++)
			{
				const wchar_t cur_char = start_of_shape[x + (shape_width * y)];

				if (cur_char != ' ')
					memcpy_s(&in_state.m_screen_buffer[cur_shape.m_pos_x + x + ((cur_shape.m_pos_y + y) * shape_renderer_system_constants::screen_width)], sizeof(wchar_t), &cur_char, sizeof(wchar_t));
			}
		}
	}
}
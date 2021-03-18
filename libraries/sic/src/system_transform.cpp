#include "sic/system_transform.h"

void sic::System_transform::on_created(Engine_context in_context)
{
	in_context.register_component<Component_transform>("Component_transform");
}
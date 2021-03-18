#include "sic/core/thread_context.h"

sic::Log sic::g_log_temporary_storage("Temporary_storage", false);

sic::Thread_context& sic::this_thread()
{
	static thread_local Thread_context context;
	return context;
}

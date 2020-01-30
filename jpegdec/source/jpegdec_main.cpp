#include <stratosphere.hpp>

#include "jpegdec_decode_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x18000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {
    ncm::ProgramId CurrentProgramId = ncm::ProgramId::JpegDec;

    namespace result {

        bool CallFatalOnResultAssertion = true;

    }

}

using namespace ams;

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::SetVersionForLibnx();
}

void __appExit(void) {
}

namespace {
    constexpr size_t NumServers  = 1;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    /* N only offers one session. */
    constexpr sm::ServiceName DecodeServiceName    = sm::ServiceName::Encode("caps:dc");
    constexpr size_t          DecodeMaxSessions    = 2;

}

int main(int argc, char **argv)
{
    R_ASSERT(g_server_manager.RegisterServer<jpegdec::DecodeService>(DecodeServiceName, DecodeMaxSessions));

    g_server_manager.LoopProcess();

    return 0;
}

#include <Helix/Platform/System/DynamicLibrary.h>
#include <Helix/Platform/IO/FileSystem.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Time/Time.h>
#include <Helix/Platform/Window/Window.h>
#include <Helix/Core/Types/Guid.h>
#include <Helix/Core/Strings/Format.h>
#include <Helix/Testing/Test.h>

#include <atomic>
#include <cstdlib>

#define HX_CHECK_MSG_NO_DISPLAY() HX_CHECK(std::getenv("DISPLAY") == nullptr)

using namespace helix;
using namespace helix::platform;

HX_TEST(DynamicLibrary, OpenSystemLibrary)
{
#if HX_PLATFORM_WINDOWS
    const char* name = "kernel32.dll";
    const char* symbol = "GetTickCount64";
#elif HX_PLATFORM_LINUX
    const char* name = "libc.so.6";
    const char* symbol = "malloc";
#else
    const char* name = nullptr;
    const char* symbol = nullptr;
#endif
    if (!name)
        return;
    DynamicLibrary lib;
    HX_REQUIRE(lib.Open(name));
    HX_CHECK(lib.Symbol(symbol) != nullptr);
    HX_CHECK(lib.Symbol("helix_symbol_that_does_not_exist") == nullptr);
    DynamicLibrary missing;
    HX_CHECK(!missing.Open("helix_missing_library_xyz"));
}

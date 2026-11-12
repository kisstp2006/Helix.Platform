#include <Helix/Core/Strings/Format.h>
#include <Helix/Core/Types/Guid.h>
#include <Helix/Platform/System/Process.h>
#include <Helix/Platform/Window/Window.h>
#include <Helix/Testing/Test.h>

using namespace helix;
using namespace helix::platform;

HX_TEST(Process, SingleInstanceHoldsItsName)
{
    String name = Format("test-{}", Guid::Generate().ToString());
    SingleInstance first;
    HX_CHECK(first.Acquire(name));
#if HX_PLATFORM_WINDOWS || HX_PLATFORM_LINUX
    HX_CHECK(first.IsHeld());
    // A second hold of the name fails while the first lives, and succeeds once it is let go.
    SingleInstance second;
    HX_CHECK(!second.Acquire(name) && !second.IsHeld());
    first.Release();
    HX_CHECK(second.Acquire(name) && second.IsHeld());
#endif
}

HX_TEST(Process, DebuggerAndDisplayCallsAreSafe)
{
    // Under a test runner no debugger is expected; the calls must simply work everywhere.
    (void)IsDebuggerAttached();
    SetKeepScreenOn(true);
    SetKeepScreenOn(false);
    SetDisplayOrientation(DisplayOrientation::Auto);
    HX_CHECK(true);
}

HX_TEST(Process, EnvironmentVariablesAreRead)
{
    String value;
    HX_CHECK(ReadEnvironment("PATH", value) && !value.Empty());
    HX_CHECK(!ReadEnvironment("HELIX_SURELY_NOT_SET_6F1D", value));
}

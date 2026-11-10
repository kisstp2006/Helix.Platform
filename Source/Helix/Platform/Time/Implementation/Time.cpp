#include <Helix/Platform/Time/Time.h>

namespace helix::platform
{
    f64 Now() { return f64(Ticks()) / f64(TicksPerSecond()); }
}

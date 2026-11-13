#include "Misc/Build.h"
// Shipping links every module into one binary. The game module already compiles
// these sources from Private/CoreLib, so skip them here to avoid duplicate symbols.
#if !IS_MONOLITHIC
#include "../../../Core/src/TrapTimeline.cpp"
#endif

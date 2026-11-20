#include "core.h"
#include "log.h"
#include "net.h"

int net_value() { return core_value() + log_value(); }

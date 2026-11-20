#include <cstdio>
#include "common.h"
#include "extra.h"

int main()
{
    std::printf("sub opt=%d def=%d common=%d extra=%d\n",
                OPT_SEEN, DEF_SEEN, common_value(), extra_value());
}

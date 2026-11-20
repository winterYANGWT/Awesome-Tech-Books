#include <cstdio>

int main() {
#if FAST_PATH
    const char* path = "fast";
#else
    const char* path = "slow";
#endif
    std::printf("%s path, mode=%s, data=%s, log=%d\n",
                path, APP_MODE, APP_DATA_DIR, LOG_ENABLED);
}

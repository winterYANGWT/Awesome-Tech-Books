#include <cstdio>

int main() {
#ifdef HAVE_UNISTD_H
    std::printf("HAVE_UNISTD_H=1\n");
#else
    std::printf("HAVE_UNISTD_H=0\n");
#endif
}

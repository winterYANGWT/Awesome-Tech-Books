#include <cstdio>

extern "C" int table_count();
extern "C" int table_sum();

int main() { std::printf("table: %d rows, sum %d\n", table_count(), table_sum()); }

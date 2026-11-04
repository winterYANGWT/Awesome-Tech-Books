// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）

#include <algorithm>
#include <cstdio>
#include <execution>
#include <random>
#include <string>
#include <vector>

struct counts {
    long total = 0;
    long errors = 0;
};

struct combine_counts {
    counts operator()(counts a, counts b) const {
        return {a.total + b.total, a.errors + b.errors};
    }
    counts operator()(counts a, int status) const {
        return {a.total + 1, a.errors + (status != 200 ? 1 : 0)};
    }
    counts operator()(int status, counts a) const { return (*this)(a, status); }
    counts operator()(int a, int b) const {
        return {2, (a != 200 ? 1 : 0) + (b != 200 ? 1 : 0)};
    }
};

int status_of(const std::string& line) {
    return line.substr(line.rfind(' ') + 1) == "200" ? 200 : 500;
}

int main() {
    std::mt19937 rng(20261103);
    std::vector<std::string> log;
    for (int i = 0; i < 200000; ++i) {
        log.push_back("GET /page" + std::to_string(rng() % 40) + ((rng() % 10) ? " 200" : " 500"));
    }

    const counts result = std::transform_reduce(std::execution::par, log.begin(), log.end(),
                                                counts{}, combine_counts{}, status_of);

    std::printf("total=%ld errors=%ld\n", result.total, result.errors);
}

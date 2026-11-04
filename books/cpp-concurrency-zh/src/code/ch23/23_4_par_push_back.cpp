// mode: race
// flags: -ltbb
// expect: data race

#include <algorithm>
#include <execution>
#include <numeric>
#include <vector>

int main() {
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 0);

    std::vector<int> output;
    std::for_each(std::execution::par, input.begin(), input.end(),
                  [&](int i) { output.push_back(i * 2 + 1); });

    return output.size() == input.size() ? 0 : 1;
}

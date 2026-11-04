// mode: run
// expect: is_lock_free = 0
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>

struct Config {
    int version;
};

std::atomic<std::shared_ptr<const Config>> slot;

int main() {
    auto initial = std::make_shared<const Config>(Config{0});
    slot.store(initial);
    std::printf("initial use_count = %ld\n", initial.use_count());
    std::printf("is_lock_free = %d\n", slot.is_lock_free());

    std::thread writer([] {
        for (int i = 1; i <= 100; ++i) {
            slot.store(std::make_shared<const Config>(Config{i}));
        }
    });
    std::thread reader([] {
        int loads = 0;
        for (int i = 0; i < 2000; ++i) {
            auto p = slot.load();
            if (p) ++loads;
        }
        std::printf("reader loads = %d\n", loads);
    });
    writer.join();
    reader.join();

    auto final = slot.load();
    std::printf("final version = %d\n", final->version);
    std::printf("final use_count = %ld\n", final.use_count());
}

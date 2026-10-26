// 第 19.2 节：Strategy 用 std::function 保存算法，同一类型的对象可以有不同行为
#include <functional>
#include <iostream>
#include <string>
#include <utility>

class Character;

int default_health(const Character& character);

class Character {
public:
    // 目标签名固定，具体可调用体可以是函数、lambda 或函数对象。
    using HealthFn = std::function<int(const Character&)>;

    explicit Character(std::string name, int base = 100,
                       HealthFn health = default_health)
        : name_(std::move(name)), base_(base), health_(std::move(health)) {}

    int health_value() const { return health_(*this); }
    int base() const { return base_; }
    const std::string& name() const { return name_; }

    // 运行期替换算法：这是函数指针/虚函数做不到的“按对象换行为”。
    void set_health(HealthFn health) { health_ = std::move(health); }

private:
    std::string name_;
    int base_;
    HealthFn health_;
};

int default_health(const Character& character) {
    return character.base();
}

int fragile_health(const Character&) {
    return 25;
}

class Level {
public:
    explicit Level(int bonus) : bonus_(bonus) {}
    int health(const Character& character) const {
        return character.base() + bonus_;
    }

private:
    int bonus_;
};

int main() {
    Character plain("plain");
    Character fragile("fragile", 100, fragile_health);
    std::cout << plain.name() << ": " << plain.health_value() << '\n';
    std::cout << fragile.name() << ": " << fragile.health_value() << '\n';

    Level level(20);
    Character in_level("in-level", 60,
                       [&level](const Character& c) { return level.health(c); });
    std::cout << in_level.name() << ": " << in_level.health_value() << '\n';

    // 同一个对象、同一个类型，运行期换一套算法。
    plain.set_health([](const Character& c) { return c.base() / 2; });
    std::cout << plain.name() << " after swap: " << plain.health_value() << '\n';

    // 函数指针也能直接存进 std::function，不需要单独的适配层。
    Character::HealthFn raw = fragile_health;
    Character another("another", 100, raw);
    std::cout << another.name() << ": " << another.health_value() << '\n';
}

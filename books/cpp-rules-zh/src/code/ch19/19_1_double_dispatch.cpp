// 第 19.1 节：用两次虚调用实现双重分派，两个对象的动态类型都参与选择
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class GameObject;
class Ship;
class Station;
class Asteroid;

class GameObject {
public:
    virtual ~GameObject() = default;
    virtual std::string name() const = 0;

    // 第一次分派：按左操作数的动态类型选一个 collide。
    virtual void collide(GameObject& other) = 0;
    // 第二次分派：按右操作数的动态类型选到具体重载。
    virtual void collide(Ship& other) = 0;
    virtual void collide(Station& other) = 0;
    virtual void collide(Asteroid& other) = 0;
};

class Ship : public GameObject {
public:
    std::string name() const override;
    void collide(GameObject& other) override;
    void collide(Ship& other) override;
    void collide(Station& other) override;
    void collide(Asteroid& other) override;
};

class Station : public GameObject {
public:
    std::string name() const override;
    void collide(GameObject& other) override;
    void collide(Ship& other) override;
    void collide(Station& other) override;
    void collide(Asteroid& other) override;
};

class Asteroid : public GameObject {
public:
    std::string name() const override;
    void collide(GameObject& other) override;
    void collide(Ship& other) override;
    void collide(Station& other) override;
    void collide(Asteroid& other) override;
};

void record(const GameObject& a, const GameObject& b, const char* rule) {
    std::cout << a.name() << " x " << b.name() << ": " << rule << '\n';
}

std::string Ship::name() const { return "ship"; }
std::string Station::name() const { return "station"; }
std::string Asteroid::name() const { return "asteroid"; }

// *this 的静态类型是 Ship，所以选 collide(Ship&)；
// 它是虚函数，再按 other 的动态类型分派一次。
void Ship::collide(GameObject& other) { other.collide(*this); }
void Ship::collide(Ship& other) { record(*this, other, "两船受损"); }
void Ship::collide(Station& other) { record(*this, other, "飞船对接空间站"); }
void Ship::collide(Asteroid& other) { record(*this, other, "小行星被摧毁"); }

void Station::collide(GameObject& other) { other.collide(*this); }
void Station::collide(Ship& other) { record(*this, other, "空间站接纳飞船"); }
void Station::collide(Station& other) { record(*this, other, "两站受损"); }
void Station::collide(Asteroid& other) { record(*this, other, "小行星被摧毁"); }

void Asteroid::collide(GameObject& other) { other.collide(*this); }
void Asteroid::collide(Ship& other) { record(*this, other, "飞船被撞"); }
void Asteroid::collide(Station& other) { record(*this, other, "空间站被撞"); }
void Asteroid::collide(Asteroid& other) { record(*this, other, "两颗小行星碎裂"); }

int main() {
    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(std::make_unique<Ship>());
    objects.push_back(std::make_unique<Station>());
    objects.push_back(std::make_unique<Asteroid>());

    for (std::size_t i = 0; i < objects.size(); ++i) {
        for (std::size_t j = i + 1; j < objects.size(); ++j) {
            objects[i]->collide(*objects[j]);
        }
    }
}

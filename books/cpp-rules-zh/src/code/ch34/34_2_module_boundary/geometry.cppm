// 第 34.2 节：模块接口用 export 决定哪些名字对导入方可见
export module geometry;

// 模块接口里定义的宏不会导出给导入方。
#define GEOMETRY_INTERNAL_LIMIT 64

// 没有 export：只在模块内部可见。
namespace detail {

constexpr double pi = 3.14159265358979323846;

double square(double x) {
    return x * x;
}

}  // namespace detail

// export 把 geo 里的名字放进模块的导出集合。
export namespace geo {

double circle_area(double radius) {
    return detail::pi * detail::square(radius);
}

constexpr int max_sides = 64;

struct Circle {
    double radius;
};

double area(const Circle& c) {
    return circle_area(c.radius);
}

}  // namespace geo

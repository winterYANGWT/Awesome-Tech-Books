// 第 33.3 节：按值成员必须看到完整定义
// mode: fail
// expect: incomplete
class Engine;

class Car {
    Engine engine_;   // 大小未知，不能作为按值成员
};

int main() {}

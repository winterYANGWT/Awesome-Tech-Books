// 第 18.2 节：私有继承不提供对外的 is-a 转换
// mode: fail
// expect: private
class Person {
public:
    virtual ~Person() = default;
};

class Student : private Person {};

void eat(const Person&) {}

int main() {
    Student student;
    eat(student);  // 错误：Student 到 Person 的转换在类外不可访问
}

// 第 18.4 节：多继承的合理用法——公开继承接口，私有继承实现
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class IPerson {
public:
    virtual ~IPerson() = default;
    virtual std::string name() const = 0;
    virtual std::string birthDate() const = 0;
};

class PersonInfo {
public:
    PersonInfo(std::string name, std::string birthDate)
        : name_(std::move(name)), birthDate_(std::move(birthDate)) {}
    virtual ~PersonInfo() = default;

    virtual std::string theName() const { return delimit(name_); }
    virtual std::string theBirthDate() const { return delimit(birthDate_); }

private:
    virtual std::string open() const { return "["; }
    virtual std::string close() const { return "]"; }
    std::string delimit(const std::string& value) const { return open() + value + close(); }

    std::string name_;
    std::string birthDate_;
};

class CPerson : public IPerson, private PersonInfo {
public:
    CPerson(std::string name, std::string birthDate)
        : PersonInfo(std::move(name), std::move(birthDate)) {}

    std::string name() const override { return PersonInfo::theName(); }
    std::string birthDate() const override { return PersonInfo::theBirthDate(); }

private:
    std::string open() const override { return ""; }
    std::string close() const override { return ""; }
};

int main() {
    PersonInfo raw("Ada Lovelace", "1815-12-10");
    std::cout << "PersonInfo 默认格式：" << raw.theName() << '\n';

    CPerson person("Ada Lovelace", "1815-12-10");
    const IPerson& iface = person;
    std::cout << "通过 IPerson* / IPerson&：" << iface.name() << "（" << iface.birthDate()
              << "）\n";

    std::cout << std::boolalpha
              << "CPerson -> IPerson*=" << std::is_convertible_v<CPerson*, IPerson*>
              << "，CPerson -> PersonInfo*="
              << std::is_convertible_v<CPerson*, PersonInfo*> << '\n';

    static_assert(std::is_convertible_v<CPerson*, IPerson*>);
    static_assert(!std::is_convertible_v<CPerson*, PersonInfo*>);
}

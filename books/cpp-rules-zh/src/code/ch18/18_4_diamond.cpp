// 第 18.4 节：普通菱形复制两份基类数据，虚继承共享一份
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class File {
public:
    explicit File(std::string name) : name_(std::move(name)) { ++constructed_; }
    virtual ~File() = default;

    const std::string& name() const { return name_; }
    static int constructed() { return constructed_; }
    static void reset() { constructed_ = 0; }

private:
    std::string name_;
    static inline int constructed_ = 0;
};

class InputFile : public File {
public:
    using File::File;
};

class OutputFile : public File {
public:
    using File::File;
};

// 普通多继承：IOFile 里有两个 File 子对象。
class IOFile : public InputFile, public OutputFile {
public:
    IOFile(std::string input, std::string output)
        : InputFile(std::move(input)), OutputFile(std::move(output)) {}
};

class VInputFile : virtual public File {
public:
    VInputFile() : File("unused-input") {}
    explicit VInputFile(std::string name) : File(std::move(name)) {}
};

class VOutputFile : virtual public File {
public:
    VOutputFile() : File("unused-output") {}
    explicit VOutputFile(std::string name) : File(std::move(name)) {}
};

// 虚继承：File 由最派生类 VIOFile 直接初始化，只存在一个子对象。
class VIOFile : public VInputFile, public VOutputFile {
public:
    explicit VIOFile(std::string name)
        : File(std::move(name)), VInputFile(), VOutputFile() {}
};

int main() {
    File::reset();
    IOFile io("in.txt", "out.txt");
    std::cout << "普通多继承：File 构造了 " << File::constructed() << " 次\n";
    std::cout << "  InputFile::name()=" << io.InputFile::name()
              << "，OutputFile::name()=" << io.OutputFile::name() << '\n';

    File::reset();
    VIOFile vio("shared.txt");
    std::cout << "虚继承：File 构造了 " << File::constructed() << " 次\n";
    std::cout << "  name()=" << vio.name() << '\n';

    static_assert(std::is_base_of_v<File, IOFile>);
    static_assert(std::is_base_of_v<File, VIOFile>);
}

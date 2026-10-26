// 第 28.6 节：文件流的默认截断、追加、app 的读写位置与打开失败检查
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void print_file(const fs::path& path, const char* title) {
    std::ifstream in(path);
    if (!in) {
        std::cout << title << ": 打开失败\n";
        return;
    }
    std::cout << title << ":\n";
    std::string line;
    while (std::getline(in, line)) {
        std::cout << line << '\n';
    }
}

int main() {
    const fs::path path = fs::temp_directory_path() / "cpp_rules_ch28_modes.txt";
    std::error_code ignored;
    fs::remove(path, ignored);

    // 默认的 ofstream 会截断同名文件：第二次打开后第一次写的内容已经没了。
    {
        std::ofstream out(path);
        out << "first\n";
    }
    {
        std::ofstream out(path);
        out << "second\n";
    }
    print_file(path, "默认 ofstream 两次打开后的内容");

    // 追加必须显式写 ios_base::app。
    {
        std::ofstream out(path, std::ios_base::app);
        out << "third\n";
    }
    print_file(path, "append 之后的内容");

    // 可读的 app 模式：本机 libstdc++/libc++ 都把初始读位置放在开头，
    // 但写操作始终定位到末尾，与当前读位置无关。
    {
        std::fstream io(path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
        if (!io) {
            std::cout << "in|out|app 打开失败\n";
            return 1;
        }
        std::string firstLine;
        std::getline(io, firstLine);
        std::cout << "in|out|app 打开后读到的第一行: " << firstLine << '\n';

        io << "fourth\n";  // 即使刚读过第一行，app 仍把这次写放到末尾
        io.clear();
        io.seekg(0);       // 重新从头读，必须先把读位置移回去
        std::cout << "app 写入后从头重读:\n";
        std::string line;
        while (std::getline(io, line)) {
            std::cout << line << '\n';
        }
    }

    // 打开失败不会抛异常，而是把流置为失败状态；不检查就读取只会得到空数据。
    std::ifstream missing(fs::temp_directory_path() / "cpp_rules_ch28_no_such_file.txt");
    std::cout << "打开不存在的文件: " << (missing ? "成功" : "失败") << '\n';

    fs::remove(path, ignored);
}

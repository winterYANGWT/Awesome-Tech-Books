// mode: run
// std: c++23
// flags: -fno-inline -lstdc++exp
// expect: 捕获点栈帧
// 异常传回调用者后，原来的调用栈已经展开；在 catch 点用 std::stacktrace
// 记录一份栈，随 exception_ptr 一起交给上层。
#include <cstdio>
#include <exception>
#include <stacktrace>
#include <stdexcept>
#include <string>

namespace {

void leaf() {
    throw std::runtime_error("底层失败");
}

void middle() {
    try {
        leaf();
    } catch (...) {
        const std::stacktrace trace = std::stacktrace::current();
        std::printf("捕获点栈帧（只列本文件的帧）：\n");
        for (const std::stacktrace_entry& frame : trace) {
            const std::string file = frame.source_file();
            const std::size_t slash = file.find_last_of('/');
            const std::string name =
                slash == std::string::npos ? file : file.substr(slash + 1);
            if (name == "21_1_stacktrace.cpp") {
                std::printf("  %s at %s:%u\n", frame.description().c_str(),
                            name.c_str(),
                            static_cast<unsigned>(frame.source_line()));
            }
        }
        throw;  // 原样交给 main，栈已经在这里被记下来了
    }
}

}  // namespace

int main() {
    try {
        middle();
    } catch (const std::exception& e) {
        std::printf("main 重新捕获：%s\n", e.what());
    }
    return 0;
}

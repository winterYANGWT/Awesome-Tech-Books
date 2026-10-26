// 第 11.5 节：重抛用 throw; 保留动态类型，throw e; 会按静态类型切片
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

struct AppError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct DbError : AppError {
    using AppError::AppError;
};

void rethrow_current() {
    try {
        throw DbError("主库连接断开");
    } catch (const AppError& e) {
        std::cout << "  内层捕获：" << e.what() << "\n";
        throw;                            // 重抛当前异常，不复制、不改类型
    }
}

void rethrow_by_name(const AppError& e) {
    throw e;                              // 复制一份静态类型为 AppError 的新异常
}

int main() {
    std::cout << "--- throw; ---\n";
    try {
        rethrow_current();
    } catch (const DbError& e) {
        std::cout << "  外层捕获 DbError：" << e.what() << "\n";
    } catch (const AppError& e) {
        std::cout << "  外层捕获 AppError：" << e.what() << "\n";
    }

    std::cout << "--- throw e; ---\n";
    try {
        try {
            throw DbError("从库连接断开");
        } catch (const AppError& e) {
            rethrow_by_name(e);
        }
    } catch (const DbError& e) {
        std::cout << "  外层捕获 DbError：" << e.what() << "\n";
    } catch (const AppError& e) {
        std::cout << "  外层捕获 AppError（已切片）：" << e.what() << "\n";
    }

    std::cout << "--- exception_ptr 延迟重抛 ---\n";
    std::exception_ptr saved;
    try {
        throw DbError("审计写入失败");
    } catch (...) {
        saved = std::current_exception();  // 保存的是异常对象本身，不是副本的静态类型
    }
    try {
        std::rethrow_exception(saved);
    } catch (const DbError& e) {
        std::cout << "  仍然捕获 DbError：" << e.what() << "\n";
    } catch (const AppError& e) {
        std::cout << "  退化成 AppError：" << e.what() << "\n";
    }
}

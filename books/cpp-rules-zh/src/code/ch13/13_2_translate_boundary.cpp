// 第 13.2 节：模块内部抛异常，C API 边界把异常翻译成错误码
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

struct NotFound : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct DbError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Row {
    int id;
    std::string name;
};

// 内部实现：只认异常，不知道外面的 C 调用方。
Row fetch_row(int id) {
    if (id == 7) {
        throw NotFound("row 7 does not exist");
    }
    if (id < 0) {
        throw DbError("database is offline");
    }
    return {id, "row-" + std::to_string(id)};
}

// C 边界：调用方只认识状态码和一段固定大小的消息。
// 所有异常都必须在这里被接住并翻译，一个也不能漏出去。
inline constexpr int api_ok = 0;
inline constexpr int api_not_found = 1;
inline constexpr int api_internal = 2;

struct ApiResult {
    int status;
    char message[128];
};

struct ApiRow {
    int id;
    char name[32];
};

ApiResult make_result(int status, const char* message) noexcept {
    ApiResult result{};
    result.status = status;
    std::snprintf(result.message, sizeof result.message, "%s", message);
    return result;
}

extern "C" ApiResult api_fetch(int id, ApiRow* out) noexcept {
    try {
        const Row row = fetch_row(id);
        out->id = row.id;
        std::snprintf(out->name, sizeof out->name, "%s", row.name.c_str());
        return make_result(api_ok, "ok");
    } catch (const NotFound& e) {
        return make_result(api_not_found, e.what());
    } catch (const std::exception& e) {
        return make_result(api_internal, e.what());
    } catch (...) {
        return make_result(api_internal, "unknown error");
    }
}

const char* status_name(int status) {
    switch (status) {
    case api_ok: return "ok";
    case api_not_found: return "not_found";
    default: return "internal";
    }
}

int main() {
    ApiRow row{};
    ApiResult result = api_fetch(7, &row);
    std::cout << "id=7  status=" << status_name(result.status)
              << "  message=" << result.message << "\n";

    result = api_fetch(-1, &row);
    std::cout << "id=-1 status=" << status_name(result.status)
              << "  message=" << result.message << "\n";

    result = api_fetch(3, &row);
    std::cout << "id=3  status=" << status_name(result.status)
              << "  row=" << row.name << "\n";
}

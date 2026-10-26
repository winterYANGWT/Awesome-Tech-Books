// 第 35.1 节：把内部异常翻译成 C 接口的错误码
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

enum class payment_status : int { ok = 0, declined = 2, network = 3, internal = 9 };

class declined : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class network_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// 模块内部照常用异常表达“为什么失败”。
int charge_impl(int cents, std::string& receipt) {
    if (cents < 0) {
        throw std::invalid_argument("negative amount");
    }
    if (cents == 0) {
        throw declined("card declined");
    }
    if (cents > 10000) {
        throw network_error("gateway timeout");
    }
    receipt = "receipt-" + std::to_string(cents);
    return cents;
}

// 导出的 C 接口不让异常穿过边界：catch(...) 是最后一道闸门。
extern "C" int api_charge(int cents, char* receipt, int capacity) noexcept {
    try {
        std::string text;
        const int charged = charge_impl(cents, text);
        if (receipt != nullptr && capacity > 0) {
            const size_t n = std::min(static_cast<size_t>(capacity - 1), text.size());
            std::memcpy(receipt, text.data(), n);
            receipt[n] = '\0';
        }
        return charged > 0 ? static_cast<int>(payment_status::ok)
                           : static_cast<int>(payment_status::internal);
    } catch (const declined&) {
        return static_cast<int>(payment_status::declined);
    } catch (const network_error&) {
        return static_cast<int>(payment_status::network);
    } catch (...) {
        return static_cast<int>(payment_status::internal);
    }
}

const char* status_name(int status) {
    switch (static_cast<payment_status>(status)) {
    case payment_status::ok: return "ok";
    case payment_status::declined: return "declined";
    case payment_status::network: return "network";
    case payment_status::internal: return "internal";
    }
    return "unknown";
}

int main() {
    char receipt[64];
    for (int amount : {1250, 0, 15000, -1}) {
        receipt[0] = '\0';
        const int status = api_charge(amount, receipt, static_cast<int>(sizeof receipt));
        std::cout << amount << " -> " << status_name(status)
                  << " receipt=" << (receipt[0] != '\0' ? receipt : "-") << '\n';
    }
}

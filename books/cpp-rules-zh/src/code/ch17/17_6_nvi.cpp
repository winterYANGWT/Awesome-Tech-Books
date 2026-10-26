// 第 17.6 节：公开非虚外壳集中做检查，私有虚函数只负责定制
#include <iostream>
#include <memory>
#include <string>

class Importer {
public:
    virtual ~Importer() = default;  // 允许经 Importer* 多态删除

    // 公开、非虚、稳定：前置检查、日志、计数都放在这里，派生类改不了。
    [[nodiscard]] bool import(const std::string& path) {
        ++attempts_;
        if (path.empty()) {
            std::cout << "  [外壳] 拒绝空路径\n";
            return false;
        }
        const bool ok = doImport(path);
        std::cout << "  [外壳] " << (ok ? "成功" : "失败")
                  << "，累计尝试 " << attempts_ << " 次\n";
        return ok;
    }

private:
    // 定制点：派生类可以覆盖，但外部调用不到。
    virtual bool doImport(const std::string& path) = 0;
    int attempts_ = 0;
};

class CsvImporter : public Importer {
private:
    bool doImport(const std::string& path) override {
        std::cout << "  [CsvImporter] 读取 " << path << '\n';
        return path.ends_with(".csv");
    }
};

int main() {
    std::unique_ptr<Importer> importer = std::make_unique<CsvImporter>();
    for (const char* path : {"data.csv", "data.txt", ""}) {
        std::cout << "import(\"" << path << "\")\n";
        const bool ok = importer->import(path);
        std::cout << "  -> " << (ok ? "true" : "false") << '\n';
    }
}

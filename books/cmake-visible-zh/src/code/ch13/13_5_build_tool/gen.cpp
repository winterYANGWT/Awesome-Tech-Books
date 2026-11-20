#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: gen <table.csv> <table.cpp>\n");
        return 2;
    }
    std::ifstream in(argv[1]);
    std::ofstream out(argv[2]);
    int sum = 0, count = 0;
    std::string line;
    while (std::getline(in, line)) {
        const auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        char* end = nullptr;
        const long value = std::strtol(line.c_str() + comma + 1, &end, 10);
        if (end == line.c_str() + comma + 1) continue;
        sum += static_cast<int>(value);
        ++count;
    }
    out << "// 由 gen 从 table.csv 生成，不要手改。\n"
        << "extern \"C\" int table_count() { return " << count << "; }\n"
        << "extern \"C\" int table_sum() { return " << sum << "; }\n";
}

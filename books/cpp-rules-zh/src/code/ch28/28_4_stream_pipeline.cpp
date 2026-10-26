// 第 28.4 节：用流引用和模板把“数据从哪来”与“怎么处理”分开
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<std::remove_reference_t<decltype(std::cin)>, std::istream>);
static_assert(std::is_same_v<std::remove_reference_t<decltype(std::cout)>, std::ostream>);

char to_upper(char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

// 处理函数只认“输入流”和“输出流”，不关心背后是控制台、文件还是 stringstream。
template <class In, class Out>
void transform_lines(In& in, Out& out, char (*transform)(char)) {
    std::string line;
    while (std::getline(in, line)) {
        for (char& c : line) {
            c = transform(c);
        }
        out << line << '\n';
    }
}

// 不做任何处理时，可以直接把输入缓冲区搬到输出流。
void echo_rdbuf(std::istream& in, std::ostream& out) {
    out << in.rdbuf();
}

int main() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path();
    const fs::path input = dir / "cpp_rules_ch28_pipeline_in.txt";
    const fs::path output = dir / "cpp_rules_ch28_pipeline_out.txt";

    {
        std::ofstream file(input);
        file << "alpha\nbeta\n";
    }

    // 同一个 transform_lines，实参换成 ifstream/ofstream 就能处理文件。
    {
        std::ifstream in(input);
        std::ofstream out(output);
        if (!in || !out) {
            std::cerr << "打开文件失败\n";
            return 1;
        }
        transform_lines(in, out, to_upper);
    }

    std::ifstream result(output);
    std::ostringstream shown;
    echo_rdbuf(result, shown);
    std::cout << "文件处理结果:\n" << shown.str();

    // 换成 stringstream，处理代码一行都不用改。
    std::istringstream memoryInput("one\ntwo\n");
    std::ostringstream memoryOutput;
    transform_lines(memoryInput, memoryOutput, to_upper);
    std::cout << "内存流结果:\n" << memoryOutput.str();

    std::error_code ignored;
    fs::remove(input, ignored);
    fs::remove(output, ignored);
}

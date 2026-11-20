// 不包含标准库头文件：本机没有 aarch64 的 sysroot，头文件找不到
extern "C" int blob_size() { return 4096; }

// lazy_late.cu: lazy.cu 的一部分。单独一个编译单元，它的 kernel 在可执行文件里是另一个模块。
// PART_OF: lazy.cu
__global__ void late(float* a) { a[0] += 1; }

void launch_late(float* a, cudaStream_t s) { late<<<1, 1, 0, s>>>(a); }

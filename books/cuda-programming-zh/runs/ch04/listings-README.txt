第 4 章 4.2、4.3 节的图所用的工具输出（CUDA 13.3，RTX 5060 Laptop GPU，计算能力 12.0）。
文件里的源文件绝对路径已替换成 tiny.cu，其余为工具原样输出。

tiny-lineinfo.ptx       nvcc -arch=sm_120 -lineinfo -ptx tiny.cu -o tiny.ptx
                        （-lineinfo 只多出 .loc/.file 行，指令和不加时相同）
tiny-sass-lines.txt     nvcc -arch=sm_120 -lineinfo -cubin tiny.cu -o tiny.cubin
                        nvdisasm -g -c tiny.cubin    每条 SASS 来自源码第几行；只保留 kernel 到 EXIT
tiny-sass-ptxlines.txt  nvdisasm -gp -c tiny.cubin   每条 SASS 来自 PTX 第几行（行号即 tiny-lineinfo.ptx 的行号）
fatbin-options.txt      对 4.3 节的 8 种编译选项各执行：nvcc <选项> tiny.cu -o tiny；
                        cuobjdump --list-elf --list-ptx tiny；./tiny

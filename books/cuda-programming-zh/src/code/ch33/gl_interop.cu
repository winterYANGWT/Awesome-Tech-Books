// gl_interop.cu: 没有窗口也能用 OpenGL：通过 EGL 在 GPU 上建一个 OpenGL 上下文，
// 把 OpenGL 的 buffer 注册给 CUDA，每“帧”映射、用 kernel 写入、解除映射，再由 OpenGL 读回检查。
//   nvcc -O2 -arch=native gl_interop.cu -lEGL -lOpenGL -o gl_interop && ./gl_interop
// NVCCFLAGS: -lEGL -lOpenGL
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <cuda_gl_interop.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)
#define EGL_OK(x)                                                                          \
    do {                                                                                   \
        if (!(x)) {                                                                        \
            fprintf(stderr, "%s:%d: EGL 错误 0x%x\n", __FILE__, __LINE__, eglGetError());   \
            exit(1);                                                                       \
        }                                                                                  \
    } while (0)

static double now_us() {
    return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

const size_t N = 16 << 20, BYTES = N * sizeof(float);   // 64 MB

__global__ void write_frame(float* a, size_t n, int frame) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        a[i] = float(frame) + float(i % 1000);
}

int main() {
    // ANCHOR: egl
    // 用 EGL 的 device 平台，在 CUDA 设备 0 对应的 GPU 上建一个不带窗口的 OpenGL 上下文。
    auto queryDevices = (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    auto queryDeviceAttrib = (PFNEGLQUERYDEVICEATTRIBEXTPROC)eglGetProcAddress("eglQueryDeviceAttribEXT");
    auto getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLDeviceEXT devices[8];
    EGLint ndev = 0;
    EGL_OK(queryDevices(8, devices, &ndev));
    EGLDeviceEXT device = EGL_NO_DEVICE_EXT;
    for (int i = 0; i < ndev; ++i) {
        EGLAttrib cuda_dev = -1;                      // 只有 NVIDIA 的设备有这个属性
        if (queryDeviceAttrib(devices[i], EGL_CUDA_DEVICE_NV, &cuda_dev) && cuda_dev == 0) device = devices[i];
    }
    EGL_OK(device != EGL_NO_DEVICE_EXT);
    EGLDisplay dpy = getPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, device, nullptr);
    EGLint major, minor;
    EGL_OK(eglInitialize(dpy, &major, &minor));
    EGL_OK(eglBindAPI(EGL_OPENGL_API));
    const EGLint cfg_attr[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
    EGLConfig cfg;
    EGLint ncfg;
    EGL_OK(eglChooseConfig(dpy, cfg_attr, &cfg, 1, &ncfg) && ncfg == 1);
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, nullptr);
    EGL_OK(ctx != EGL_NO_CONTEXT);
    EGL_OK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx));   // 不需要任何绘制表面
    printf("EGL %d.%d，OpenGL %s，%s\n", major, minor, (const char*)glGetString(GL_VERSION),
           (const char*)glGetString(GL_RENDERER));
    // ANCHOR_END: egl

    // ANCHOR: register
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, BYTES, nullptr, GL_DYNAMIC_DRAW);   // OpenGL 分配，比如一个顶点缓冲
    CHECK(cudaFree(0));                               // 先建好 CUDA 上下文，不算进注册的时间
    double t0 = now_us();
    cudaGraphicsResource_t res;
    CHECK(cudaGraphicsGLRegisterBuffer(&res, vbo, cudaGraphicsRegisterFlagsWriteDiscard));   // 只注册一次
    printf("注册 %zu MB 的 buffer：%.0f us\n", BYTES >> 20, now_us() - t0);
    // ANCHOR_END: register

    // ANCHOR: frame
    cudaStream_t s;
    CHECK(cudaStreamCreate(&s));
    auto frame = [&](int f) {
        CHECK(cudaGraphicsMapResources(1, &res, s));  // 这段时间里 OpenGL 不能用这个 buffer
        float* p;
        size_t size;
        CHECK(cudaGraphicsResourceGetMappedPointer((void**)&p, &size, res));
        write_frame<<<208, 256, 0, s>>>(p, N, f);
        CHECK(cudaGraphicsUnmapResources(1, &res, s));   // 解除映射以后 OpenGL 才能接着用
    };
    // ANCHOR_END: frame
    frame(0);
    CHECK(cudaStreamSynchronize(s));
    const int frames = 200;
    t0 = now_us();
    for (int f = 1; f <= frames; ++f) frame(f);
    CHECK(cudaStreamSynchronize(s));
    double per_frame = (now_us() - t0) / frames;
    // 对照：同样的 kernel 直接写一块 cudaMalloc 的内存
    float* plain;
    CHECK(cudaMalloc(&plain, BYTES));
    write_frame<<<208, 256, 0, s>>>(plain, N, 0);
    CHECK(cudaStreamSynchronize(s));
    t0 = now_us();
    for (int f = 1; f <= frames; ++f) write_frame<<<208, 256, 0, s>>>(plain, N, f);
    CHECK(cudaStreamSynchronize(s));
    double per_kernel = (now_us() - t0) / frames;
    printf("每帧：映射 + kernel + 解除映射 %.0f us，只有 kernel %.0f us\n", per_frame, per_kernel);

    // ANCHOR: readback
    std::vector<float> h(N);                          // OpenGL 读回最后一帧的内容
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, BYTES, h.data());
    size_t wrong = 0;
    for (size_t i = 0; i < N; ++i) wrong += h[i] != float(frames) + float(i % 1000);
    printf("OpenGL 读回第 %d 帧，错误 %zu 个\n", frames, wrong);
    // ANCHOR_END: readback

    CHECK(cudaFree(plain));
    CHECK(cudaGraphicsUnregisterResource(res));
    glDeleteBuffers(1, &vbo);
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
}

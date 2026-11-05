// shell_probe.c — 32 位探针：直接 dlopen libjunqi.so 跑壳自解密，读 18 地址运行时值
// 编译：armv7a-linux-androideabi21-clang -O0 -shared -fPIC shell_probe.c -o libprobe.so -ldl
// 用法：push libjunqi.so + libprobe.so 到设备 32 位环境，
//   启动宿主程序（32 位 shell app 或 mumu 32 位 shell）：
//     dlopen("/data/local/tmp/libjunqi.so")   // 壳 dlopen → init_array 自解密
//     dlopen("/data/local/tmp/libprobe.so")    // 探针 → read_globals() 读 18 地址
//     read_globals("/data/local/tmp/jni_runtime_values.txt")
//   或直接探针自带 main（32 位可执行体）dlopen 壳 + 读 + 写文件。
//
// 关键：14 JNI 入口读的 18 个全局地址（15 .text 常量池 + 2 .data + 1 共享），
//   静态是密文/占位，运行时（壳 init_array 自解密 + dlopen 重定位）才是真值。
//   探针在壳 dlopen 完成后读这些地址 = 拿到"对齐原版"14 入口需要的注入值。
#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>

// 18 地址 = libjunqi.so 文件/段内偏移（ELF LOAD 段 vaddr==fileoff，运行时 base+off）
static const uint32_t TARGETS[18] = {
    0x233f0,0x23404,0x23420,0x23434,0x23448,0x2345c,
    0x23470,0x23484,0x23498,0x234ac,0x234c0,0x234d4,
    0x235c4,0x235cc,0x23604,0x23608,
    0x15be80,0x15be84
};
#define N 18

// 探针导出：宿主在 dlopen 壳之后调用
// 返回 0 成功；把 18 个运行时值写到 out_path
int jq_probe_read(const char *libjunqi_path, const char *out_path);

int jq_probe_read(const char *libjunqi_path, const char *out_path) {
    // 1. dlopen 壳（触发 .init_array 自解密例程 + dlopen 重定位）
    void *h = dlopen(libjunqi_path, RTLD_NOW);
    if (!h) { fprintf(stderr, "dlopen fail: %s\n", dlerror()); return 1; }

    // 2. 找壳基址（dlopen 返回值是首个符号地址；用 dladdr 更准）
    Dl_info info;
    // 取壳里任意符号定位基址：用 shell_probe 自身在壳里的地址推
    // 更稳：壳加载后其 .text 段 RX 映射 = base + 0（vaddr==fileoff）
    // 直接 dlopen 返回的基址 = 壳 ELF 基址（32 位 PIE 下 base + 段偏移）
    uint32_t base = (uint32_t)h;  // 32 位下 dlopen 返回 base（PIE 基址）

    // 3. 读 18 地址 = base + off
    FILE *f = fopen(out_path, "w");
    if (!f) { fprintf(stderr, "open out fail: %s\n", out_path); return 2; }
    fprintf(f, "// 探针 dump：libjunqi.so 基址=0x%08x，壳已 dlopen+自解密\n", base);
    fprintf(f, "struct jq_runtime_off jq_probe_tbl[N] = {\n");
    int ok = 0;
    for (int i = 0; i < N; i++) {
        uint32_t off = TARGETS[i];
        uint32_t val = *(volatile uint32_t *)(base + off);
        fprintf(f, "  {0x%06x, 0x%08x}, // off=0x%06x\n", off, val, off);
        ok++;
    }
    fprintf(f, "};\n");
    fclose(f);
    // 4. 不 dlclose 壳（保持解密态），宿主继续用
    printf("jq_probe: read %d globals, base=0x%08x\n", ok, base);
    return 0;
}

// 32 位可执行体入口（若编成可执行而非 .so）
#ifndef PROBE_AS_LIB
#include <string.h>
int main(int argc, char **argv) {
    const char *lib = argv[1] ? argv[1] : "/data/local/tmp/libjunqi.so";
    const char *out = argv[2] ? argv[2] : "/data/local/tmp/jni_runtime_values.c";
    // 确保 dlopen 先跑壳 init_array
    return jq_probe_read(lib, out) ? 1 : 0;
}
#endif

/*
 * gl2_probe_gl.c — GLES2/GLSL 几何出帧能力探测（编进 libjunqi_rs.so，app 进程内跑）
 * 目的：MuMu 64 位 GLES1 固定管线几何 no-op（已证），但宿主有 libvulkan_enc / libGLESv2_enc
 *   桥接 Intel GPU。本文件验证"独立建 GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）
 *   + GLSL shader + VBO + glVertexAttribPointer"能否真正出几何像素。
 * 返回 1 = GL2 几何出帧可用（可把 render.c 改 GLES2），0 = 不可用。
 * 诚实：这是探测，非正式渲染；正式改 render.c 走 GLES2 后本文件可删。
 */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"junqi_rs",__VA_ARGS__)

int rs_gl2_probe(void)
{
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (d == EGL_NO_DISPLAY) { LOGI("GL2探测：display 无效"); return 0; }
    EGLint maj=0, min=0;
    if (!eglInitialize(d, &maj, &min)) { LOGI("GL2探测：eglInitialize 失败"); return 0; }
    LOGI("GL2探测：EGL %d.%d", maj, min);

    EGLint cfga[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                      EGL_RED_SIZE,8, EGL_GREEN_SIZE,8, EGL_BLUE_SIZE,8,
                      EGL_ALPHA_SIZE,8, EGL_DEPTH_SIZE,16, EGL_NONE };
    EGLConfig cfg; EGLint n=0;
    if (!eglChooseConfig(d, cfga, &cfg, 1, &n) || !n) { LOGI("GL2探测：PBuffer config 不可用"); return 0; }

    /* 关键：GL2 context（client version=2），不是 GL1 context 调 GL2 函数 */
    EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext c = eglCreateContext(d, cfg, EGL_NO_CONTEXT, ca);
    if (c == EGL_NO_CONTEXT) { LOGI("GL2探测：GL2 context 创建失败 err=0x%x", eglGetError()); return 0; }
    LOGI("GL2探测：GL2 context 创建成功");

    EGLint psa[] = { EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
    EGLSurface s = eglCreatePbufferSurface(d, cfg, psa);
    if (s == EGL_NO_SURFACE) { LOGI("GL2探测：PBuffer surface 失败"); return 0; }
    if (!eglMakeCurrent(d, s, s, c)) { LOGI("GL2探测：makeCurrent 失败 err=0x%x", eglGetError()); return 0; }

    LOGI("GL2探测：GL_VERSION=%s", (const char*)glGetString(GL_VERSION));
    LOGI("GL2探测：GL_RENDERER=%s", (const char*)glGetString(GL_RENDERER));

    const char *vs = "attribute vec2 aPos; void main(){ gl_Position=vec4(aPos,0.0,1.0); }";
    const char *fs = "precision mediump float; void main(){ gl_FragColor=vec4(1.0,0.0,0.0,1.0); }";
    GLuint vs_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_, 1, &vs, NULL); glCompileShader(vs_);
    GLint ok=0; glGetShaderiv(vs_, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[256]; glGetShaderInfoLog(vs_,256,NULL,log); LOGI("GL2探测：VS 编译失败 %s", log); return 0; }
    GLuint fs_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_, 1, &fs, NULL); glCompileShader(fs_);
    glGetShaderiv(fs_, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[256]; glGetShaderInfoLog(fs_,256,NULL,log); LOGI("GL2探测：FS 编译失败 %s", log); return 0; }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs_); glAttachShader(prog, fs_); glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[256]; glGetProgramInfoLog(prog,256,NULL,log); LOGI("GL2探测：link 失败 %s", log); return 0; }
    glUseProgram(prog);

    float tri[] = { -1.0f,-1.0f,  3.0f,-1.0f,  -1.0f,3.0f };
    GLuint vbo; glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    GLuint loc = glGetAttribLocation(prog, "aPos");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glViewport(0, 0, 64, 64);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFinish();

    unsigned char px[4] = {0};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    LOGI("GL2探测：中心像素 RGBA=(%d,%d,%d,%d)（期望 255,0,0,255 纯红）", px[0],px[1],px[2],px[3]);
    int out = (px[0] > 200 && px[1] < 40 && px[2] < 40);
    LOGI("GL2+GLSL 几何出帧 = %s", out ? "成功（可改 render.c 走 GLES2）" : "失败（MuMu GL2 也不出像素）");

    glDeleteProgram(prog); glDeleteShader(vs_); glDeleteShader(fs_);
    glDeleteBuffers(1, &vbo);
    eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(d, c);
    eglDestroySurface(d, s);
    eglTerminate(d);
    return out;
}

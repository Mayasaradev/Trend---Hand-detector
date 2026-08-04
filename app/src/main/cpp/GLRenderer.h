// GLRenderer.h
#pragma once
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <array>

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();
    void init(ANativeWindow* window);
    void destroy();
    void draw();
    ANativeWindow* getSurfaceTexture(); // return surface untuk kamera
    void setFingertips(const float* tips, int count);

private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    GLuint programBackground_ = 0;
    GLuint programEffect_ = 0;
    GLuint quadVAO_ = 0;
    GLuint cameraTexture_ = 0;
    GLuint surfaceTextureId_ = 0;
    ASurfaceTexture* surfaceTexture_ = nullptr;
    ANativeWindow* window_ = nullptr;

    std::array<float, 10> fingertipCoords_; // 5 * 2 dinormalisasi 0-1
    int numFingers_ = 0;
    int screenWidth_ = 0, screenHeight_ = 0;

    void createQuad();
    void loadShaders();
    GLuint compileShader(GLenum type, const char* source);
};

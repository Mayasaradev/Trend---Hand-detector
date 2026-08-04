// GLRenderer.cpp
#include "GLRenderer.h"
#include "Shaders.h"
#include <android/surface_texture.h>
#include <android/surface_texture_jni.h>
#include <cassert>
#include <cmath>

GLRenderer::GLRenderer() {}
GLRenderer::~GLRenderer() { destroy(); }

void GLRenderer::init(ANativeWindow* window) {
    window_ = window;
    screenWidth_ = ANativeWindow_getWidth(window);
    screenHeight_ = ANativeWindow_getHeight(window);

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display_, nullptr, nullptr);
    EGLint attribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                         EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                         EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                         EGL_NONE };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display_, attribs, &config, 1, &numConfigs);
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context_ = eglCreateContext(display_, config, nullptr, contextAttribs);
    surface_ = eglCreateWindowSurface(display_, config, window_, nullptr);
    eglMakeCurrent(display_, surface_, surface_, context_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    createQuad();
    loadShaders();

    // Buat SurfaceTexture untuk kamera
    glGenTextures(1, &cameraTexture_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, cameraTexture_);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    surfaceTexture_ = ASurfaceTexture_createFromGLTexture(cameraTexture_);
    surfaceTextureId_ = cameraTexture_;
}

void GLRenderer::destroy() {
    if (surfaceTexture_) ASurfaceTexture_release(surfaceTexture_);
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display_, context_);
    eglDestroySurface(display_, surface_);
    eglTerminate(display_);
}

ANativeWindow* GLRenderer::getSurfaceTexture() {
    return ASurfaceTexture_acquireANativeWindow(surfaceTexture_);
}

void GLRenderer::setFingertips(const float* tips, int count) {
    numFingers_ = count;
    for (int i=0; i<count*2; ++i) fingertipCoords_[i] = tips[i];
}

void GLRenderer::draw() {
    ASurfaceTexture_updateTexImage(surfaceTexture_);
    glClear(GL_COLOR_BUFFER_BIT);

    // 1. Render background dengan kamera
    glUseProgram(programBackground_);
    glBindVertexArray(quadVAO_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, cameraTexture_);
    glUniform1i(glGetUniformLocation(programBackground_, "u_Texture"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Jika tangan terdeteksi, render efek distorsi
    if (numFingers_ == 5) {
        glUseProgram(programEffect_);
        glUniform1i(glGetUniformLocation(programEffect_, "u_Texture"), 0);
        glUniform2fv(glGetUniformLocation(programEffect_, "u_Fingertips"), 5, fingertipCoords_.data());
        glUniform1i(glGetUniformLocation(programEffect_, "u_NumFingers"), 5);
        glUniform2f(glGetUniformLocation(programEffect_, "u_ScreenSize"), (float)screenWidth_, (float)screenHeight_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    eglSwapBuffers(display_, surface_);
}

void GLRenderer::createQuad() {
    float quad[] = {
        -1, -1, 0, 1,
         1, -1, 0, 1,
        -1,  1, 0, 1,
         1, -1, 0, 1,
         1,  1, 0, 1,
        -1,  1, 0, 1
    };
    GLuint vbo;
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &vbo);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void GLRenderer::loadShaders() {
    programBackground_ = glCreateProgram();
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, backgroundFragmentSrc);
    glAttachShader(programBackground_, vs); glAttachShader(programBackground_, fs);
    glLinkProgram(programBackground_);

    programEffect_ = glCreateProgram();
    GLuint vs2 = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs2 = compileShader(GL_FRAGMENT_SHADER, effectFragmentSrc);
    glAttachShader(programEffect_, vs2); glAttachShader(programEffect_, fs2);
    glLinkProgram(programEffect_);
}

GLuint GLRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

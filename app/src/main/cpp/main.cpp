#include <android_native_app_glue.h>
#include <android/sensor.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include "CameraManager.h"
#include "TFLiteInference.h"
#include "GLRenderer.h"
#include "Shaders.h"

struct AppState {
    android_app* app;
    std::unique_ptr<CameraManager> camera;
    std::unique_ptr<TFLiteInference> inference;
    std::unique_ptr<GLRenderer> renderer;

    // Data landmark hasil deteksi
    float handLandmarks[21 * 2] = {0}; // 21 titik, x,y normalised 0-1
    bool handDetected = false;
    std::mutex landmarkMutex;
};

static void handleCmd(struct android_app* app, int32_t cmd) {
    auto* state = (AppState*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                state->renderer->init(app->window);
                state->camera->start(state->renderer->getSurfaceTexture());
            }
            break;
        case APP_CMD_TERM_WINDOW:
            state->renderer->destroy();
            state->camera->stop();
            break;
    }
}

void android_main(struct android_app* app) {
    AppState state;
    state.app = app;
    app->userData = &state;
    app->onAppCmd = handleCmd;

    state.camera = std::make_unique<CameraManager>(app);
    state.inference = std::make_unique<TFLiteInference>(app->activity->assetManager);
    state.renderer = std::make_unique<GLRenderer>();

    // Thread inference
    std::thread inferThread([&state]() {
        while (!state.app->destroyRequested) {
            auto image = state.camera->getLatestImage();
            if (image) {
                bool handFound = false;
                std::vector<float> landmarks(21 * 2);
                // Proses deteksi + landmark
                handFound = state.inference->process(image, landmarks.data());
                if (handFound) {
                    std::lock_guard<std::mutex> lock(state.landmarkMutex);
                    state.handDetected = true;
                    std::copy(landmarks.begin(), landmarks.end(), state.handLandmarks);
                } else {
                    std::lock_guard<std::mutex> lock(state.landmarkMutex);
                    state.handDetected = false;
                }
            }
        }
    });

    while (!app->destroyRequested) {
        int ident;
        int events;
        struct android_poll_source* source;
        while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
        }

        // Update renderer dengan landmark terbaru
        float fingertips[5 * 2];
        int numFingers = 0;
        {
            std::lock_guard<std::mutex> lock(state.landmarkMutex);
            if (state.handDetected) {
                // Ambil ujung jari: indeks 4,8,12,16,20
                const int tips[] = {4, 8, 12, 16, 20};
                for (int i = 0; i < 5; ++i) {
                    fingertips[2*i] = state.handLandmarks[2*tips[i]];
                    fingertips[2*i+1] = state.handLandmarks[2*tips[i]+1];
                }
                numFingers = 5;
            }
        }
        state.renderer->setFingertips(fingertips, numFingers);
        state.renderer->draw();
    }

    inferThread.join();
}

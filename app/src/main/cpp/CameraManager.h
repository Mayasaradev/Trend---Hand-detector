#pragma once
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>   // <-- tambah
#include <mutex>                       // <-- tambah
#include <condition_variable>          // <-- tambah
#include <vector>
#include <memory>

class CameraManager {
public:
    explicit CameraManager(android_app* app);
    ~CameraManager();
    void start(ANativeWindow* textureWindow);
    void stop();
    AImage* getLatestImage(); // blocking atau non-blocking, pilih sesuai

private:
    void openCamera();
    void createSession(ANativeWindow* window);
    static void imageCallback(void* context, AImageReader* reader);

    android_app* app_;
    ACameraManager* cameraManager_ = nullptr;
    ACameraDevice* cameraDevice_ = nullptr;
    ACaptureSessionOutputContainer* outputs_ = nullptr;
    ACaptureSessionOutput* sessionOutput_ = nullptr;
    ACameraCaptureSession* captureSession_ = nullptr;
    AImageReader* imageReader_ = nullptr;
    ANativeWindow* textureWindow_ = nullptr;
    std::mutex imageMutex_;
    std::vector<AImage*> imageQueue_;
    std::condition_variable imageCv_;
};

// CameraManager.cpp
#include "CameraManager.h"
#include <android/log.h>
#include <cassert>

#define TAG "CameraManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

CameraManager::CameraManager(android_app* app) : app_(app) {
    cameraManager_ = ACameraManager_create();
}

CameraManager::~CameraManager() {
    stop();
    if (cameraManager_) ACameraManager_delete(cameraManager_);
}

void CameraManager::start(ANativeWindow* window) {
    textureWindow_ = window;
    openCamera();
}

void CameraManager::stop() {
    if (captureSession_) {
        ACameraCaptureSession_stopRepeating(captureSession_);
        ACameraCaptureSession_close(captureSession_);
        captureSession_ = nullptr;
    }
    if (cameraDevice_) {
        ACameraDevice_close(cameraDevice_);
        cameraDevice_ = nullptr;
    }
    if (imageReader_) {
        AImageReader_delete(imageReader_);
        imageReader_ = nullptr;
    }
}

void CameraManager::openCamera() {
    ACameraIdList* cameraIds = nullptr;
    ACameraManager_getCameraIdList(cameraManager_, &cameraIds);
    const char* backCameraId = nullptr;
    for (int i = 0; i < cameraIds->numCameras; ++i) {
        const char* id = cameraIds->cameraIds[i];
        ACameraMetadata* metadata;
        ACameraManager_getCameraCharacteristics(cameraManager_, id, &metadata);
        ACameraMetadata_const_entry entry;
        ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry);
        if (entry.data.u8[0] == ACAMERA_LENS_FACING_BACK) {
            backCameraId = id;
            break;
        }
    }
    assert(backCameraId);

    ACameraManager_openCamera(cameraManager_, backCameraId, nullptr, &cameraDevice_);
    ACameraIdList_delete(cameraIds);

    // ImageReader untuk inferensi
    media_status_t status = AImageReader_new(640, 480, AIMAGE_FORMAT_YUV_420_888, 2, &imageReader_);
    AImageReader_setImageListener(imageReader_, new AImageReader_ImageListener{this, imageCallback});

    createSession(textureWindow_);
}

void CameraManager::createSession(ANativeWindow* window) {
    ACaptureSessionOutputContainer_create(&outputs_);
    ACaptureSessionOutput_create(window, &sessionOutput_);
    ACaptureSessionOutputContainer_add(outputs_, sessionOutput_);

    ANativeWindow* imageWindow;
    AImageReader_getWindow(imageReader_, &imageWindow);
    ACaptureSessionOutput* imageOut;
    ACaptureSessionOutput_create(imageWindow, &imageOut);
    ACaptureSessionOutputContainer_add(outputs_, imageOut);

    ACameraDevice_createCaptureSession(cameraDevice_, outputs_, nullptr, &captureSession_);

    ACaptureRequest* request;
    ACameraDevice_createCaptureRequest(cameraDevice_, TEMPLATE_PREVIEW, &request);
    ACaptureRequest_addTarget(request, sessionOutput_);
    ACaptureRequest_addTarget(request, imageOut);

    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &request, nullptr);
    ACaptureRequest_free(request);
}

void CameraManager::imageCallback(void* context, AImageReader* reader) {
    auto* self = (CameraManager*)context;
    AImage* image;
    media_status_t status = AImageReader_acquireNextImage(reader, &image);
    if (status == AMEDIA_OK) {
        std::lock_guard<std::mutex> lock(self->imageMutex_);
        self->imageQueue_.push_back(image);
        self->imageCv_.notify_one();
    }
}

AImage* CameraManager::getLatestImage() {
    std::unique_lock<std::mutex> lock(imageMutex_);
    imageCv_.wait(lock, [this]{ return !imageQueue_.empty(); });
    AImage* img = imageQueue_.front();
    imageQueue_.erase(imageQueue_.begin());
    return img;
}

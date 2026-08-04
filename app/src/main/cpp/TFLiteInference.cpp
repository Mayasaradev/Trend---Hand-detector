// TFLiteInference.cpp
#include "TFLiteInference.h"
#include <android/log.h>
#include <media/NdkImage.h>
#include <cstring>

#define TAG "TFLite"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

TFLiteInference::TFLiteInference(AAssetManager* mgr) : assetMgr_(mgr) {
    // Load palm detection model
    AAsset* palmAsset = AAssetManager_open(assetMgr_, "palm_detection_full.tflite", AASSET_MODE_BUFFER);
    size_t palmSize = AAsset_getLength(palmAsset);
    const void* palmData = AAsset_getBuffer(palmAsset);
    palmModel_ = tflite::FlatBufferModel::BuildFromBuffer((const char*)palmData, palmSize);
    AAsset_close(palmAsset);

    // Load hand landmark model
    AAsset* lmAsset = AAssetManager_open(assetMgr_, "hand_landmark_full.tflite", AASSET_MODE_BUFFER);
    size_t lmSize = AAsset_getLength(lmAsset);
    const void* lmData = AAsset_getBuffer(lmAsset);
    landmarkModel_ = tflite::FlatBufferModel::BuildFromBuffer((const char*)lmData, lmSize);
    AAsset_close(lmAsset);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder palmBuilder(*palmModel_, resolver);
    palmBuilder(&palmInterpreter_);
    palmInterpreter_->AllocateTensors();

    tflite::InterpreterBuilder lmBuilder(*landmarkModel_, resolver);
    lmBuilder(&landmarkInterpreter_);
    landmarkInterpreter_->AllocateTensors();

    // GPU delegate (optional)
    // gpuDelegate_ = TfLiteGpuDelegateV2Create(/* options */);
    // palmInterpreter_->ModifyGraphWithDelegate(gpuDelegate_);
    // landmarkInterpreter_->ModifyGraphWithDelegate(gpuDelegate_);
}

TFLiteInference::~TFLiteInference() {
    if (gpuDelegate_) TfLiteGpuDelegateV2Delete(gpuDelegate_);
}

bool TFLiteInference::process(AImage* yuvImage, float* landmarksOut) {
    // 1. Konversi YUV ke RGB 256x256 untuk palm detection
    int32_t srcW, srcH;
    AImage_getWidth(yuvImage, &srcW);
    AImage_getHeight(yuvImage, &srcH);
    const int palmInputSize = 256;
    uint8_t rgbPalm[palmInputSize * palmInputSize * 3];
    yuvToRgb(yuvImage, rgbPalm, srcW, srcH); // Perlu resize ke 256x256 di dalam fungsi

    // 2. Palm detection inference
    auto& palmInput = palmInterpreter_->typed_tensor<float>(palmInterpreter_->inputs()[0]);
    // Normalisasi dari [0,255] ke [-1,1]? Sesuaikan dengan model
    for (int i=0; i < palmInputSize*palmInputSize*3; ++i)
        palmInput[i] = (rgbPalm[i] / 127.5f) - 1.0f;
    palmInterpreter_->Invoke();

    // 3. Ambil deteksi (contoh: 1 deteksi, box [ymin,xmin,ymax,xmax] dinormalisasi)
    const float* detection = palmInterpreter_->typed_output_tensor<float>(0);
    float score = detection[4+1]; // format tergantung model, sesuaikan
    if (score < 0.5f) return false;

    float xmin = detection[1] * srcW;
    float ymin = detection[0] * srcH;
    float xmax = detection[3] * srcW;
    float ymax = detection[2] * srcH;
    float handW = xmax - xmin;
    float handH = ymax - ymin;
    // Perbesar sedikit
    float margin = 0.2f;
    xmin -= margin * handW; ymin -= margin * handH;
    xmax += margin * handW; ymax += margin * handH;
    xmin = std::max(0.0f, xmin); ymin = std::max(0.0f, ymin);
    xmax = std::min((float)srcW, xmax); ymax = std::min((float)srcH, ymax);

    // 4. Crop dan resize region tangan ke 224x224 RGB untuk landmark
    const int lmInputSize = 224;
    uint8_t rgbHand[lmInputSize * lmInputSize * 3];
    // Crop & resize dari yuvImage (harus diimplementasikan, bisa pakai libyuv)
    // Asumsi sederhana: kita resize full frame dulu lalu crop (tidak optimal, tapi ok)
    // Placeholder, untuk fokus ke alur. Implementasi nyata pakai libyuv.

    // 5. Landmark inference
    auto& lmInput = landmarkInterpreter_->typed_tensor<float>(landmarkInterpreter_->inputs()[0]);
    for (int i=0; i < lmInputSize*lmInputSize*3; ++i)
        lmInput[i] = (rgbHand[i] / 127.5f) - 1.0f;
    landmarkInterpreter_->Invoke();
    const float* lmOutput = landmarkInterpreter_->typed_output_tensor<float>(0); // shape [1,63] x,y,z per titik

    // 6. Transform ke koordinat full frame, lalu normalisasi 0-1
    for (int i=0; i<21; ++i) {
        float x = lmOutput[3*i] / lmInputSize * (xmax - xmin) + xmin;
        float y = lmOutput[3*i+1] / lmInputSize * (ymax - ymin) + ymin;
        landmarksOut[2*i] = x / srcW;
        landmarksOut[2*i+1] = y / srcH;
    }
    return true;
}

void TFLiteInference::yuvToRgb(AImage* yuv, uint8_t* rgbOut, int dstW, int dstH) {
    // Implementasi menggunakan libyuv atau konversi sederhana.
    // Di sini hanya placeholder.
    // Gunakan libyuv::NV12ToRGB24 atau I420ToRGB24.
}

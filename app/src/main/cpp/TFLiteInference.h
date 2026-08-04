// TFLiteInference.h
#pragma once
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/delegates/gpu/delegate.h>
#include <android/asset_manager.h>
#include <vector>
#include <cstdint>

class TFLiteInference {
public:
    TFLiteInference(AAssetManager* mgr);
    ~TFLiteInference();
    // Proses frame YUV_420_888 dan kembalikan true + landmark jika tangan terdeteksi
    bool process(AImage* yuvImage, float* landmarks21x2);

private:
    std::unique_ptr<tflite::FlatBufferModel> palmModel_;
    std::unique_ptr<tflite::FlatBufferModel> landmarkModel_;
    std::unique_ptr<tflite::Interpreter> palmInterpreter_;
    std::unique_ptr<tflite::Interpreter> landmarkInterpreter_;
    TfLiteDelegate* gpuDelegate_ = nullptr;
    AAssetManager* assetMgr_;
    // konversi YUV → RGB
    void yuvToRgb(AImage* yuv, uint8_t* rgbOut, int width, int height);
};

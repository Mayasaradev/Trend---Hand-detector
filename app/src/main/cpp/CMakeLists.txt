cmake_minimum_required(VERSION 3.18)
project("handdistort")

add_library(handdistort SHARED
    main.cpp
    CameraManager.cpp
    TFLiteInference.cpp
    GLRenderer.cpp
)

target_include_directories(handdistort PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(android-lib android)
find_library(log-lib log)
find_library(camera2ndk-lib camera2ndk)
find_library(mediandk-lib mediandk)
find_library(GLESv3-lib GLESv3)
find_library(EGL-lib EGL)

# TensorFlow Lite prebuilt (disediakan oleh dependency implementation)
# Library dari AAR akan tersedia di build/cmake
set(TFLITE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../../tensorflow-lite/headers)
set(TFLITE_LIB_DIR ${CMAKE_SOURCE_DIR}/../../../tensorflow-lite/jni)

# Tambahkan path ke tflite
target_include_directories(handdistort PRIVATE ${TFLITE_INCLUDE_DIR})
target_link_directories(handdistort PRIVATE ${TFLITE_LIB_DIR})

target_link_libraries(handdistort
    ${android-lib}
    ${log-lib}
    ${camera2ndk-lib}
    ${mediandk-lib}
    ${GLESv3-lib}
    ${EGL-lib}
    tensorflowlite
    tensorflowlite_gpu_delegate
    -fopenmp
)

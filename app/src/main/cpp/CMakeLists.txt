cmake_minimum_required(VERSION 3.18)
project("handdistort")

add_library(handdistort SHARED
    main.cpp
    CameraManager.cpp
    TFLiteInference.cpp
    GLRenderer.cpp
)

# Include direktori sumber sendiri
target_include_directories(handdistort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Cari library Android
find_library(android-lib android)
find_library(log-lib log)
find_library(camera2ndk-lib camera2ndk)
find_library(mediandk-lib mediandk)
find_library(GLESv3-lib GLESv3)
find_library(EGL-lib EGL)
find_library(native_app_glue-lib native_app_glue)

# Cari TensorFlow Lite melalui Prefab (nama pakai tanda hubung)
find_package(tensorflow-lite REQUIRED CONFIG)

target_link_libraries(handdistort
    ${android-lib}
    ${log-lib}
    ${camera2ndk-lib}
    ${mediandk-lib}
    ${GLESv3-lib}
    ${EGL-lib}
    ${native_app_glue-lib}
    tensorflow-lite::tensorflow-lite
    tensorflow-lite-gpu::tensorflow-lite-gpu
    -fopenmp
)
cmake_minimum_required(VERSION 3.18)
project("handdistort")

add_library(handdistort SHARED
    main.cpp
    CameraManager.cpp
    TFLiteInference.cpp
    GLRenderer.cpp
)

target_include_directories(handdistort PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Path ke header TFLite yang sudah diekstrak
set(TFLITE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/../../../../build/tflite/headers)
target_include_directories(handdistort PRIVATE ${TFLITE_INCLUDE_DIR})

# Library sistem
find_library(android-lib android)
find_library(log-lib log)
find_library(camera2ndk-lib camera2ndk)
find_library(mediandk-lib mediandk)
find_library(GLESv3-lib GLESv3)
find_library(EGL-lib EGL)
find_library(native_app_glue-lib native_app_glue)

# Library TFLite dari hasil ekstrak
set(TFLITE_LIB_DIR ${CMAKE_BINARY_DIR}/../../../../build/tflite/jni/arm64-v8a)
add_library(tensorflowlite SHARED IMPORTED)
set_target_properties(tensorflowlite PROPERTIES
    IMPORTED_LOCATION ${TFLITE_LIB_DIR}/libtensorflowlite.so
)
add_library(tensorflowlite_gpu SHARED IMPORTED)
set_target_properties(tensorflowlite_gpu PROPERTIES
    IMPORTED_LOCATION ${TFLITE_LIB_DIR}/libtensorflowlite_gpu.so
)

target_link_libraries(handdistort
    ${android-lib}
    ${log-lib}
    ${camera2ndk-lib}
    ${mediandk-lib}
    ${GLESv3-lib}
    ${EGL-lib}
    ${native_app_glue-lib}
    tensorflowlite
    tensorflowlite_gpu
    -fopenmp
)
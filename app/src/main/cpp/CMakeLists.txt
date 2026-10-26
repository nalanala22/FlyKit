cmake_minimum_required(VERSION 3.22.1)
project(flykit LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(flykit SHARED
  jni_bridge.cpp core/features.cpp hook/sigscan.cpp hook/hooks.cpp hook/probe.cpp hook/elfinfo.cpp)
target_link_libraries(flykit log)

# Opsional: backend hook Dobby. Isi dengan path libdobby.a untuk ABI yang dibangun:
#   -DFK_DOBBY_LIB=/path/arm64-v8a/libdobby.a
set(FK_DOBBY_LIB "" CACHE FILEPATH "Path libdobby.a")
if(FK_DOBBY_LIB)
  target_compile_definitions(flykit PRIVATE FK_USE_DOBBY)
  target_link_libraries(flykit ${FK_DOBBY_LIB})
endif()

cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "PancakeECS")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

# Flags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g")

set(SOURCE_FILES src/ECSManager.cpp src/ECSManager.h
        src/ComponentView.h src/ComponentData.h
        src/GameObject.cpp src/GameObject.h
        src/TypeId.h src/TypeId.cpp
        src/Scene.cpp src/Scene.h
        src/ComponentViewBase.h   src/ComponentViewBase.cpp
        main.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
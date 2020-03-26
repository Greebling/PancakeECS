cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "PancakeECS")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)


set(SOURCE_FILES src/ECSManager.cpp src/ECSManager.h
        src/ComponentView.h src/ComponentData.h
        src/GameObject.h
        src/TypeId.h
        src/Scene.cpp src/Scene.h
        src/ComponentViewBase.h
        src/Entity.h)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
#add_executable(${PROJECT_NAME} ${SOURCE_FILES})
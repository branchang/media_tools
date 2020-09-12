cmake_minimum_required(VERSION 2.6)
project(media_tools)

# 添加字幕文件
option(USE_ADD_SUBTITLE "use add subtitle" OFF)

SET(CMAKE_C_COMPILER "/usr/local/Cellar/llvm/10.0.0_3/bin/clang")
SET(CMAKE_CXX_COMPILER "/usr/local/Cellar/llvm/10.0.0_3/bin/clang++")

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_MACOSX_RPATH 1)
SET(CMAKE_VERBOSE_MAKEFILE on)


SET(SRC_LIST main.cpp)

INCLUDE_DIRECTORIES(
    /usr/local/include
)

if(USE_ADD_SUBTITLE)
    AUX_SOURCE_DIRECTORY(subtitle SUBTITLE_SRC_LIST)
    message(${SUBTITLE_SRC_LIST})
    list(APPEND SRC_LIST ${SUBTITLE_SRC_LIST})
    INCLUDE_DIRECTORIES(AFTER subtitle)
    add_definitions(-DADD_SUBTITLE)
endif()

SET(LIB_LIST pthread avutil swscale avformat swresample avformat avcodec avfilter)

LINK_DIRECTORIES(/usr/local/lib/ )

ADD_EXECUTABLE(media_tool ${SRC_LIST})
TARGET_LINK_LIBRARIES(media_tool ${LIB_LIST})

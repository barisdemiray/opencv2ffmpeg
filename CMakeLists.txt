
cmake_minimum_required(VERSION 3.0)

set(EXE OpenCV2FFmpeg)

add_executable(${EXE} main.cpp)
target_link_libraries(${EXE} opencv_imgproc opencv_highgui opencv_core swscale avutil avcodec avformat)


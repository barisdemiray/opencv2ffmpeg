# Intro
A sample code to grab frames from a video using OpenCV, and then to create a video by encoding these frames in H.264 using FFmpeg.

# Build

Steps for an out-of-source build.

    mkdir build
    cd build
    cmake ..
    make

# Dependencies

This code is dependent on, well, OpenCV and FFmpeg. Especially FFmpeg API seems to change a bit too often. Versions I used are,

- libavutil      54. 27.100
- libavcodec     56. 41.100
- libavformat    56. 36.100
- libavdevice    56.  4.100
- libavfilter     5. 16.101
- libavresample   2.  1.  0
- libswscale      3.  1.101
- libswresample   1.  2.100
- libpostproc    53.  3.100

And OpenCV 2.4.9.


//============================================================================
// Name        : OpenCV2FFmpeg.cpp
// Author      : Baris Demiray
// Version     : 1.0.0
// Copyright   : GPL
// Description : Sample application for cv::Mat to AVFrame conversion
//============================================================================

#include <iostream>
#include <cassert>
#include <glob.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
}

/**
 * Pixel formats and codecs
 */
static const AVPixelFormat sourcePixelFormat = AV_PIX_FMT_BGR24;
static const AVPixelFormat destPixelFormat = AV_PIX_FMT_YUV420P;
static const AVCodecID destCodec = AV_CODEC_ID_H264;


int main(int argc, char** argv)
{
    /**
     * Verify command line arguments
     */
    if (argc != 4) {
        std::cout << "This tool grabs <numberOfFramesToEncode> frames from <input> and encodes a H.264 video with these at <output>" << std::endl;
        std::cout << "Usage: " << argv[0] << " <input> <output> <numberOfFramesToEncode>" << std::endl;
        std::cout << "Sample: " << argv[0] << "sample.mpg sample.out 250" << std::endl;
        exit(1);
    }

    std::string input(argv[1]), output(argv[2]);
    uint32_t framesToEncode;
    std::istringstream(argv[3]) >> framesToEncode;

    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    /**
     * Create cv::Mat
     */
    cv::VideoCapture videoCapturer(input);
    if (videoCapturer.isOpened() == false) {
        std::cerr << "Cannot open video at '" << input << "'" << std::endl;
        exit(1);
    }

    /**
     * Get some information of the video and print them
     */
    double totalFrameCount = videoCapturer.get(CV_CAP_PROP_FRAME_COUNT);
    uint width = videoCapturer.get(CV_CAP_PROP_FRAME_WIDTH),
         height = videoCapturer.get(CV_CAP_PROP_FRAME_HEIGHT);

    std::cout << input << "[Width:" << width << ", Height=" << height
              << ", FPS: " << videoCapturer.get(CV_CAP_PROP_FPS)
              << ", FrameCount: " << totalFrameCount << "]" << std::endl;

    /**
     * Be sure we're not asking more frames than there is
     */
    if (framesToEncode > totalFrameCount) {
        std::cerr << "You asked for " << framesToEncode << " but there are only " << totalFrameCount
                  << " frames, will encode as many as there is" << std::endl;
        framesToEncode = totalFrameCount;
    }

    /**
     * Create an encoder and open it
     */
    avcodec_register_all();

    AVCodec *h264encoder = avcodec_find_encoder(destCodec);
    AVCodecContext *h264encoderContext = avcodec_alloc_context3(h264encoder);

    h264encoderContext->pix_fmt = destPixelFormat;
    h264encoderContext->width = width;
    h264encoderContext->height = height;

    if (avcodec_open2(h264encoderContext, h264encoder, NULL) < 0) {
        std::cerr << "Cannot open codec, exiting.." << std::endl;
        exit(1);
    }

    /**
     * Create a stream
     */
    AVFormatContext *cv2avFormatContext = avformat_alloc_context();
    AVStream *h264outputstream = avformat_new_stream(cv2avFormatContext, h264encoder);

    AVFrame *sourceAvFrame = av_frame_alloc(), *destAvFrame = av_frame_alloc();
    int got_frame;

    FILE* videoOutFile = fopen(output.c_str(), "wb");
    if (videoOutFile == 0) {
        std::cerr << "Cannot open output video file at '" << output << "'" << std::endl;
        exit(1);
    }

    /**
     * Prepare the conversion context
     */
    SwsContext *bgr2yuvcontext = sws_getContext(width, height, sourcePixelFormat,
                                                width, height, destPixelFormat,
                                                SWS_BICUBIC, NULL, NULL, NULL);
    /**
     * Convert and encode frames
     */
    for (uint i=0; i < framesToEncode; i++) {
        /**
         * Get frame from OpenCV
         */
        cv::Mat cvFrame;
        assert(videoCapturer.read(cvFrame) == true);

        /**
         * Allocate source frame, i.e. input to sws_scale()
         */
        av_image_alloc(sourceAvFrame->data, sourceAvFrame->linesize, width, height, sourcePixelFormat, 1);

        /**
         * Copy image data into AVFrame from cv::Mat
         */
        for (uint32_t h = 0; h < height; h++)
            memcpy(&(sourceAvFrame->data[0][h*sourceAvFrame->linesize[0]]), &(cvFrame.data[h*cvFrame.step]), width*3);

        /**
         * Allocate destination frame, i.e. output from sws_scale()
         */
        av_image_alloc(destAvFrame->data, destAvFrame->linesize, width, height, destPixelFormat, 1);

        sws_scale(bgr2yuvcontext, sourceAvFrame->data, sourceAvFrame->linesize,
                  0, height, destAvFrame->data, destAvFrame->linesize);

        /**
         * Prepare an AVPacket and set buffer to NULL so that it'll be allocated by FFmpeg
         */
        AVPacket avEncodedPacket;
        av_init_packet(&avEncodedPacket);
        avEncodedPacket.data = NULL;
        avEncodedPacket.size = 0;

        destAvFrame->pts = i;
        avcodec_encode_video2(h264encoderContext, &avEncodedPacket, destAvFrame, &got_frame);

        if (got_frame) {
            std::cerr << "Encoded a frame of size " << avEncodedPacket.size << ", writing it.." << std::endl;

            if (fwrite(avEncodedPacket.data, 1, avEncodedPacket.size, videoOutFile) < (unsigned)avEncodedPacket.size)
                std::cerr << "Could not write all " << avEncodedPacket.size << " bytes, but will continue.." << std::endl;

            fflush(videoOutFile);
        }

        av_free_packet(&avEncodedPacket);
        av_freep(sourceAvFrame->data);
        av_freep(destAvFrame->data);
    }

    fwrite(endcode, 1, sizeof(endcode), videoOutFile);
    fclose(videoOutFile);
}

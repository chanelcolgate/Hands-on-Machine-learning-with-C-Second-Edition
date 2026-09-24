#include <iostream>
#include <opencv2/opencv.hpp>

#include "drivers/ffmpeg_rtsp_reader.hpp"

int main() {
    const std::string url =
        "rtsp://admin:123456@192.168.1.60:554/0";

    FFmpegRtspReader reader;

    if (!reader.open(url)) {
        std::cerr << "Cannot open RTSP stream\n";
        return 1;
    }

    cv::Mat frame;

    for (int i = 0; i < 100; ++i) {
        if (!reader.read(frame)) {
            std::cerr << "Cannot read frame " << i << '\n';
            return 2;
        }

        std::cout << "Frame " << i
                  << ": "
                  << frame.cols << "x"
                  << frame.rows << '\n';

        cv::imwrite("frame_" + std::to_string(i) + ".jpg", frame);
    }

    reader.close();
    return 0;
}

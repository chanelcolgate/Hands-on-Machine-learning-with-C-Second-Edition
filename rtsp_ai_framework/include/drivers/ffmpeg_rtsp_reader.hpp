#pragma once

#include <cstdint>
#include <string>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class FFmpegRtspReader {
public:
    FFmpegRtspReader() = default;

    ~FFmpegRtspReader() {
        close();
    }

    FFmpegRtspReader(const FFmpegRtspReader&) = delete;
    FFmpegRtspReader& operator=(const FFmpegRtspReader&) = delete;

    FFmpegRtspReader(FFmpegRtspReader&&) = delete;
    FFmpegRtspReader& operator=(FFmpegRtspReader&&) = delete;

    bool open(const std::string& url) {
        close();
        avformat_network_init();

        AVDictionary* options = nullptr;

        // Dùng TCP để ổn định hơn khi đọc RTSP.
        av_dict_set(&options, "rtsp_transport", "tcp", 0);

        // 5 giây, đơn vị micro giây.
        // stimeout được hỗ trợ bởi nhiều bản FFmpeg cũ.
        av_dict_set(&options, "stimeout", "5000000", 0);

        int ret = avformat_open_input(
            &fmt_ctx_,
            url.c_str(),
            nullptr,
            &options
        );

        av_dict_free(&options);

        if (ret < 0) {
            log_error("avformat_open_input", ret);
            close();
            return false;
        }

        ret = avformat_find_stream_info(fmt_ctx_, nullptr);
        if (ret < 0) {
            log_error("avformat_find_stream_info", ret);
            close();
            return false;
        }

        video_stream_idx_ = -1;

        for (unsigned int i = 0; i < fmt_ctx_->nb_streams; ++i) {
            if (fmt_ctx_->streams[i]->codecpar->codec_type ==
                AVMEDIA_TYPE_VIDEO) {
                video_stream_idx_ = static_cast<int>(i);
                break;
            }
        }

        if (video_stream_idx_ < 0) {
            std::cerr << "[FFmpegRtspReader] No video stream found\n";
            close();
            return false;
        }

        AVCodecParameters* codecpar =
            fmt_ctx_->streams[video_stream_idx_]->codecpar;

        const AVCodec* codec =
            avcodec_find_decoder(codecpar->codec_id);

        if (codec == nullptr) {
            std::cerr << "[FFmpegRtspReader] Decoder not found\n";
            close();
            return false;
        }

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (codec_ctx_ == nullptr) {
            std::cerr << "[FFmpegRtspReader] Could not allocate codec context\n";
            close();
            return false;
        }

        ret = avcodec_parameters_to_context(codec_ctx_, codecpar);
        if (ret < 0) {
            log_error("avcodec_parameters_to_context", ret);
            close();
            return false;
        }

        ret = avcodec_open2(codec_ctx_, codec, nullptr);
        if (ret < 0) {
            log_error("avcodec_open2", ret);
            close();
            return false;
        }

        if (codec_ctx_->width <= 0 || codec_ctx_->height <= 0) {
            std::cerr << "[FFmpegRtspReader] Invalid video dimensions: "
                      << codec_ctx_->width << "x"
                      << codec_ctx_->height << '\n';
            close();
            return false;
        }

        frame_ = av_frame_alloc();
        frame_bgr_ = av_frame_alloc();

        if (frame_ == nullptr || frame_bgr_ == nullptr) {
            std::cerr << "[FFmpegRtspReader] Could not allocate frames\n";
            close();
            return false;
        }

        const int buffer_size = av_image_get_buffer_size(
            AV_PIX_FMT_BGR24,
            codec_ctx_->width,
            codec_ctx_->height,
            1
        );

        if (buffer_size <= 0) {
            std::cerr << "[FFmpegRtspReader] Invalid BGR buffer size\n";
            close();
            return false;
        }

        buffer_ = static_cast<uint8_t*>(
            av_malloc(static_cast<size_t>(buffer_size))
        );

        if (buffer_ == nullptr) {
            std::cerr << "[FFmpegRtspReader] Could not allocate BGR buffer\n";
            close();
            return false;
        }

        ret = av_image_fill_arrays(
            frame_bgr_->data,
            frame_bgr_->linesize,
            buffer_,
            AV_PIX_FMT_BGR24,
            codec_ctx_->width,
            codec_ctx_->height,
            1
        );

        if (ret < 0) {
            log_error("av_image_fill_arrays", ret);
            close();
            return false;
        }

        sws_ctx_ = sws_getContext(
            codec_ctx_->width,
            codec_ctx_->height,
            codec_ctx_->pix_fmt,
            codec_ctx_->width,
            codec_ctx_->height,
            AV_PIX_FMT_BGR24,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );

        if (sws_ctx_ == nullptr) {
            std::cerr << "[FFmpegRtspReader] Could not create SwsContext\n";
            close();
            return false;
        }

        std::cout << "[FFmpegRtspReader] Stream opened: "
                  << codec_ctx_->width << "x"
                  << codec_ctx_->height << '\n';

        return true;
    }

    bool read(cv::Mat& output) {
        output.release();

        if (!is_open()) {
            return false;
        }

        AVPacket packet{};
        int ret = 0;

        while ((ret = av_read_frame(fmt_ctx_, &packet)) >= 0) {
            if (packet.stream_index != video_stream_idx_) {
                av_packet_unref(&packet);
                continue;
            }

            ret = avcodec_send_packet(codec_ctx_, &packet);
            av_packet_unref(&packet);

            if (ret < 0) {
                continue;
            }

            while (true) {
                ret = avcodec_receive_frame(codec_ctx_, frame_);

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }

                if (ret < 0) {
                    log_error("avcodec_receive_frame", ret);
                    return false;
                }

                sws_scale(
                    sws_ctx_,
                    frame_->data,
                    frame_->linesize,
                    0,
                    codec_ctx_->height,
                    frame_bgr_->data,
                    frame_bgr_->linesize
                );

                output = cv::Mat(
                    codec_ctx_->height,
                    codec_ctx_->width,
                    CV_8UC3,
                    frame_bgr_->data[0],
                    frame_bgr_->linesize[0]
                ).clone();

                return !output.empty();
            }
        }

        if (ret < 0 && ret != AVERROR_EOF) {
            log_error("av_read_frame", ret);
        }

        return false;
    }

    bool is_open() const {
        return fmt_ctx_ != nullptr &&
               codec_ctx_ != nullptr &&
               video_stream_idx_ >= 0;
    }

    int width() const {
        return codec_ctx_ != nullptr ? codec_ctx_->width : 0;
    }

    int height() const {
        return codec_ctx_ != nullptr ? codec_ctx_->height : 0;
    }

    void close() {
        if (sws_ctx_ != nullptr) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = nullptr;
        }

        if (buffer_ != nullptr) {
            av_free(buffer_);
            buffer_ = nullptr;
        }

        if (frame_ != nullptr) {
            av_frame_free(&frame_);
        }

        if (frame_bgr_ != nullptr) {
            av_frame_free(&frame_bgr_);
        }

        if (codec_ctx_ != nullptr) {
            avcodec_free_context(&codec_ctx_);
        }

        if (fmt_ctx_ != nullptr) {
            avformat_close_input(&fmt_ctx_);
        }

        video_stream_idx_ = -1;
    }

    double get_fps() const {
        if (fmt_ctx_ == nullptr ||
            video_stream_idx_ < 0 ||
            video_stream_idx_ >=
                static_cast<int>(fmt_ctx_->nb_streams)) {
            return 0.0;
        }

        AVStream* stream = fmt_ctx_->streams[video_stream_idx_];

        AVRational frame_rate = av_guess_frame_rate(
            fmt_ctx_,
            stream,
            nullptr
        );

        if (frame_rate.num <= 0 || frame_rate.den <= 0) {
            return 0.0;
        }

        return av_q2d(frame_rate);
    }

private:
    static void log_error(const char* operation, int error_code) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE]{};

        av_strerror(
            error_code,
            error_buffer,
            sizeof(error_buffer)
        );

        std::cerr << "[FFmpegRtspReader] "
                  << operation
                  << " failed: "
                  << error_buffer
                  << '\n';
    }

    AVFormatContext* fmt_ctx_{nullptr};
    AVCodecContext* codec_ctx_{nullptr};

    int video_stream_idx_{-1};

    SwsContext* sws_ctx_{nullptr};

    AVFrame* frame_{nullptr};
    AVFrame* frame_bgr_{nullptr};

    uint8_t* buffer_{nullptr};
};

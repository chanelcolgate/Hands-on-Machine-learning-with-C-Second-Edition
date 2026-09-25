#pragma once

#include <cstdint>
#include <iostream>
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
    ~FFmpegRtspReader() { close(); }
    FFmpegRtspReader(const FFmpegRtspReader&) = delete;
    FFmpegRtspReader& operator=(const FFmpegRtspReader&) = delete;
    FFmpegRtspReader(FFmpegRtspReader&&) = delete;
    FFmpegRtspReader& operator=(FFmpegRtspReader&&) = delete;

    bool open(const std::string& url) {
        close();
        avformat_network_init();

        AVDictionary* options = nullptr;
        av_dict_set(&options, "rtsp_transport", "udp", 0);
        av_dict_set(&options, "stimeout", "5000000", 0);
        av_dict_set(&options, "rw_timeout", "5000000", 0);
        av_dict_set(&options, "buffer_size", "10240000", 0);

        int ret = avformat_open_input(&fmt_ctx_, url.c_str(), nullptr, &options);
        av_dict_free(&options);
        if (ret < 0 || fmt_ctx_ == nullptr) {
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
            const AVCodecParameters* params = fmt_ctx_->streams[i]->codecpar;
            std::cout << "[FFmpegRtspReader] Stream " << i
                      << ": type=" << av_get_media_type_string(params->codec_type)
                      << ", codec=" << avcodec_get_name(params->codec_id);
            if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
                std::cout << ", size=" << params->width << "x" << params->height;
                if (video_stream_idx_ < 0) video_stream_idx_ = static_cast<int>(i);
            }
            std::cout << std::endl;
        }

        if (video_stream_idx_ < 0) {
            std::cerr << "[FFmpegRtspReader] No video stream found" << std::endl;
            close();
            return false;
        }

        AVCodecParameters* codecpar = fmt_ctx_->streams[video_stream_idx_]->codecpar;
        const AVCodec* codec = select_decoder(codecpar->codec_id);
        if (codec == nullptr) {
            std::cerr << "[FFmpegRtspReader] No decoder available for "
                      << avcodec_get_name(codecpar->codec_id) << std::endl;
            close();
            return false;
        }

        std::cout << "[FFmpegRtspReader] Using decoder: " << codec->name << std::endl;
        codec_ctx_ = avcodec_alloc_context3(codec);
        if (codec_ctx_ == nullptr) {
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
            std::cerr << "[FFmpegRtspReader] Invalid video dimensions" << std::endl;
            close();
            return false;
        }

        frame_ = av_frame_alloc();
        frame_bgr_ = av_frame_alloc();
        if (frame_ == nullptr || frame_bgr_ == nullptr) {
            close();
            return false;
        }

        const int buffer_size = av_image_get_buffer_size(
            AV_PIX_FMT_BGR24, codec_ctx_->width, codec_ctx_->height, 1);
        if (buffer_size <= 0) {
            close();
            return false;
        }

        buffer_ = static_cast<uint8_t*>(av_malloc(static_cast<size_t>(buffer_size)));
        if (buffer_ == nullptr) {
            close();
            return false;
        }

        ret = av_image_fill_arrays(frame_bgr_->data, frame_bgr_->linesize, buffer_,
                                   AV_PIX_FMT_BGR24, codec_ctx_->width,
                                   codec_ctx_->height, 1);
        if (ret < 0) {
            log_error("av_image_fill_arrays", ret);
            close();
            return false;
        }

        sws_ctx_ = sws_getContext(
            codec_ctx_->width, codec_ctx_->height, codec_ctx_->pix_fmt,
            codec_ctx_->width, codec_ctx_->height, AV_PIX_FMT_BGR24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (sws_ctx_ == nullptr) {
            std::cerr << "[FFmpegRtspReader] Could not create SwsContext" << std::endl;
            close();
            return false;
        }

        std::cout << "[FFmpegRtspReader] Connected using transport: udp\n"
                  << "[FFmpegRtspReader] Stream opened: " << width() << "x"
                  << height() << std::endl;
        return true;
    }

    bool read(cv::Mat& output) {
        output.release();
        if (!is_open()) return false;

        AVPacket packet{};
        int ret = 0;
        while ((ret = av_read_frame(fmt_ctx_, &packet)) >= 0) {
            if (packet.stream_index != video_stream_idx_) {
                av_packet_unref(&packet);
                continue;
            }

            ret = avcodec_send_packet(codec_ctx_, &packet);
            av_packet_unref(&packet);
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                log_error("avcodec_send_packet", ret);
                continue;
            }

            while (true) {
                ret = avcodec_receive_frame(codec_ctx_, frame_);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) {
                    log_error("avcodec_receive_frame", ret);
                    return false;
                }

                sws_scale(sws_ctx_, frame_->data, frame_->linesize, 0,
                          codec_ctx_->height, frame_bgr_->data,
                          frame_bgr_->linesize);

                output = cv::Mat(codec_ctx_->height, codec_ctx_->width, CV_8UC3,
                                 frame_bgr_->data[0], frame_bgr_->linesize[0]).clone();
                if (!output.empty()) return true;
            }
        }

        if (ret < 0 && ret != AVERROR_EOF) log_error("av_read_frame", ret);
        return false;
    }

    bool is_open() const {
        return fmt_ctx_ != nullptr && codec_ctx_ != nullptr && video_stream_idx_ >= 0;
    }

    int width() const { return codec_ctx_ != nullptr ? codec_ctx_->width : 0; }
    int height() const { return codec_ctx_ != nullptr ? codec_ctx_->height : 0; }

    void close() {
        if (sws_ctx_ != nullptr) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = nullptr;
        }
        if (buffer_ != nullptr) {
            av_free(buffer_);
            buffer_ = nullptr;
        }
        if (frame_ != nullptr) av_frame_free(&frame_);
        if (frame_bgr_ != nullptr) av_frame_free(&frame_bgr_);
        if (codec_ctx_ != nullptr) avcodec_free_context(&codec_ctx_);
        if (fmt_ctx_ != nullptr) avformat_close_input(&fmt_ctx_);
        video_stream_idx_ = -1;
    }

    double get_fps() const {
        if (fmt_ctx_ == nullptr || video_stream_idx_ < 0 ||
            video_stream_idx_ >= static_cast<int>(fmt_ctx_->nb_streams)) return 0.0;
        AVRational rate = av_guess_frame_rate(
            fmt_ctx_, fmt_ctx_->streams[video_stream_idx_], nullptr);
        return rate.num > 0 && rate.den > 0 ? av_q2d(rate) : 0.0;
    }

private:
    static const AVCodec* select_decoder(AVCodecID codec_id) {
        // The runtime log shows libopenh264.dll is installed. Prefer it for
        // H.264, but fall back to FFmpeg's registered decoder for other builds.
        if (codec_id == AV_CODEC_ID_H264) {
            if (const AVCodec* openh264 = avcodec_find_decoder_by_name("libopenh264")) {
                return openh264;
            }
        }
        return avcodec_find_decoder(codec_id);
    }

    static void log_error(const char* operation, int error_code) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE]{};
        av_strerror(error_code, error_buffer, sizeof(error_buffer));
        std::cerr << "[FFmpegRtspReader] " << operation << " failed: "
                  << error_buffer << std::endl;
    }

    AVFormatContext* fmt_ctx_{nullptr};
    AVCodecContext* codec_ctx_{nullptr};
    int video_stream_idx_{-1};
    SwsContext* sws_ctx_{nullptr};
    AVFrame* frame_{nullptr};
    AVFrame* frame_bgr_{nullptr};
    uint8_t* buffer_{nullptr};
};

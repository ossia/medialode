#include "video_worker.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include <iostream>

nlohmann::json handle_video_file(int file_id, const std::string& path) {
    nlohmann::json j;
    AVFormatContext* fmt = nullptr;

    if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
        j["type"] = "FileError";
        j["request_id"] = std::to_string(file_id);
        j["path"] = path;
        j["error"] = "Failed to open file";
        return j;
    }

    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        j["type"] = "FileError";
        j["request_id"] = std::to_string(file_id);
        j["path"] = path;
        j["error"] = "Failed to find stream info";
        return j;
    }

    int video_stream_index = -1;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        avformat_close_input(&fmt);
        j["type"] = "FileError";
        j["request_id"] = std::to_string(file_id);
        j["path"] = path;
        j["error"] = "No video stream found";
        return j;
    }

    AVStream* stream = fmt->streams[video_stream_index];
    AVCodecParameters* codecpar = stream->codecpar;

    double duration = (fmt->duration > 0) ? (fmt->duration / (double)AV_TIME_BASE) : 0.0;
    double framerate = 0.0;
    if (stream->avg_frame_rate.num && stream->avg_frame_rate.den) {
        framerate = av_q2d(stream->avg_frame_rate);
    }

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);

    j["type"] = "FileScanned";
    j["request_id"] = std::to_string(file_id);
    j["path"] = path;
    j["duration"] = duration;
    j["width"] = codecpar->width;
    j["height"] = codecpar->height;
    j["codec"] = codec ? codec->name : "unknown";
    j["framerate"] = framerate;

    avformat_close_input(&fmt);
    return j;
}


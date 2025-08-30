#pragma once
#include <nlohmann/json.hpp>
#include <string>

// Extract video metadata using FFmpeg/libav
// Returns a JSON object containing width, height, codec, framerate, duration
nlohmann::json handle_video_file(int file_id, const std::string& path);


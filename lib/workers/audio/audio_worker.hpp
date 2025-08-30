#pragma once
#include <nlohmann/json.hpp>
#include <string>

// Extract metadata for an audio file
// Returns JSON with keys: duration, sample_rate, channels, bitrate, format
nlohmann::json handle_audio_file(const std::string& path);


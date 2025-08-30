#include "audio_worker.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <sstream>
#include <array>
#include <memory>
#include <stdexcept>
#include <iostream>

// Run a shell command and capture output
static std::string run_command(const std::string& cmd) {
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen failed");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

nlohmann::json handle_audio_file(const std::string& path) {
    using json = nlohmann::json;
    json meta;

    try {
        // Use ffprobe (must be installed)
        std::string cmd =
            "ffprobe -v quiet -print_format json -show_format -show_streams \"" + path + "\"";
        std::string output = run_command(cmd);

        // Parse ffprobe output
        auto j = json::parse(output);

        if (j.contains("format")) {
            if (j["format"].contains("duration"))
                meta["duration"] = std::stod(j["format"]["duration"].get<std::string>());
            if (j["format"].contains("bit_rate"))
                meta["bitrate"] = std::stoi(j["format"]["bit_rate"].get<std::string>()) / 1000;
            if (j["format"].contains("format_name"))
                meta["format"] = j["format"]["format_name"].get<std::string>();
        }

        if (j.contains("streams")) {
            for (auto& s : j["streams"]) {
                if (s["codec_type"] == "audio") {
                    if (s.contains("sample_rate"))
                        meta["sample_rate"] = std::stoi(s["sample_rate"].get<std::string>());
                    if (s.contains("channels"))
                        meta["channels"] = s["channels"].get<int>();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[AudioWorker] Error extracting metadata: " << e.what() << "\n";
        meta["error"] = e.what();
    }

    return meta;
}


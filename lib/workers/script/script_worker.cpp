#include "script_worker.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

std::string detect_language(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    if (ext == ".js") return "javascript";
    if (ext == ".ts") return "typescript";
    if (ext == ".glsl" || ext == ".frag" || ext == ".vert") return "glsl";
    if (ext == ".py") return "python";
    return "unknown";
}

std::map<std::string, std::string> handle_script_file(const std::string& path) {
    std::map<std::string, std::string> meta;
    if (!fs::exists(path)) {
        throw std::runtime_error("file not found");
    }

    auto lang = detect_language(path);
    meta["language"] = lang;

    // very basic validation
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("cannot open file");
    }

    std::ostringstream buf;
    buf << f.rdbuf();
    std::string content = buf.str();

    if (content.empty()) {
        meta["status"] = "error";
        meta["error"] = "empty file";
        return meta;
    }

    // TODO: expand with real parsers per language later
    meta["status"] = "ok";
    return meta;
}


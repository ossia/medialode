#pragma once
#include <nlohmann/json.hpp>
#include <string>

// Scan an image file and return metadata JSON response
nlohmann::json handle_image_file(int file_id, const std::string& path);


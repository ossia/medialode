#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "image_worker.hpp"

using json = nlohmann::json;

json handle_image_file(int file_id, const std::string& path)
{
    int w = 0, h = 0, ch = 0;

    // Load image (only header to get dimensions)
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data)
    {
        return {
            {"type", "FileError"},
            {"file_id", file_id},
            {"reason", "failed to load image"}
        };
    }
    stbi_image_free(data);

    // (Optional) preview path
    std::string preview_path = "previews/" + std::to_string(file_id) + ".jpg";

    return {
        {"type", "FileScanned"},
        {"file_id", file_id},
        {"kind", "image"},
        {"width", w},
        {"height", h},
        {"channels", ch},
        {"preview_path", preview_path}
    };
}


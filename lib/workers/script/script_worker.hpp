#pragma once
#include <string>
#include <map>

// Detects language from extension
std::string detect_language(const std::string& path);

// Validates a script file (syntax check for .js/.glsl/etc.)
std::map<std::string, std::string> handle_script_file(const std::string& path);


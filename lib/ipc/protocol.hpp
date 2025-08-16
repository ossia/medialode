#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <vector>

namespace medialode::ipc {

// --- Message types --- //

struct PingRequest {
  std::string message = "ping";
};

struct PingResponse {
  std::string message = "pong";
};

struct ScanFoldersRequest {
  std::vector<std::string> folders;
};

struct ScanFoldersResponse {
  bool success;
  std::string error_message;
};

// --- JSON serialization --- //

using json = nlohmann::json;

inline void to_json(json& j, const PingRequest& msg) {
  j = json{{"message", msg.message}};
}

inline void from_json(const json& j, PingRequest& msg) {
  j.at("message").get_to(msg.message);
}

inline void to_json(json& j, const PingResponse& msg) {
  j = json{{"message", msg.message}};
}

inline void from_json(const json& j, PingResponse& msg) {
  j.at("message").get_to(msg.message);
}

inline void to_json(json& j, const ScanFoldersRequest& msg) {
  j = json{{"folders", msg.folders}};
}

inline void from_json(const json& j, ScanFoldersRequest& msg) {
  j.at("folders").get_to(msg.folders);
}

inline void to_json(json& j, const ScanFoldersResponse& msg) {
  j = json{{"success", msg.success}, {"error_message", msg.error_message}};
}

inline void from_json(const json& j, ScanFoldersResponse& msg) {
  j.at("success").get_to(msg.success);
  j.at("error_message").get_to(msg.error_message);
}

// --- Envelope for polymorphic messages --- //

using MessageVariant = std::variant<PingRequest, PingResponse, ScanFoldersRequest, ScanFoldersResponse>;

struct MessageEnvelope {
  std::string type;
  json data;
};

inline void to_json(json& j, const MessageEnvelope& env) {
  j = json{{"type", env.type}, {"data", env.data}};
}

inline void from_json(const json& j, MessageEnvelope& env) {
  j.at("type").get_to(env.type);
  j.at("data").get_to(env.data);
}

inline MessageVariant parse_variant(const MessageEnvelope& env) {
  if (env.type == "PingRequest") return env.data.get<PingRequest>();
  if (env.type == "PingResponse") return env.data.get<PingResponse>();
  if (env.type == "ScanFoldersRequest") return env.data.get<ScanFoldersRequest>();
  if (env.type == "ScanFoldersResponse") return env.data.get<ScanFoldersResponse>();
  throw std::runtime_error("Unknown message type: " + env.type);
}

inline MessageEnvelope make_envelope(const MessageVariant& var) {
  MessageEnvelope env;
  std::visit([&](auto&& v) {
    using T = std::decay_t<decltype(v)>;
    env.type = typeid(T).name(); // fallback, will replace below
    env.data = v;
    if constexpr (std::is_same_v<T, PingRequest>) env.type = "PingRequest";
    else if constexpr (std::is_same_v<T, PingResponse>) env.type = "PingResponse";
    else if constexpr (std::is_same_v<T, ScanFoldersRequest>) env.type = "ScanFoldersRequest";
    else if constexpr (std::is_same_v<T, ScanFoldersResponse>) env.type = "ScanFoldersResponse";
  }, var);
  return env;
}

} // namespace medialode::ipc


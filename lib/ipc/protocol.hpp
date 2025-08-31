#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

namespace medialode::ipc {

using json = nlohmann::json;

// Existing
struct PingRequest { std::string type = "PingRequest"; };
struct PingResponse { std::string type = "PingResponse"; std::string message; };

struct ScanFoldersRequest { std::string type = "ScanFoldersRequest"; std::vector<std::string> paths; };
struct ScanFoldersResponse {
  std::string type = "ScanFoldersResponse";
  std::size_t scanned_folders = 0;
  std::size_t scanned_files   = 0;
};

// Worker/client identity & file scan
struct WorkerHello {
  std::string type = "WorkerHello";
  std::string role = "scan-worker"; // default role
  std::string token;                // required by server
  std::string name;                 // worker identifier
};

struct ClientHello {
  std::string type = "ClientHello";
  std::string token; // optional
};

struct ScanFileRequest {
  std::string type = "ScanFileRequest";
  std::string request_id;
  std::string path;
};

struct FileScanned {
  std::string type = "FileScanned";
  std::string request_id;
  std::string path;
  std::uintmax_t size = 0;
  std::int64_t   mtime = 0;
};

struct FileError {
  std::string type = "FileError";
  std::string request_id;
  std::string path;
  std::string error;
};

// CLI
struct ListTypesRequest {
  std::string type = "ListTypesRequest";
};

struct ListTypesResponse {
  std::string type = "ListTypesResponse";
  std::vector<std::string> types;
};

struct ListFilesRequest {
  std::string type = "ListFilesRequest";
  std::string folder;
  std::string kind; // "audio", "video", "image", "script", or empty for all
};

struct ListFilesResponse {
  std::string type = "ListFilesResponse";
  std::vector<std::string> files;
};


// Union
using IPCMessage = std::variant<
  PingRequest, PingResponse,
  ScanFoldersRequest, ScanFoldersResponse,
  WorkerHello, ClientHello,
  ScanFileRequest, FileScanned, FileError,
  ListTypesRequest, ListTypesResponse
>;

// JSON

// WorkerHello (merged definition)
inline void to_json(nlohmann::json& j, const WorkerHello& m) {
  j = nlohmann::json{
    {"type", m.type},
    {"role", m.role},
    {"token", m.token},
    {"name", m.name}
  };
}
inline void from_json(const nlohmann::json& j, WorkerHello& m) {
  m.role  = j.value("role", "scan-worker");
  m.token = j.value("token", "");
  m.name  = j.value("name", "");
}

inline void to_json(json& j, const PingRequest& m){ j = {{"type", m.type}}; }
inline void from_json(const json&, PingRequest&){}

inline void to_json(json& j, const PingResponse& m){ j = {{"type", m.type},{"message", m.message}}; }
inline void from_json(const json& j, PingResponse& m){ m.message = j.at("message").get<std::string>(); }

inline void to_json(json& j, const ScanFoldersRequest& m){ j = {{"type", m.type},{"paths", m.paths}}; }
inline void from_json(const json& j, ScanFoldersRequest& m){ m.paths = j.at("paths").get<std::vector<std::string>>(); }

inline void to_json(json& j, const ScanFoldersResponse& m){
  j = {{"type", m.type},{"scanned_folders", m.scanned_folders},{"scanned_files", m.scanned_files}};
}
inline void from_json(const json& j, ScanFoldersResponse& m){
  m.scanned_folders = j.at("scanned_folders").get<std::size_t>();
  m.scanned_files   = j.at("scanned_files").get<std::size_t>();
}

inline void to_json(json& j, const ClientHello& m){ j = {{"type", m.type},{"token", m.token}}; }
inline void from_json(const json& j, ClientHello& m){ m.token = j.value("token",""); }

inline void to_json(json& j, const ScanFileRequest& m){ j = {{"type", m.type},{"request_id", m.request_id},{"path", m.path}}; }
inline void from_json(const json& j, ScanFileRequest& m){
  m.request_id = j.at("request_id").get<std::string>();
  m.path       = j.at("path").get<std::string>();
}

inline void to_json(json& j, const FileScanned& m){
  j = {{"type", m.type},{"request_id", m.request_id},{"path", m.path},{"size", m.size},{"mtime", m.mtime}};
}
inline void from_json(const json& j, FileScanned& m){
  m.request_id = j.at("request_id").get<std::string>();
  m.path       = j.at("path").get<std::string>();
  m.size       = j.at("size").get<std::uintmax_t>();
  m.mtime      = j.at("mtime").get<std::int64_t>();
}

inline void to_json(json& j, const FileError& m){
  j = {{"type", m.type},{"request_id", m.request_id},{"path", m.path},{"error", m.error}};
}
inline void from_json(const json& j, FileError& m){
  m.request_id = j.at("request_id").get<std::string>();
  m.path       = j.at("path").get<std::string>();
  m.error      = j.at("error").get<std::string>();
}

// CLI
inline void to_json(json& j, const ListTypesRequest& m){
  j = {{"type", m.type}};
}
inline void from_json(const json&, ListTypesRequest&){}

inline void to_json(json& j, const ListTypesResponse& m){
  j = {{"type", m.type},{"types", m.types}};
}
inline void from_json(const json& j, ListTypesResponse& m){
  m.types = j.at("types").get<std::vector<std::string>>();
}

inline void to_json(json& j, const ListFilesRequest& m) {
  j = {{"type", m.type}, {"folder", m.folder}, {"kind", m.kind}};
}
inline void from_json(const json& j, ListFilesRequest& m) {
  m.folder = j.value("folder", "");
  m.kind   = j.value("kind", "");
}

inline void to_json(json& j, const ListFilesResponse& m) {
  j = {{"type", m.type}, {"files", m.files}};
}
inline void from_json(const json& j, ListFilesResponse& m) {
  m.files = j.at("files").get<std::vector<std::string>>();
}

// Parse helper
inline IPCMessage parseMessage(const std::string& text){
  auto j = json::parse(text);
  auto t = j.at("type").get<std::string>();
  if(t=="PingRequest") return j.get<PingRequest>();
  if(t=="PingResponse") return j.get<PingResponse>();
  if(t=="ScanFoldersRequest") return j.get<ScanFoldersRequest>();
  if(t=="ScanFoldersResponse") return j.get<ScanFoldersResponse>();
  if(t=="WorkerHello") return j.get<WorkerHello>();
  if(t=="ClientHello") return j.get<ClientHello>();
  if(t=="ScanFileRequest") return j.get<ScanFileRequest>();
  if(t=="FileScanned") return j.get<FileScanned>();
  if(t=="FileError") return j.get<FileError>();
  if(t=="ListTypesRequest") return j.get<ListTypesRequest>();
  if(t=="ListTypesResponse") return j.get<ListTypesResponse>();
  throw std::runtime_error("Unknown message type: " + t);
}

inline std::string to_text(const IPCMessage& msg){
  json j; std::visit([&](auto const& v){ j=v; }, msg); return j.dump();
}

} // namespace medialode::ipc


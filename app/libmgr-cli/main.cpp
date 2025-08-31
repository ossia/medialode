#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: libmgr-cli <command> [args...]\n";
    std::cerr << "Commands:\n";
    std::cerr << "  list types\n";
    std::cerr << "  list files --type <kind> --folder <path> [--scan]\n";
    return 1;
  }

  std::string command = argv[1];
  std::string host = "127.0.0.1";
  std::string port = "8081";
  std::string typeFilter, folderFilter;
  bool doScan = false;

  // parse args
  for (int i = 2; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--host" && i + 1 < argc) host = argv[++i];
    else if (a == "--port" && i + 1 < argc) port = argv[++i];
    else if (a == "--type" && i + 1 < argc) typeFilter = argv[++i];
    else if (a == "--folder" && i + 1 < argc) folderFilter = argv[++i];
    else if (a == "--scan") doScan = true;
  }

  try {
    asio::io_context ioc;
    tcp::resolver resolver{ioc};
    auto results = resolver.resolve(host, port);
    websocket::stream<tcp::socket> ws{ioc};
    asio::connect(ws.next_layer(), results);
    ws.handshake(host, "/");

    // identify as client
    medialode::ipc::ClientHello hello;
    ws.write(asio::buffer(nlohmann::json(hello).dump()));

    if (command == "list" && argc >= 3 && std::string(argv[2]) == "types") {
      medialode::ipc::ListTypesRequest req;
      ws.write(asio::buffer(nlohmann::json(req).dump()));

      beast::flat_buffer buffer;
      ws.read(buffer);
      auto text = beast::buffers_to_string(buffer.data());
      auto j = nlohmann::json::parse(text);

      if (j.at("type") == "ListTypesResponse") {
        std::cout << "Available types:\n";
        for (auto& t : j.at("types")) {
          std::cout << "  - " << t.get<std::string>() << "\n";
        }
      } else {
        std::cerr << "Unexpected response: " << text << "\n";
      }
    }
    else if (command == "list" && argc >= 3 && std::string(argv[2]) == "files") {
      if (folderFilter.empty()) {
        std::cerr << "--folder is required\n";
        return 1;
      }

      // optional scan step
      if (doScan) {
        medialode::ipc::ScanFoldersRequest scanReq;
        scanReq.paths = {folderFilter};
        ws.write(asio::buffer(nlohmann::json(scanReq).dump()));

        beast::flat_buffer bufScan;
        ws.read(bufScan);
        auto scanText = beast::buffers_to_string(bufScan.data());
        auto scanJ = nlohmann::json::parse(scanText);

        if (scanJ.at("type") == "ScanFoldersResponse") {
          std::cout << "Scanned " << scanJ.at("scanned_files").get<int>()
                    << " files in " << scanJ.at("scanned_folders").get<int>()
                    << " folders.\n";
        } else {
          std::cerr << "Unexpected scan response: " << scanText << "\n";
        }
      }

      // now query DB
      medialode::ipc::ListFilesRequest req;
      req.kind = typeFilter;
      req.folder = folderFilter;
      ws.write(asio::buffer(nlohmann::json(req).dump()));

      beast::flat_buffer buffer;
      ws.read(buffer);
      auto text = beast::buffers_to_string(buffer.data());
      auto j = nlohmann::json::parse(text);

      if (j.at("type") == "ListFilesResponse") {
        std::cout << "Files in " << folderFilter;
        if (!typeFilter.empty()) std::cout << " (type=" << typeFilter << ")";
        std::cout << ":\n";
        for (auto& f : j.at("files")) {
          std::cout << "  - " << f.get<std::string>() << "\n";
        }
      } else {
        std::cerr << "Unexpected response: " << text << "\n";
      }
    }
    else {
      std::cerr << "Unknown command: " << command << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}


#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  std::string port = "8081";
  std::string path;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--host" && i + 1 < argc) host = argv[++i];
    else if (a == "--port" && i + 1 < argc) port = argv[++i];
    else if (a == "--help" || a == "-h") {
      std::cout << "Usage: scan-file-cli [--host HOST] [--port PORT] <file>\n";
      return 0;
    } else {
      path = a;
    }
  }
  if (path.empty()) {
    std::cerr << "Usage: scan-file-cli [--host HOST] [--port PORT] <file>\n";
    return 1;
  }

  try {
    asio::io_context ioc;
    tcp::resolver resolver{ioc};
    auto const results = resolver.resolve(host, port);

    websocket::stream<tcp::socket> ws{ioc};
    asio::connect(ws.next_layer(), results);
    ws.handshake(host, "/");

    // ClientHello
    medialode::ipc::ClientHello hello;
    hello.token = "";
    ws.write(asio::buffer(nlohmann::json(hello).dump()));

    // Build ScanFileRequest with unique id
    medialode::ipc::ScanFileRequest req;
    req.request_id = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
    req.path = path;
    ws.write(asio::buffer(nlohmann::json(req).dump()));

    beast::flat_buffer buffer;
    while (true) {
      buffer.consume(buffer.size());
      ws.read(buffer);
      std::string response = beast::buffers_to_string(buffer.data());
      auto j = nlohmann::json::parse(response);
      std::string type = j.at("type").get<std::string>();

      if (type == "FileScanned" || type == "FileError") {
        std::cout << response << "\n";
        break;
      } else {
        std::cerr << "Got unrelated msg: " << response << "\n";
      }
    }

    ws.close(websocket::close_code::normal);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}


#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

int main(int argc, char* argv[]) {
  // Defaults
  std::string host = "127.0.0.1";
  std::string port = "8081";
  std::vector<std::string> paths;

  // Tiny arg parser
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--host" && i + 1 < argc) { host = argv[++i]; }
    else if (a == "--port" && i + 1 < argc) { port = argv[++i]; }
    else if (a == "--help" || a == "-h") {
      std::cout << "Usage: scan-folders-cli [--host HOST] [--port PORT] <folder1> [folder2 ...]\n";
      return 0;
    } else {
      paths.push_back(a);
    }
  }

  if (paths.empty()) {
    std::cerr << "Usage: scan-folders-cli [--host HOST] [--port PORT] <folder1> [folder2 ...]\n";
    return 1;
  }

  try {
    asio::io_context ioc;

    // Resolve and connect
    tcp::resolver resolver{ioc};
    auto const results = resolver.resolve(host, port);

    websocket::stream<tcp::socket> ws{ioc};
    asio::connect(ws.next_layer(), results);
    ws.handshake(host, "/");

    // Build request
    medialode::ipc::ScanFoldersRequest req;
    req.paths = paths;
    nlohmann::json j = req;

    // Send
    ws.write(asio::buffer(j.dump()));

    // Read response
    beast::flat_buffer buffer;
    ws.read(buffer);
    std::string response = beast::buffers_to_string(buffer.data());
    std::cout << response << "\n";

    // Close
    ws.close(websocket::close_code::normal);
    return 0;
  } catch (const beast::system_error& se) {
    std::cerr << "Client error: " << se.code().message() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }
}


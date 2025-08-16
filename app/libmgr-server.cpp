#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

//  WebSocket session
class ws_session : public std::enable_shared_from_this<ws_session> {
  websocket::stream<tcp::socket> ws_;
  beast::flat_buffer               buffer_;

public:
  explicit ws_session(tcp::socket socket)
    : ws_(std::move(socket)) {}

  void run() {
    // Accept the WebSocket handshake
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator(
      [](websocket::response_type& res) {
        res.set(beast::http::field::server, "libmgr-server/0.1");
      }));

    auto self = shared_from_this();
    ws_.async_accept([self](beast::error_code ec) {
      if (ec) {
        std::cerr << "[Session] accept error: " << ec.message() << "\n";
        return;
      }
      std::cout << "[Session] WebSocket accepted\n";
      self->do_read();
    });
  }

private:
  void do_read() {
    auto self = shared_from_this();
    ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t /*bytes*/) {
      if (ec == websocket::error::closed) {
        std::cout << "[Session] closed by peer\n";
        return;
      }
      if (ec) {
        std::cerr << "[Session] read error: " << ec.message() << "\n";
        return;
      }

      try {
        const std::string msg = beast::buffers_to_string(self->buffer_.data());
        std::cout << "[Session] received: " << msg << "\n";

        // Parse JSON and handle a "ping" style RPC
        json req = json::parse(msg, /*cb*/nullptr, /*allow_exceptions*/true);

        // Accept either { "type": "ping" } or { "type": "PingRequest" }
        const std::string type = req.value("type", "");
        if (type == "ping" || type == "PingRequest") {
          json resp = {{"status", "ok"}};
          const auto payload = resp.dump();

          auto write_self = self->shared_from_this();
          self->ws_.async_write(
            asio::buffer(payload),
            [write_self](beast::error_code wec, std::size_t /*n*/) {
              if (wec) {
                std::cerr << "[Session] write error: " << wec.message() << "\n";
              } else {
                std::cout << "[Session] responded with {\"status\":\"ok\"}\n";
              }
            });
        } else {
          // Unknown message types just get ignored for now (skeleton)
          std::cout << "[Session] unknown request type, ignoring\n";
        }
      } catch (const std::exception& e) {
        std::cerr << "[Session] json error: " << e.what() << "\n";
      }

      // Clear buffer and keep reading
      self->buffer_.consume(self->buffer_.size());
      self->do_read();
    });
  }
};

//TCP listener
class ws_listener : public std::enable_shared_from_this<ws_listener> {
  asio::io_context& ioc_;
  tcp::acceptor     acceptor_;

public:
  ws_listener(asio::io_context& ioc, tcp::endpoint ep)
    : ioc_(ioc), acceptor_(ioc) {
    beast::error_code ec;

    acceptor_.open(ep.protocol(), ec);
    if (ec) throw beast::system_error(ec);

    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) throw beast::system_error(ec);

    acceptor_.bind(ep, ec);
    if (ec) throw beast::system_error(ec);

    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) throw beast::system_error(ec);
  }

  void run() { do_accept(); }

private:
  void do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([self](beast::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::cout << "[Server] new TCP connection\n";
        std::make_shared<ws_session>(std::move(socket))->run();
      } else {
        std::cerr << "[Server] accept error: " << ec.message() << "\n";
      }
      self->do_accept();
    });
  }
};

int main(int argc, char** argv) {
  try {
    // Allow overriding host/port via CLI: libmgr-server [host] [port]
    std::string host = "127.0.0.1";
    unsigned short port = 8081;
    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<unsigned short>(std::stoi(argv[2]));

    asio::io_context ioc;

    tcp::endpoint ep{asio::ip::make_address(host), port};
    std::cout << "[Server] listening on ws://" << host << ":" << port << "\n";

    std::make_shared<ws_listener>(ioc, ep)->run();
    ioc.run();
  } catch (const std::exception& e) {
    std::cerr << "[Fatal] " << e.what() << "\n";
    return 1;
  }
  return 0;
}


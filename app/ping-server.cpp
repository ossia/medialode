#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <memory>
#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
  websocket::stream<tcp::socket> ws_;
  beast::flat_buffer buffer_;

public:
  explicit WebSocketSession(tcp::socket socket)
    : ws_(std::move(socket)) {}

  void start() {
    std::cout << "[Session] Starting new WebSocket session\n";
    auto self = shared_from_this();
    ws_.async_accept([self](beast::error_code ec) {
      if (!ec) {
        std::cout << "[Session] Connection accepted\n";
        self->do_read();
      } else {
        std::cerr << "[Session] Accept error: " << ec.message() << "\n";
      }
    });
  }

  void do_read() {
    auto self = shared_from_this();
    ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t bytes_transferred) {
      if (ec) {
        if (ec == websocket::error::closed) {
          std::cout << "[Session] Connection closed by client\n";
        } else {
          std::cerr << "[Session] Read error: " << ec.message() << "\n";
        }
        return;
      }

      try {
        std::string msg_text = beast::buffers_to_string(self->buffer_.data());
        self->buffer_.consume(self->buffer_.size()); // ✅ consume immediately

        std::cout << "[Session] Received message: " << msg_text << "\n";

        // Parse JSON and handle IPC
        auto j = nlohmann::json::parse(msg_text);
        auto envelope = j.get<medialode::ipc::MessageEnvelope>();
        auto var = medialode::ipc::parse_variant(envelope);

        if (std::holds_alternative<medialode::ipc::PingRequest>(var)) {
          std::cout << "[Session] Handling PingRequest -> Sending PingResponse\n";
          medialode::ipc::PingResponse resp;
          auto reply = medialode::ipc::make_envelope(resp);
          nlohmann::json jresp = reply;

          self->ws_.async_write(
            asio::buffer(jresp.dump()),
            [self](beast::error_code ec, std::size_t) {
              if (ec) {
                std::cerr << "[Session] Write error: " << ec.message() << "\n";
              } else {
                std::cout << "[Session] Response sent successfully\n";
              }
            }
          );
        }
      } catch (const std::exception& e) {
        std::cerr << "[Session] Exception: " << e.what() << '\n';
      }

      self->do_read(); // Keep reading messages
    });
  }
};

void run_server(asio::io_context& ioc, uint16_t port = 37587) {
  auto acceptor = std::make_shared<tcp::acceptor>(ioc, tcp::endpoint(tcp::v4(), port));
  std::cout << "[Server] Running on ws://localhost:" << port << "\n";

  auto do_accept = std::make_shared<std::function<void()>>();
  *do_accept = [acceptor, do_accept]() {
    acceptor->async_accept([acceptor, do_accept](beast::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::cout << "[Server] New connection accepted\n";
        std::make_shared<WebSocketSession>(std::move(socket))->start();
      } else {
        std::cerr << "[Server] Accept error: " << ec.message() << "\n";
      }
      (*do_accept)(); // Continue accepting new clients
    });
  };

  (*do_accept)();
}

int main() {
  try {
    boost::asio::io_context ctx;
    run_server(ctx, 37587);
    ctx.run();
  } catch (const std::exception& e) {
    std::cerr << "[Fatal] Exception in main: " << e.what() << "\n";
  }
}

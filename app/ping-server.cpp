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
    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
      if (!ec) self->do_read();
    });
  }

  void do_read() {
    ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
      if (ec) return;

      try {
        auto msg_text = beast::buffers_to_string(self->buffer_.data());
        auto j = nlohmann::json::parse(msg_text);
        auto envelope = j.get<medialode::ipc::MessageEnvelope>();
        auto var = medialode::ipc::parse_variant(envelope);

        // Handle PingRequest
        if (std::holds_alternative<medialode::ipc::PingRequest>(var)) {
          medialode::ipc::PingResponse resp;
          auto reply = medialode::ipc::make_envelope(resp);
          nlohmann::json jresp = reply;
          self->ws_.async_write(
            asio::buffer(jresp.dump()), [](beast::error_code, std::size_t) {});
        }
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
      }

      self->buffer_.consume(self->buffer_.size());
      self->do_read(); // continue reading
    });
  }
};

void run_server(asio::io_context& ioc, uint16_t port = 37587) {
  tcp::acceptor acceptor{ioc, {tcp::v4(), port}};
  std::cout << "Server running on ws://localhost:" << port << "\n";

  std::function<void()> do_accept;
  do_accept = [&]() {
    acceptor.async_accept([&](beast::error_code ec, tcp::socket socket) {
      if (!ec)
        std::make_shared<WebSocketSession>(std::move(socket))->start();
      do_accept();
    });
  };

  do_accept();
}

int main() {
  boost::asio::io_context ctx;
  run_server(ctx, 37587);
  ctx.run();
}

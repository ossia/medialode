#define BOOST_ASIO_NO_DEPRECATED
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#define ASIO_STANDALONE 0

#include <boost/asio.hpp>
#include <ossia/network/sockets/websocket.hpp>
#include <iostream>
#include "ipc/protocol.hpp"

int main() {
  boost::asio::io_context ctx;
  ossia::net::websocket_simple_client socket{{.url = "ws://127.0.0.1:37587"}, ctx};

  socket.on_open.connect([&] {
    medialode::ipc::PingRequest ping;
    medialode::ipc::MessageEnvelope env = medialode::ipc::make_envelope(ping);
    nlohmann::json j = env;
    socket.send_message(j.dump());
  });

  socket.on_message.connect([&](const std::string& msg) {
    auto j = nlohmann::json::parse(msg);
    auto envelope = j.get<medialode::ipc::MessageEnvelope>();
    auto var = medialode::ipc::parse_variant(envelope);

    if (auto* pong = std::get_if<medialode::ipc::PingResponse>(&var)) {
      std::cout << "Received pong: " << pong->message << "\n";
      socket.close();
    }
  });

  socket.on_fail.connect([] { std::cerr << "Connection failed\n"; });
  socket.on_close.connect([] { std::cout << "Connection closed\n"; });

  socket.connect("ws://127.0.0.1:37587");
  ctx.run();
}


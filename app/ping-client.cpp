#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include "ipc/protocol.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto const results = resolver.resolve("127.0.0.1", "37587");
        net::connect(ws.next_layer(), results.begin(), results.end());

        ws.handshake("127.0.0.1:37587", "/");

        // Send PingRequest
        medialode::ipc::PingRequest ping;
        auto env = medialode::ipc::make_envelope(ping);
        ws.write(net::buffer(nlohmann::json(env).dump()));

        // Read PongResponse
        beast::flat_buffer buffer;
        ws.read(buffer);
        auto msg = beast::buffers_to_string(buffer.data());

        auto envelope = nlohmann::json::parse(msg).get<medialode::ipc::MessageEnvelope>();
        auto var = medialode::ipc::parse_variant(envelope);
        if (auto* pong = std::get_if<medialode::ipc::PingResponse>(&var)) {
            std::cout << "Received pong: " << pong->message << "\n";
        }

        ws.close(websocket::close_code::normal);
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "../lib/ipc/protocol.hpp"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_client> client;

int main() {
    client c;
    std::string uri = "ws://localhost:8081";

    c.init_asio();

    c.set_message_handler([&](websocketpp::connection_hdl, client::message_ptr msg) {
        try {
            auto parsed = parseMessage(msg->get_payload());

            std::visit([&](auto&& m) {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, PingResponse>) {
                    std::cout << "[Server]: PingResponse: " << m.message << "\n";
                } else if constexpr (std::is_same_v<T, ScanFoldersResponse>) {
                    std::cout << "[Server]: ScanFoldersResponse: status=" << m.status
                              << " scanned_count=" << m.scanned_count << "\n";
                } else {
                    std::cout << "[Server]: Unknown response type\n";
                }
            }, parsed);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing response: " << e.what() << "\n";
        }
    });

    websocketpp::lib::error_code ec;
    auto con = c.get_connection(uri, ec);
    if (ec) {
        std::cerr << "Connection error: " << ec.message() << "\n";
        return 1;
    }

    c.connect(con);
    std::thread t([&] { c.run(); });

    // Send a PingRequest
    PingRequest ping;
    json j_ping = ping;
    c.send(con->get_handle(), j_ping.dump(), websocketpp::frame::opcode::text);

    std::cout << "Sent PingRequest.\n";

    // Send a ScanFoldersRequest
    ScanFoldersRequest scan;
    scan.paths = {"/home/user/Music", "/mnt/external/Movies"};
    json j_scan = scan;
    c.send(con->get_handle(), j_scan.dump(), websocketpp::frame::opcode::text);

    std::cout << "Sent ScanFoldersRequest.\n";

    t.join();
}


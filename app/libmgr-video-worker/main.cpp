#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "video_worker.hpp"
#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

static std::string env_str(const char* k, std::string def="") {
    if (const char* v = std::getenv(k)) return v;
    return def;
}
static const std::string WORKER_TOKEN = env_str("LIBMGR_WORKER_TOKEN", "changeme");

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    std::string port = "8081";

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--host" && i + 1 < argc) host = argv[++i];
        else if (a == "--port" && i + 1 < argc) port = argv[++i];
    }

    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        auto results = resolver.resolve(host, port);

        websocket::stream<tcp::socket> ws(ioc);
        asio::connect(ws.next_layer(), results);
        ws.handshake(host, "/");

        // WorkerHello
        medialode::ipc::WorkerHello hello;
        hello.token = WORKER_TOKEN;
        hello.name = "video";
        ws.write(asio::buffer(nlohmann::json(hello).dump()));
        std::cout << R"({"level":"info","msg":"video-worker connected"})" << std::endl;

        beast::flat_buffer buffer;
        while (true) {
            buffer.consume(buffer.size());
            ws.read(buffer);
            auto msg_text = beast::buffers_to_string(buffer.data());
            auto j = nlohmann::json::parse(msg_text);
            std::string type = j.at("type").get<std::string>();

            if (type == "PingRequest") {
                medialode::ipc::PingResponse pr; pr.message = "pong";
                ws.write(asio::buffer(nlohmann::json(pr).dump()));
            }
            else if (type == "ScanFileRequest") {
                auto req = j.get<medialode::ipc::ScanFileRequest>();
                auto reply = handle_video_file(0, req.path);
                reply["request_id"] = req.request_id; // match request
                ws.write(asio::buffer(reply.dump()));
                std::cout << R"({"level":"info","msg":"video file processed"})" << std::endl;
            }
            else {
                std::cout << R"({"level":"warn","msg":"ignored msg"})" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << R"({"level":"error","msg":"video-worker crashed","err":")"
                  << e.what() << "\"}" << std::endl;
        return 1;
    }
}


#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <thread>
#include <random>
#include <nlohmann/json.hpp>
#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
namespace fs = std::filesystem;

static std::string env_str(const char* k, std::string def=""){
  if(const char* v = std::getenv(k)) return v; return def;
}
static const std::string WORKER_TOKEN = env_str("LIBMGR_WORKER_TOKEN", "changeme");

static std::int64_t to_epoch_seconds(fs::file_time_type t){
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(t - fs::file_time_type::clock::now()
        + system_clock::now());
  return static_cast<std::int64_t>(system_clock::to_time_t(sctp));
}

int main(int argc, char* argv[]){
  std::string host = "127.0.0.1";
  std::string port = "8081";
  for(int i=1;i<argc;++i){
    std::string a = argv[i];
    if(a=="--host" && i+1<argc) host=argv[++i];
    else if(a=="--port" && i+1<argc) port=argv[++i];
  }

  std::mt19937 rng{std::random_device{}()};

  for(;;){ // reconnect loop
    try{
      asio::io_context ioc;

      tcp::resolver resolver{ioc};
      auto results = resolver.resolve(host, port);
      websocket::stream<tcp::socket> ws{ioc};
      ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
      asio::connect(ws.next_layer(), results);
      ws.handshake(host, "/");

      // hello + token
      medialode::ipc::WorkerHello hello;
      hello.token = WORKER_TOKEN;
      ws.write(asio::buffer(nlohmann::json(hello).dump()));
      std::cout << R"({"level":"info","msg":"worker connected"})" << "\n";

      beast::flat_buffer buffer;
      for(;;){
        buffer.consume(buffer.size());
        ws.read(buffer);
        auto text = beast::buffers_to_string(buffer.data());
        auto j = nlohmann::json::parse(text);
        auto type = j.at("type").get<std::string>();

        if(type == "PingRequest"){
          medialode::ipc::PingResponse pr; pr.message = "pong";
          ws.write(asio::buffer(nlohmann::json(pr).dump()));
        } else if(type == "ScanFileRequest"){
          auto req = j.get<medialode::ipc::ScanFileRequest>();
          medialode::ipc::IPCMessage reply;
          try{
            fs::path p{req.path};
            if(!fs::exists(p) || !fs::is_regular_file(p)){
              medialode::ipc::FileError fe{.type="FileError", .request_id=req.request_id, .path=req.path, .error="not a regular file or missing"};
              reply = fe;
            } else {
              medialode::ipc::FileScanned ok;
              ok.request_id = req.request_id;
              ok.path = req.path;
              ok.size = fs::file_size(p);
              ok.mtime = to_epoch_seconds(fs::last_write_time(p));
              reply = ok;
            }
          }catch(const std::exception& e){
            medialode::ipc::FileError fe;
            fe.request_id = req.request_id; fe.path = req.path; fe.error = e.what();
            reply = fe;
          }
          ws.write(asio::buffer(medialode::ipc::to_text(reply)));
        }
      }
    }catch(const std::exception& e){
      std::cerr << R"({"level":"error","msg":"worker disconnected","err":")" << e.what() << "\"}\n";
      // exponential backoff 0.5s..30s with jitter
      static int attempt = 0;
      int backoff_ms = std::min(30000, (1<<std::min(attempt, 10)) * 500);
      std::uniform_int_distribution<int> jitter(0, backoff_ms/4);
      std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms + jitter(rng)));
      ++attempt;
      continue;
    }
  }
}


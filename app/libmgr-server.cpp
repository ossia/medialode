#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <sqlite3.h>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <csignal>
#include <optional>
#include <future>
#include <vector>
#include <utility>
#include <type_traits>

#include "ipc/protocol.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
namespace fs = std::filesystem;

static constexpr const char* DEFAULT_DB = "medialode.db";

// Config
static std::string env_str(const char* k, std::string def="") {
  if (const char* v = std::getenv(k)) return v;
  return def;
}
static const std::string WORKER_TOKEN = env_str("LIBMGR_WORKER_TOKEN", "changeme");
static const std::size_t MAX_WRITE_QUEUE = 1024;
static const std::size_t MAX_MESSAGE_SIZE = 2 * 1024 * 1024;

// Logger
static void logj(const std::string& level, const std::string& msg,
                 std::optional<std::string> req_id = std::nullopt) {
  nlohmann::json j{{"level", level}, {"msg", msg}};
  if (req_id) j["request_id"] = *req_id;
  std::cout << j.dump() << std::endl;
}

// SQLite handle
struct DbHandle {
  sqlite3* db = nullptr;
  ~DbHandle() { if (db) sqlite3_close(db); }
};

static int sqlite_busy_cb(void*, int) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return 1;
}

static void exec_retry(sqlite3* db, const char* sql) {
  for (;;) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
    if (rc == SQLITE_OK) return;
    if (rc == SQLITE_BUSY) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    std::string e = errmsg ? errmsg : sqlite3_errmsg(db);
    if (errmsg) sqlite3_free(errmsg);
    throw std::runtime_error("sqlite error: " + e + " for SQL: " + sql);
  }
}

static std::unique_ptr<DbHandle> open_db(const std::string& path) {
  auto h = std::make_unique<DbHandle>();
  if (sqlite3_open(path.c_str(), &h->db) != SQLITE_OK) {
    throw std::runtime_error("Failed to open DB: " +
                             std::string(sqlite3_errmsg(h->db)));
  }
  sqlite3_busy_handler(h->db, &sqlite_busy_cb, nullptr);
  sqlite3_busy_timeout(h->db, 5000);

  exec_retry(h->db, "PRAGMA journal_mode = WAL;");
  exec_retry(h->db, "PRAGMA synchronous = NORMAL;");
  exec_retry(h->db, "PRAGMA foreign_keys = ON;");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS folders ("
    " path TEXT PRIMARY KEY,"
    " last_scan INTEGER);");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS files ("
    " path TEXT PRIMARY KEY,"
    " folder_path TEXT,"
    " mtime INTEGER,"
    " kind TEXT DEFAULT ''"
    ");");
  exec_retry(h->db, "CREATE INDEX IF NOT EXISTS idx_files_folder ON files(folder_path);");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS image_metadata ("
    " path TEXT PRIMARY KEY,"
    " width INTEGER,"
    " height INTEGER,"
    " channels INTEGER,"
    " FOREIGN KEY(path) REFERENCES files(path) ON DELETE CASCADE"
    ");");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS audio_metadata ("
    " path TEXT PRIMARY KEY,"
    " duration REAL,"
    " sample_rate INTEGER,"
    " channels INTEGER,"
    " bitrate INTEGER,"
    " format TEXT,"
    " FOREIGN KEY(path) REFERENCES files(path) ON DELETE CASCADE"
    ");");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS video_metadata ("
    " path TEXT PRIMARY KEY,"
    " duration REAL,"
    " width INTEGER,"
    " height INTEGER,"
    " codec TEXT,"
    " framerate REAL,"
    " FOREIGN KEY(path) REFERENCES files(path) ON DELETE CASCADE"
    ");");

  exec_retry(h->db,
    "CREATE TABLE IF NOT EXISTS script_metadata ("
    " path TEXT PRIMARY KEY,"
    " language TEXT,"
    " syntax_ok INTEGER,"
    " error_message TEXT,"
    " FOREIGN KEY(path) REFERENCES files(path) ON DELETE CASCADE"
    ");");

  return h;
}

// Upsert helpers
static void upsert_folder(sqlite3* db, const std::string& path, std::int64_t last_scan) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "INSERT INTO folders(path,last_scan) VALUES(?,?) "
                    "ON CONFLICT(path) DO UPDATE SET last_scan=excluded.last_scan;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, last_scan);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

static void upsert_file(sqlite3* db, const std::string& path,
                        const std::string& folder, std::int64_t mtime,
                        const std::string& kind = "") {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "INSERT INTO files(path,folder_path,mtime,kind) VALUES(?,?,?,?) "
                    "ON CONFLICT(path) DO UPDATE SET "
                    " folder_path=excluded.folder_path,"
                    " mtime=excluded.mtime,"
                    " kind=excluded.kind;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, folder.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, mtime);
  sqlite3_bind_text(stmt, 4, kind.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

static void upsert_image_metadata(sqlite3* db,
                                  const std::string& path,
                                  int width, int height, int channels) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
    "INSERT INTO image_metadata(path,width,height,channels) VALUES(?,?,?,?) "
    "ON CONFLICT(path) DO UPDATE SET "
    " width=excluded.width,"
    " height=excluded.height,"
    " channels=excluded.channels;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, width);
  sqlite3_bind_int(stmt, 3, height);
  sqlite3_bind_int(stmt, 4, channels);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

static void upsert_audio_metadata(sqlite3* db,
                                  const std::string& path,
                                  double duration,
                                  int sample_rate,
                                  int channels,
                                  int bitrate,
                                  const std::string& format) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
    "INSERT INTO audio_metadata(path,duration,sample_rate,channels,bitrate,format)"
    " VALUES(?,?,?,?,?,?) "
    "ON CONFLICT(path) DO UPDATE SET "
    " duration=excluded.duration,"
    " sample_rate=excluded.sample_rate,"
    " channels=excluded.channels,"
    " bitrate=excluded.bitrate,"
    " format=excluded.format;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 2, duration);
  sqlite3_bind_int(stmt, 3, sample_rate);
  sqlite3_bind_int(stmt, 4, channels);
  sqlite3_bind_int(stmt, 5, bitrate);
  sqlite3_bind_text(stmt, 6, format.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

static void upsert_video_metadata(sqlite3* db,
                                  const std::string& path,
                                  double duration,
                                  int width,
                                  int height,
                                  const std::string& codec,
                                  double framerate) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
    "INSERT INTO video_metadata(path,duration,width,height,codec,framerate)"
    " VALUES(?,?,?,?,?,?) "
    "ON CONFLICT(path) DO UPDATE SET "
    " duration=excluded.duration,"
    " width=excluded.width,"
    " height=excluded.height,"
    " codec=excluded.codec,"
    " framerate=excluded.framerate;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 2, duration);
  sqlite3_bind_int(stmt, 3, width);
  sqlite3_bind_int(stmt, 4, height);
  sqlite3_bind_text(stmt, 5, codec.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 6, framerate);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

static void upsert_script_metadata(sqlite3* db,
                                   const std::string& path,
                                   const std::string& language,
                                   bool syntax_ok,
                                   const std::string& error_message) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
    "INSERT INTO script_metadata(path,language,syntax_ok,error_message)"
    " VALUES(?,?,?,?) "
    "ON CONFLICT(path) DO UPDATE SET "
    " language=excluded.language,"
    " syntax_ok=excluded.syntax_ok,"
    " error_message=excluded.error_message;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, language.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, syntax_ok ? 1 : 0);
  sqlite3_bind_text(stmt, 4, error_message.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error(sqlite3_errmsg(db));
  sqlite3_finalize(stmt);
}

// DBExecutor
class DBExecutor {
public:
  explicit DBExecutor(std::unique_ptr<DbHandle> dbh)
    : dbh_(std::move(dbh)),
      guard_(asio::make_work_guard(ioc_)),
      t_([this] { ioc_.run(); }) {}

  ~DBExecutor() {
    guard_.reset();
    ioc_.stop();
    if (t_.joinable()) t_.join();
  }

  template<class F>
  auto exec(F&& f) -> std::future<std::invoke_result_t<F, sqlite3*>> {
    using R = std::invoke_result_t<F, sqlite3*>;
    auto prom = std::make_shared<std::promise<R>>();
    asio::post(ioc_, [this, prom, fn = std::forward<F>(f)]() mutable {
      try {
        if constexpr(std::is_void_v<R>) {
          fn(dbh_->db);
          prom->set_value();
        } else {
          prom->set_value(fn(dbh_->db));
        }
      } catch (...) {
        prom->set_exception(std::current_exception());
      }
    });
    return prom->get_future();
  }

private:
  std::unique_ptr<DbHandle> dbh_;
  asio::io_context ioc_;
  asio::executor_work_guard<asio::io_context::executor_type> guard_;
  std::thread t_;
};

// ServerState
struct WebSocketSession;
struct ServerState {
  std::mutex m;
  std::vector<std::weak_ptr<WebSocketSession>> workers;
  std::unordered_map<std::string, std::weak_ptr<WebSocketSession>> pending;
  void add_worker(const std::shared_ptr<WebSocketSession>& s) {
    std::lock_guard lk(m); workers.push_back(s);
  }
  std::shared_ptr<WebSocketSession> get_any_worker() {
    std::lock_guard lk(m);
    for (auto it = workers.begin(); it != workers.end();) {
      if (auto sp = it->lock()) return sp;
      it = workers.erase(it);
    }
    return nullptr;
  }
  void map_request(const std::string& id, std::shared_ptr<WebSocketSession> c) {
    std::lock_guard lk(m); pending[id] = c;
  }
  std::shared_ptr<WebSocketSession> take_client(const std::string& id) {
    std::lock_guard lk(m);
    auto it = pending.find(id);
    if (it == pending.end()) return nullptr;
    auto sp = it->second.lock();
    pending.erase(it);
    return sp;
  }
};

// WebSocketSession
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
  websocket::stream<tcp::socket> ws_;
  beast::flat_buffer buf_;
  DBExecutor& dbx_;
  ServerState& state_;
  enum class Role { Client, Worker } role_ = Role::Client;
  std::deque<std::string> wq_;
  asio::steady_timer ping_;
  bool got_pong_ = true;

public:
  WebSocketSession(tcp::socket sock, DBExecutor& dbx, ServerState& st)
    : ws_(std::move(sock)), dbx_(dbx), state_(st),
      ping_(ws_.get_executor()) {}

  void start() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.read_message_max(MAX_MESSAGE_SIZE);
    ws_.control_callback([this](websocket::frame_type k, boost::beast::string_view) {
      if (k == websocket::frame_type::pong) got_pong_ = true;
    });
    auto self = shared_from_this();
    ws_.async_accept([self](beast::error_code ec) {
      if (!ec) { logj("info", "session accepted"); self->schedule_ping(); self->do_read(); }
      else logj("error", "accept: " + ec.message());
    });
  }

  void schedule_ping() {
    auto self = shared_from_this();
    ping_.expires_after(std::chrono::seconds(15));
    ping_.async_wait([self](beast::error_code ec) {
      if (ec) return;
      if (!self->got_pong_) {
        logj("warn", "no pong, closing");
        beast::error_code ignore;
        self->ws_.close(websocket::close_code::going_away, ignore);
        return;
      }
      self->got_pong_ = false;
      self->ws_.async_ping({}, [self](beast::error_code) { self->schedule_ping(); });
    });
  }

  void do_read() {
    auto self = shared_from_this();
    ws_.async_read(buf_, [self](beast::error_code ec, std::size_t) {
      if (ec) { logj("info", ec == websocket::error::closed ? "session closed" : "read:" + ec.message()); return; }
      std::string msg = beast::buffers_to_string(self->buf_.data());
      self->buf_.consume(self->buf_.size());
      self->handle(msg);
      self->do_read();
    });
  }

  void handle(const std::string& msg) {
    try {
      auto j = nlohmann::json::parse(msg);
      auto type = j.at("type").get<std::string>();

      if (type == "PingRequest") {
        medialode::ipc::PingResponse r; r.message = "pong";
        enqueue(nlohmann::json(r).dump());
        logj("debug", "responded to PingRequest");
      }
      else if (type == "WorkerHello") {
        auto hello = j.get<medialode::ipc::WorkerHello>();
        if (hello.token != WORKER_TOKEN) {
          logj("warn", "worker auth failed");
          ws_.close(websocket::close_code::policy_error);
          return;
        }
        role_ = Role::Worker;
        state_.add_worker(shared_from_this());
        logj("info", "worker registered: " + hello.name);
      }
      else if (type == "ClientHello") {
        logj("info", "client connected");
      }
      else if (type == "ScanFoldersRequest") {
        auto req = j.get<medialode::ipc::ScanFoldersRequest>();
        logj("debug", "received ScanFoldersRequest");
        auto self = shared_from_this();
        std::thread([self, req]() {
          try {
            std::vector<std::string> folders;
            std::vector<std::tuple<std::string, std::string, std::int64_t>> files;
            std::size_t fcount = 0, flcount = 0;
            auto now = (int64_t)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            for (auto& raw : req.paths) {
              fs::path root(raw);
              if (!fs::exists(root)) continue;
              folders.push_back(root.string()); fcount++;
              for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied);
                   it != fs::recursive_directory_iterator(); ++it) {
                if (it->is_directory()) { folders.push_back(it->path().string()); fcount++; }
                else if (it->is_regular_file()) {
                  auto mtime = std::chrono::duration_cast<std::chrono::seconds>(it->last_write_time().time_since_epoch()).count();
                  files.emplace_back(it->path().string(), it->path().parent_path().string(), mtime);
                  flcount++;
                }
              }
            }
            auto fut = self->dbx_.exec([folders = std::move(folders), files = std::move(files), now](sqlite3* db) {
              exec_retry(db, "BEGIN;");
              try {
                for (auto& f : folders) upsert_folder(db, f, now);
                for (auto& t : files) upsert_file(db, std::get<0>(t), std::get<1>(t), std::get<2>(t));
                exec_retry(db, "COMMIT;");
              } catch (...) { exec_retry(db, "ROLLBACK;"); throw; }
            });
            fut.get();
            medialode::ipc::ScanFoldersResponse resp;
            resp.scanned_folders = fcount; resp.scanned_files = flcount;
            self->enqueue(nlohmann::json(resp).dump());
            logj("debug", "ScanFoldersRequest completed");
          } catch (const std::exception& e) { logj("error", "scan-folders:" + std::string(e.what())); }
        }).detach();
      }
      else if (type == "ListTypesRequest") {
        medialode::ipc::ListTypesResponse resp;
        resp.types = {"image","audio","video","script"};
        enqueue(nlohmann::json(resp).dump());
        logj("debug", "handled ListTypesRequest");
      }
      else if (type == "ListFilesRequest") {
        auto req = j.get<medialode::ipc::ListFilesRequest>();
        logj("debug", "received ListFilesRequest");
        auto self = shared_from_this();
        std::thread([self, req]() {
          try {
            auto fut = self->dbx_.exec([req](sqlite3* db) {
              std::vector<std::string> paths;
              sqlite3_stmt* stmt = nullptr;
              std::string sql = "SELECT path FROM files WHERE 1=1";
              if (!req.kind.empty()) {
                sql += " AND kind=?";
              }
              if (!req.folder.empty()) {
                sql += " AND folder_path=?";
              }
              if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
                throw std::runtime_error(sqlite3_errmsg(db));
              int idx = 1;
              if (!req.kind.empty()) sqlite3_bind_text(stmt, idx++, req.kind.c_str(), -1, SQLITE_TRANSIENT);
              if (!req.folder.empty()) sqlite3_bind_text(stmt, idx++, req.folder.c_str(), -1, SQLITE_TRANSIENT);
              while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* txt = sqlite3_column_text(stmt, 0);
                if (txt) paths.emplace_back(reinterpret_cast<const char*>(txt));
              }
              sqlite3_finalize(stmt);
              return paths;
            });
            auto paths = fut.get();
            medialode::ipc::ListFilesResponse resp;
            resp.files = paths;
            self->enqueue(nlohmann::json(resp).dump());
            logj("debug", "handled ListFilesRequest with " + std::to_string(paths.size()) + " files");
          } catch (const std::exception& e) {
            logj("error", std::string("list-files:") + e.what());
          }
        }).detach();
      }
      else if (type == "ScanFileRequest" && role_ == Role::Client) {
        auto req = j.get<medialode::ipc::ScanFileRequest>();
        logj("debug", "received ScanFileRequest for " + req.path);
        if (req.request_id.empty()) {
          medialode::ipc::FileError fe; fe.request_id = ""; fe.path = req.path; fe.error = "missing request_id";
          enqueue(nlohmann::json(fe).dump());
          logj("error", "ScanFileRequest missing request_id");
        } else {
          auto worker = state_.get_any_worker();
          if (!worker) {
            medialode::ipc::FileError fe; fe.request_id = req.request_id; fe.path = req.path; fe.error = "no worker available";
            enqueue(nlohmann::json(fe).dump());
            logj("warn", "no worker available for ScanFileRequest");
          } else {
            state_.map_request(req.request_id, shared_from_this());
            worker->enqueue(j.dump());
            logj("debug", "ScanFileRequest delegated to worker");
          }
        }
      }
      else if (type == "FileError" && role_ == Role::Worker) {
        std::string reqid = j.at("request_id").get<std::string>();
        logj("warn", "received FileError for request " + reqid);
        if (auto client = state_.take_client(reqid)) {
          client->enqueue(j.dump());
        }
      }
      else if (type == "FileScanned" && role_ == Role::Worker) {
        std::string reqid = j.at("request_id").get<std::string>();
        std::string path = j.at("path").get<std::string>();

        logj("debug", "processing FileScanned for " + path);

        auto fut = dbx_.exec([j, path](sqlite3* db) {
          exec_retry(db, "BEGIN;");
          try {
            std::string kind = j.value("kind", "");
            std::int64_t mtime = j.value("mtime", 0);
            std::string folder = fs::path(path).parent_path().string();

            upsert_file(db, path, folder, mtime, "");

            if (kind.empty()) {
              if (j.contains("width") && j.contains("height") && j.contains("channels"))
                kind = "image";
              else if (j.contains("duration") && j.contains("sample_rate"))
                kind = "audio";
              else if (j.contains("duration") && j.contains("width") && j.contains("codec"))
                kind = "video";
              else if (j.contains("language") && j.contains("syntax_ok"))
                kind = "script";
            }

            if (kind == "image") {
              logj("debug", "upserting image metadata for " + path);
              upsert_image_metadata(db, path,
                                    j.at("width").get<int>(),
                                    j.at("height").get<int>(),
                                    j.at("channels").get<int>());
            }
            else if (kind == "audio") {
              logj("debug", "upserting audio metadata for " + path);
              upsert_audio_metadata(db, path,
                                    j.at("duration").get<double>(),
                                    j.at("sample_rate").get<int>(),
                                    j.value("channels", 0),
                                    j.value("bitrate", 0),
                                    j.value("format", std::string("")));
            }
            else if (kind == "video") {
              logj("debug", "upserting video metadata for " + path);
              upsert_video_metadata(db, path,
                                    j.at("duration").get<double>(),
                                    j.at("width").get<int>(),
                                    j.at("height").get<int>(),
                                    j.value("codec", std::string("")),
                                    j.value("framerate", 0.0));
            }
            else if (kind == "script") {
              logj("debug", "upserting script metadata for " + path);
              upsert_script_metadata(db, path,
                                     j.value("language", std::string("unknown")),
                                     j.value("syntax_ok", true),
                                     j.value("error_message", std::string("")));
            }

            upsert_file(db, path, folder, mtime, kind);

            exec_retry(db, "COMMIT;");
          } catch (const std::exception& e) {
            exec_retry(db, "ROLLBACK;");
            logj("error", std::string("DB error: ") + e.what());
            throw;
          }
        });
        fut.wait();

        if (auto client = state_.take_client(reqid)) {
          client->enqueue(j.dump());
          logj("debug", "forwarded FileScanned to client");
        }
      }
      else {
        logj("warn", "unknown msg type: " + type);
      }
    } catch (const std::exception& e) { logj("error", "json:" + std::string(e.what())); }
  }

  void enqueue(std::string payload) {
    bool idle = wq_.empty();
    wq_.push_back(std::move(payload));
    if (idle) do_write();
  }

  void do_write() {
    auto self = shared_from_this();
    ws_.async_write(asio::buffer(wq_.front()), [self](beast::error_code ec, std::size_t) {
      if (ec) { logj("error", "write:" + ec.message()); return; }
      self->wq_.pop_front();
      if (!self->wq_.empty()) self->do_write();
    });
  }
};

// run_server
void run_server(asio::io_context& ioc, std::uint16_t port, const std::string& dbfile) {
  static ServerState state;
  static DBExecutor dbx(open_db(dbfile));
  auto acceptor = std::make_shared<tcp::acceptor>(ioc);
  beast::error_code ec;
  tcp::endpoint ep{tcp::v4(), port};
  acceptor->open(ep.protocol(), ec); acceptor->set_option(asio::socket_base::reuse_address(true));
  acceptor->bind(ep, ec); acceptor->listen(asio::socket_base::max_listen_connections, ec);
  logj("info", "libmgr-server starting on ws://localhost:" + std::to_string(port));
  logj("info", "db=" + dbfile);

  auto do_accept = std::make_shared<std::function<void()>>();
  *do_accept = [acceptor, &dbx, &state, do_accept]() {
    acceptor->async_accept([acceptor, &dbx, &state, do_accept](beast::error_code ec, tcp::socket sock) {
      if (!ec) std::make_shared<WebSocketSession>(std::move(sock), dbx, state)->start();
      if (acceptor->is_open()) (*do_accept)();
    });
  };
  (*do_accept)();

  static std::shared_ptr<asio::signal_set> signals;
  signals = std::make_shared<asio::signal_set>(ioc, SIGINT, SIGTERM);
  signals->async_wait([acceptor](const beast::error_code&, int) {
    logj("info", "shutdown requested");
    beast::error_code ignore; acceptor->close(ignore);
  });
}

// main
int main(int argc, char* argv[]) {
  try {
    std::string dbfile = DEFAULT_DB;
    std::uint16_t port = 8081;
    if (argc > 1) dbfile = argv[1];
    if (argc > 2) port = (std::uint16_t)std::atoi(argv[2]);
    asio::io_context ioc;
    run_server(ioc, port, dbfile);
    ioc.run();
    return 0;
  } catch (const std::exception& e) {
    logj("fatal", e.what());
    return 1;
  }
}


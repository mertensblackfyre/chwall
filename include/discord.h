#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <endian.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

enum class Opcode : uint32_t {
  Handshake = 0,
  Frame = 1,
  Close = 2,
  Ping = 3,
  Pong = 4,
};

enum class ConnectionState : int {
  Disconnected = 0,
  Connected = 1,
  Ready = 2,
};

struct Packet {
  uint32_t op = 0;
  std::string payload;
  size_t len = payload.size();
};

class UnixSocket {
private:
  int fd = -1;
  ConnectionState state = ConnectionState::Disconnected;

public:
  const char *client_id = "1574438305502855208";
  UnixSocket() = default;
  ~UnixSocket() { close(); }

  UnixSocket(const UnixSocket &) = delete;
  UnixSocket &operator=(const UnixSocket &) = delete;

  UnixSocket(UnixSocket &&other) noexcept : fd(std::exchange(other.fd, -1)) {}

  inline bool create();
  inline bool close();
  inline bool send(const Packet *packet);
  inline bool read();
};

bool UnixSocket::create() {

  std::string dir = "/tmp";
  const char *xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");
  if (xdg_runtime_dir != nullptr)
    dir = std::string(xdg_runtime_dir);

  for (int i = 0; i < 10; ++i) {
    std::string path = fmt::format("{}/discord-ipc-{}", dir, i);
    if (path.size() >= sizeof(sockaddr_un::sun_path))
      continue;

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
      fmt::println(stderr, "socket() failed: {}", std::strerror(errno));
      ::close(fd);
      return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);

    if (::connect(fd, reinterpret_cast<const sockaddr *>(&address),
                  sizeof(address)) == 0) {

      this->fd = fd;
      return true;
    }
    ::close(fd);
  }
  return false;
};

bool UnixSocket::read() {

  if (fd == -1) {
    return false;
  }

  char buffer[BUFFER_SIZE];
  ssize_t received = recv(fd, buffer, BUFFER_SIZE, 0);
  if (received > 0) {
    try {
      auto js_payload =
          nlohmann::json::parse(std::string_view(buffer, received));

      if (js_payload.contains("evt") && js_payload["evt"].is_string()) {
        fmt::println("{}", js_payload["evt"].get<std::string>());
      }
    } catch (const nlohmann::json::exception &e) {
      fmt::println("JSON error: {}", e.what());
    }
  } else if (received == 0) {
    fmt::println("peer closed the connection");
    ::close(fd);
  } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
    fmt::println("no data available yet");
  } else {
    perror("recv");
  }
  return true;
}

bool UnixSocket::send(const Packet *packet) {

  size_t offset = 0;
  char *buffer = (char *)calloc(3, (sizeof(uint32_t) * 2) + packet->len);

  if (buffer == nullptr) {
    perror("error calloc()");
    exit(EXIT_FAILURE);
  }

  uint32_t len = static_cast<uint32_t>(packet->payload.size());
  std::memcpy(buffer + offset, &packet->op, sizeof(packet->op));
  offset += sizeof(packet->op);
  std::memcpy(buffer + offset, &len, sizeof(len));
  offset += sizeof(len);
  std::memcpy(buffer + offset, packet->payload.data(), packet->payload.size());
  offset += packet->payload.size();

  ssize_t sent_bytes = ::send(fd, buffer, offset, MSG_NOSIGNAL);
  if (sent_bytes < 0) {
    fmt::println("sent bytes: {}", sent_bytes);
    free(buffer);
    ::close(fd);
    return false;
  }

  fmt::println("sent bytes: {}", sent_bytes);
  return true;
}

bool UnixSocket::close() {
  if (fd < 0)
    return true;
  int rc = ::close(fd);
  fd = -1;
  return rc == 0;
}

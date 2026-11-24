#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

enum class Opcode : uint32_t {
  Handshake = 0,
  Frame = 1,
  Close = 2,
  Ping = 3,
  Pong = 4,
};

#define BUFFER_SIZE 4096
#ifdef MSG_NOSIGNAL
static int MsgFlags = MSG_NOSIGNAL;
#else
static int MsgFlags = 0;
#endif

std::string get_runtime_dir() {
  const char *xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir == nullptr) {
    std::cerr << "Warning: XDG_RUNTIME_DIR not set. Falling back to /tmp."
              << std::endl;
    return "/tmp";
  }

  return std::string(xdg_runtime_dir);
}

void read_response(int fd) {

  if (fd == -1) {
    return;
  }

  char buffer[BUFFER_SIZE];
  ssize_t received = recv(fd, buffer,BUFFER_SIZE, 0);
  if (received > 0) {
    fmt::println("received={}", received);
    fmt::println("payload={}", std::string_view(buffer, received));
  } else if (received == 0) {
    fmt::println("peer closed the connection");
    close(fd);
  } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
    fmt::println("no data available yet");
  } else {
    perror("recv");
  }
}
int connect_discord(const char *dir, const std::string payload) {

  uint32_t opcode = (uint32_t)Opcode::Handshake;
  if (!dir || *dir == '\0')
    return -1;

  for (int i = 0; i < 10; ++i) {
    std::string path = fmt::format("{}/discord-ipc-{}", dir, i);
    if (path.size() >= sizeof(sockaddr_un::sun_path))
      continue;

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
      fmt::println(stderr, "socket() failed: {}", std::strerror(errno));
      return -1;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);

    if (::connect(fd, reinterpret_cast<const sockaddr *>(&address),
                  sizeof(address)) == 0) {

      size_t offset = 0;
      char *buffer = (char *)calloc(3, (sizeof(uint32_t) * 2) + payload.size());
      if (buffer == nullptr) {
        perror("error calloc()");
        return fd;
      }

      size_t len = payload.size();
      std::memcpy(buffer + offset, &opcode, sizeof(opcode));
      offset += sizeof(opcode);
      std::memcpy(buffer + offset, &len, sizeof(len));
      offset += sizeof(len);
      std::memcpy(buffer + offset, &payload, payload.size());
      offset += payload.size();

      ssize_t sent_bytes = ::send(fd, buffer, offset, MsgFlags);
      if (sent_bytes < 0) {
        ::close(fd);
      }
      fmt::println("{}", sent_bytes);
      read_response(fd);
      return fd;
    }

    ::close(fd);
  }

  return -1;
}

int main() {
  std::string runtime_dir = get_runtime_dir();

  std::string client_id = "1574438305502855208";
  std::string js = fmt::format(R"({{"v": 1, "client_id": "{}"}})", client_id);

  int fd = connect_discord(runtime_dir.c_str(), js);
  if (fd >= 0) {
    ::close(fd);
  }

  return 0;
}

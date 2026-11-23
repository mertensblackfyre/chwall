#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

std::string get_runtime_dir() {
  const char *xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir == nullptr) {
    std::cerr << "Warning: XDG_RUNTIME_DIR not set. Falling back to /tmp."
              << std::endl;
    return "/tmp";
  }

  return std::string(xdg_runtime_dir);
}
int connect_discord(const char *dir) {
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
      return fd;
    }

    ::close(fd);
  }

  return -1;
}

int main() {
  std::string runtime_dir = get_runtime_dir();
  int fd = connect_discord(runtime_dir.c_str());
  if (fd >= 0) {
    fmt::println("Connected to Discord IPC on fd {}", fd);
    ::close(fd);
  } else {
    fmt::println("Discord IPC socket not found");
  }
  return 0;
}

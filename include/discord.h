
#include <cstdint>
#include <cstdlib>
#include <fmt/base.h>

#include "connection.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <unistd.h>

#include "utils.h"

inline int discord_rpc_handshake(UnixSocket *sock) {
  sock->create();

  if (sock->state != ConnectionState::Connected) {
    fmt::println("wasnt able to discord rpc");
    return EXIT_FAILURE;
  }

  fmt::println("connected to discord rpc");
  std::string js =
      fmt::format(R"({{"v": 1, "client_id": "{}"}})", sock->client_id);

  Packet *p = (Packet *)sock->packet_builder(js, (uint32_t)Opcode::Handshake);

  sock->send(p);
  sock->read();

  if (sock->state != ConnectionState::Ready) {
    fmt::println("state isnt ready");

    free(p);
    return EXIT_FAILURE;
  }

  free(p);
  return EXIT_SUCCESS;
}

inline int discord_rpc_set_activity(UnixSocket *sock, MPV_INFO info) {
  if (sock->state == ConnectionState::Disconnected) {
    return EXIT_FAILURE;
  }

  MediaActivity m;
  m.title = info.title;
  m.paused = info.paused;
  m.duration_sec = info.duration;
  m.position_sec = info.time_pos;

  if (info.paused == true) {
    m.state_text = "Paused";
  } else if (info.idle_active == true) {
    m.state_text = "Idle";
  } else {

    m.state_text = "Playing";
  }
  std::string js = build_set_activity_json(m);
  Packet *set_activity_js =
      (Packet *)sock->packet_builder(js, (uint32_t)Opcode::Frame);

  sock->send(set_activity_js);
  sock->read();

  free(set_activity_js);

  return EXIT_SUCCESS;
}

#include "../include/connection.h"
#include "../include/discord.h"
#include "../include/utils.h"
#include <cstdlib>
#include <fmt/base.h>
#include <mpv/client.h>
#include <string>

extern "C" int mpv_open_cplugin(mpv_handle *handle) {
  UnixSocket sock;

  mpv_observe_property(handle, PROP_TITLE, "media-title", MPV_FORMAT_STRING);
  mpv_observe_property(handle, PROP_PAUSE, "pause", MPV_FORMAT_FLAG);
  mpv_observe_property(handle, PROP_IDLE, "idle-active", MPV_FORMAT_FLAG);
  mpv_observe_property(handle, PROP_DURATION, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(handle, PROP_TIME_POS, "time-pos", MPV_FORMAT_DOUBLE);

  MPV_INFO info;

  discord_rpc_handshake(&sock);

  while (true) {
    mpv_event *event = mpv_wait_event(handle, -1);
    if (!event)
      continue;

    if (event->event_id == MPV_EVENT_SHUTDOWN)
      break;

    if (event->event_id != MPV_EVENT_PROPERTY_CHANGE)
      continue;

    auto *prop = static_cast<mpv_event_property *>(event->data);
    if (!prop || !prop->data)
      continue;

    switch (event->reply_userdata) {
    case PROP_TITLE:
      if (prop->format == MPV_FORMAT_STRING) {
        auto *s = *static_cast<const char *const *>(prop->data);
        info.title = s ? s : "";
      }
      break;
    case PROP_PAUSE:
      if (prop->format == MPV_FORMAT_FLAG) {
        info.paused = (*static_cast<const int *>(prop->data)) != 0;
      }
      break;
    case PROP_IDLE:
      if (prop->format == MPV_FORMAT_FLAG) {
        info.idle_active = (*static_cast<const int *>(prop->data)) != 0;
      }
      break;
    case PROP_DURATION:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        info.duration = *static_cast<const double *>(prop->data);
      } else if (prop->format == MPV_FORMAT_NONE) {
        info.duration = 0.0;
      }
      break;
    case PROP_TIME_POS:
      if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
        info.time_pos = *static_cast<const double *>(prop->data);
      } else if (prop->format == MPV_FORMAT_NONE) {
        info.time_pos = 0.0;
      }
      break;
    }
    if (sock.state == ConnectionState::Ready) {
      discord_rpc_set_activity(&sock, info);
    }
  }
  return EXIT_SUCCESS;
}

#pragma once

#include <cstdint>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <unistd.h>

enum Props : uint64_t {
  PROP_TITLE = 1,
  PROP_PAUSE = 2,
  PROP_IDLE = 3,
  PROP_DURATION = 4,
  PROP_TIME_POS = 5
};

struct MPV_INFO {
  std::string title;
  bool paused = false, idle_active = false;
  double duration = 0.0, time_pos = 0.0;
} typedef mpv_curr_info;

struct MediaActivity {
  std::string title = "Unknown title";
  std::string state_text = "Unknown state";
  std::string large_image = "N/A";
  std::string large_text = "";
  bool paused = false;
  double position_sec = 0.0;
  double duration_sec = 0.0;
};

inline std::string build_set_activity_json(const MediaActivity &media) {
  static uint64_t nonce_counter = 0;
  const std::string nonce = fmt::format("act-{}", ++nonce_counter);

  const auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

  nlohmann::json activity = {
      {"details", media.title},
      {"state", media.state_text},
  };

  if (!media.paused && media.position_sec >= 0.0) {
    const int64_t pos = static_cast<int64_t>(media.position_sec);
    const int64_t start = static_cast<int64_t>(now_sec) - pos;
    nlohmann::json timestamps = {{"start", start}};

    if (media.duration_sec > media.position_sec) {
      const int64_t dur = static_cast<int64_t>(media.duration_sec);
      timestamps["end"] = start + dur;
    }
    activity["timestamps"] = std::move(timestamps);
  }

  if (!media.large_image.empty()) {
    nlohmann::json assets = {{"large_image", media.large_image}};
    if (!media.large_text.empty()) {
      assets["large_text"] = media.large_text;
    }
    activity["assets"] = std::move(assets);
  }

  nlohmann::json frame = {
      {"cmd", "SET_ACTIVITY"},
      {"nonce", nonce},
      {"args",
       {
           {"pid", ::getpid()},
           {"activity", std::move(activity)},
       }},
  };

  return frame.dump();
}

#include "modules/sway/hide.hpp"
#include <spdlog/spdlog.h>
#include "client.hpp"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

namespace waybar::modules::sway {

Hide::Hide(const std::string& id, const Bar& bar, const Json::Value& config)
    : ALabel(config, "hide", id, "{}", 0, true), bar_(bar), windowId_(-1) {
  ipc_.subscribe(R"(["bar_state_update","barconfig_update"])");
  ipc_.signal_event.connect(sigc::mem_fun(*this, &Hide::onEvent));
  // Launch worker
  worker();
}

void Hide::onEvent(const struct Ipc::ipc_response& res) {
  auto payload = parser_.parse(res.payload);
  mutex_.lock();
  auto &bar = const_cast<Bar &>(bar_);
  if (payload.isMember("mode")) {
    auto mode = payload["mode"].asString();
    if (mode == "hide") {
      // Hide the bars when configuring the "hide" bar
      spdlog::debug("sway/hide: hiding bar(s)");
      bar.setVisible(false);
      bar.setExclusive(false);
    } else if (mode == "dock") {
      spdlog::debug("sway/hide: showing bar(s)");
      bar.setVisible(true);
      bar.setExclusive(true);
    }
  } else if (payload.isMember("visible_by_modifier")) {
    visible_by_modifier_ = payload["visible_by_modifier"].asBool();
    spdlog::debug("sway/hide: visible by modifier: {}", visible_by_modifier_);
    if (visible_by_modifier_) {
        bar.setHiddenClass(false);
        bar.moveToTopLayer();
    } else {
        bar.setHiddenClass(true);
        bar.moveToConfiguredLayer();
        bar.setExclusive(false);
    }
  }
  mutex_.unlock();
}

void Hide::worker() {
  thread_ = [this] {
    try {
      ipc_.handleEvent();
    } catch (const std::exception& e) {
      spdlog::error("Hide: {}", e.what());
    }
  };
}

auto Hide::update() -> void {
}
}  // namespace waybar::modules::sway

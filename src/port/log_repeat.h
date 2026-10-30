// The runtime's log without the same line thousands of times: each logger's sinks behind a filter that writes a
// message's first kFirst times, then only one in kEvery, with the count. Saints Row's draws log 2 invalid texture fetch
// constants each ("has "invalid" type"), some 500 lines a second written and flushed one by one on the GPU's command
// thread (20 MB in five minutes), the thread that sets the pace. Installed by PortApp::OnPostInitLogging
// and again once the GPU plugin has loaded (it registers its category, "gpu", then), before any of the runtime's
// threads start; a logger already filtered is left alone.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <rex/logging.h>
#include <spdlog/sinks/base_sink.h>

namespace port::log_repeat {

constexpr uint32_t kFirst = 20;
constexpr uint32_t kEvery = 10000;

class Filter : public spdlog::sinks::base_sink<std::mutex> {
 public:
  explicit Filter(std::vector<spdlog::sink_ptr> sinks) : sinks_(std::move(sinks)) {}

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    std::string_view text(msg.payload.data(), msg.payload.size());
    if (counts_.size() > 4096) counts_.clear();
    uint32_t n = ++counts_[std::hash<std::string_view>{}(text)];
    if (n <= kFirst) {
      Forward(msg);
      if (n == kFirst) Note(msg, "(the line above repeats: from now on one in 10000 is written)");
    } else if (n % kEvery == 0) {
      Note(msg, std::string(text) + " (repeated " + std::to_string(n) + " times)");
    }
  }

  void flush_() override {
    if (!written_) return;  // a filtered line has nothing to flush: no disk write for it
    written_ = false;
    for (auto& s : sinks_) s->flush();
  }

 private:
  void Forward(const spdlog::details::log_msg& msg) {
    for (auto& s : sinks_) {
      if (s->should_log(msg.level)) s->log(msg);
    }
    written_ = true;
  }

  void Note(const spdlog::details::log_msg& msg, const std::string& text) {
    spdlog::details::log_msg note(msg.time, msg.source, msg.logger_name, msg.level, text);
    note.thread_id = msg.thread_id;
    Forward(note);
  }

  std::vector<spdlog::sink_ptr> sinks_;
  std::unordered_map<size_t, uint32_t> counts_;
  bool written_ = false;
};

inline void Install() {
  // the built-in categories register on their first use, which may come after this: registered now, so their
  // loggers exist to be filtered (a name registers once)
  for (auto id : {rex::log::core(), rex::log::cpu(), rex::log::apu(), rex::log::gpu(), rex::log::krnl(),
                  rex::log::sys(), rex::log::fs()}) {
    (void)id;
  }
  for (const auto& entry : rex::GetAllCategories()) {
    if (!entry.logger) continue;
    auto& sinks = entry.logger->sinks();
    if (sinks.empty() || (sinks.size() == 1 && dynamic_cast<Filter*>(sinks[0].get()))) continue;
    auto filter = std::make_shared<Filter>(sinks);
    sinks.assign(1, filter);
  }
}

}  // namespace port::log_repeat

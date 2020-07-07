#include "multi_io_context.h"

#include "util/log.h"

namespace librevault {

multi_io_context::multi_io_context(QString name) : name_(std::move(name)) {}
multi_io_context::~multi_io_context() { stop(false); }

void multi_io_context::start() {
  ctx_work_ = std::make_unique<boost::asio::io_context::work>(ctx_);
  worker_thread_ = std::make_unique<std::thread>([this] { run_thread(); });  // Running io_context in threads
}

void multi_io_context::stop(bool stop_gently) {
  ctx_work_.reset();

  if (!stop_gently) ctx_.stop();
  if (worker_thread_->joinable()) worker_thread_->join();
}

void multi_io_context::run_thread() {
  LOGD("Asio thread started");
  ctx_.run();
  LOGD("Asio thread started");
}

}  // namespace librevault

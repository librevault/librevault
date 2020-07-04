#include "BandwidthCounter.h"

namespace librevault {

BandwidthCounter::BandwidthCounter() { last_heartbeat_.start(); }

BandwidthCounter::Stats BandwidthCounter::heartbeat() {
  qreal period = qreal(last_heartbeat_.restart()) / 1000;

  return {.down_bytes = down_bytes_,
          .down_bytes_blocks = down_bytes_blocks_,
          .down_bandwidth = qreal(down_bytes_last_.exchange(0)) / period,
          .down_bandwidth_blocks = qreal(down_bytes_blocks_last_.exchange(0)) / period,
          .up_bytes = up_bytes_,
          .up_bytes_blocks = up_bytes_blocks_,
          .up_bandwidth = qreal(up_bytes_last_.exchange(0)) / period,
          .up_bandwidth_blocks = qreal(up_bytes_blocks_last_.exchange(0)) / period};
}

QJsonObject BandwidthCounter::heartbeat_json() {
  BandwidthCounter::Stats traffic_stats = heartbeat();
  return {
      {"up_bandwidth", traffic_stats.up_bandwidth},     {"up_bandwidth_blocks", traffic_stats.up_bandwidth_blocks},
      {"down_bandwidth", traffic_stats.down_bandwidth}, {"down_bandwidth_blocks", traffic_stats.down_bandwidth_blocks},
      {"up_bytes", qint64(traffic_stats.up_bytes)},     {"up_bytes_blocks", qint64(traffic_stats.up_bytes_blocks)},
      {"down_bytes", qint64(traffic_stats.down_bytes)}, {"down_bytes_blocks", qint64(traffic_stats.down_bytes_blocks)}};
}

void BandwidthCounter::add_down(quint64 bytes) {
  down_bytes_ += bytes;
  down_bytes_last_ += bytes;
}

void BandwidthCounter::add_down_blocks(quint64 bytes) {
  down_bytes_blocks_ += bytes;
  down_bytes_blocks_last_ += bytes;
}

void BandwidthCounter::add_up(quint64 bytes) {
  up_bytes_ += bytes;
  up_bytes_last_ += bytes;
}

void BandwidthCounter::add_up_blocks(quint64 bytes) {
  up_bytes_blocks_ += bytes;
  up_bytes_blocks_last_ += bytes;
}

}  // namespace librevault

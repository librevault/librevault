#pragma once
#include <QElapsedTimer>
#include <QJsonObject>
#include <atomic>

namespace librevault {

class BandwidthCounter {
 public:
  struct Stats {
    // Download
    quint64 down_bytes;
    quint64 down_bytes_blocks;

    qreal down_bandwidth;
    qreal down_bandwidth_blocks;

    // Upload
    quint64 up_bytes;
    quint64 up_bytes_blocks;

    qreal up_bandwidth;
    qreal up_bandwidth_blocks;
  };

  BandwidthCounter();

  Stats heartbeat();
  QJsonObject heartbeat_json();

  void add_down(quint64 bytes);
  void add_down_blocks(quint64 bytes);
  void add_up(quint64 bytes);
  void add_up_blocks(quint64 bytes);

 private:
  QElapsedTimer last_heartbeat_;

  // Download
  std::atomic<quint64> down_bytes_ = 0;
  std::atomic<quint64> down_bytes_blocks_ = 0;

  std::atomic<quint64> down_bytes_last_ = 0;
  std::atomic<quint64> down_bytes_blocks_last_ = 0;

  // Upload
  std::atomic<quint64> up_bytes_ = 0;
  std::atomic<quint64> up_bytes_blocks_ = 0;

  std::atomic<quint64> up_bytes_last_ = 0;
  std::atomic<quint64> up_bytes_blocks_last_ = 0;
};

}  // namespace librevault

#pragma once
#include <QElapsedTimer>
#include <QJsonObject>
#include <atomic>

namespace librevault {

class BandwidthCounter {
 public:
  struct Stats {
    // Download
    quint64 down_bytes_;
    quint64 down_bytes_blocks_;

    qreal down_bandwidth_;
    qreal down_bandwidth_blocks_;

    // Upload
    quint64 up_bytes_;
    quint64 up_bytes_blocks_;

    qreal up_bandwidth_;
    qreal up_bandwidth_blocks_;
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
  std::atomic<quint64> down_bytes_;
  std::atomic<quint64> down_bytes_blocks_;

  std::atomic<quint64> down_bytes_last_;
  std::atomic<quint64> down_bytes_blocks_last_;

  // Upload
  std::atomic<quint64> up_bytes_;
  std::atomic<quint64> up_bytes_blocks_;

  std::atomic<quint64> up_bytes_last_;
  std::atomic<quint64> up_bytes_blocks_last_;
};

}  // namespace librevault

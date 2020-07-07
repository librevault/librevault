#pragma once

#include <QJsonObject>
#include <QStatusBar>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

class Daemon;
class StatusBar : public QObject {
  Q_OBJECT

 public:
  StatusBar(QStatusBar* bar, Daemon* daemon);
  ~StatusBar() override;

 public slots:
  void refresh();

 private:
  QStatusBar* bar_;
  Daemon* daemon_;

  QWidget* container_;
  QHBoxLayout* container_layout_;
  QLabel* dht_label_;
  QLabel* down_label_;
  QLabel* up_label_;

  QFrame* create_separator() const;

 private slots:
  void refreshBandwidth(float up_bandwidth, float down_bandwidth, double up_bytes, double down_bytes);

  void showDHTMenu(const QPoint& pos);
};

#include "StatusBar.h"

#include <QtWidgets/QMenu>

#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "icons/GUIIconProvider.h"
#include "util/human_size.h"

StatusBar::StatusBar(QStatusBar* bar, Daemon* daemon) : QObject(bar), bar_(bar), daemon_(daemon) {
  container_ = new QWidget(bar_);
  container_layout_ = new QHBoxLayout(container_);
  container_layout_->setContentsMargins(0, 0, 0, 0);

  dht_label_ = new QLabel(container_);
  dht_label_->setContextMenuPolicy(Qt::CustomContextMenu);
  container_layout_->addWidget(dht_label_);

  container_layout_->addWidget(create_separator());

  down_label_ = new QLabel(container_);
  container_layout_->addWidget(down_label_);

  container_layout_->addWidget(create_separator());

  up_label_ = new QLabel(container_);
  container_layout_->addWidget(up_label_);

  bar->addPermanentWidget(container_);

  // Connecting signals
  connect(daemon_->state(), &RemoteState::changed, this, &StatusBar::refresh);
  connect(daemon_->config(), &RemoteConfig::changed, this, &StatusBar::refresh);

  connect(dht_label_, &QLabel::customContextMenuRequested, this, &StatusBar::showDHTMenu);

  // Setting defaults
  refresh();
}

StatusBar::~StatusBar() {}

void StatusBar::refresh() {
  /* Refresh DHT */
  if (daemon_->config()->getGlobal("mainline_dht_enabled").toBool()) {
    dht_label_->setText(tr("DHT: %n nodes", "DHT", daemon_->state()->getGlobalValue("dht_nodes_count").toInt()));
  } else {
    dht_label_->setText(tr("DHT: disabled", "DHT"));
  }

  /* Refresh bandwidth */
  float up_bandwidth = 0;
  float down_bandwidth = 0;
  double up_bytes = 0;
  double down_bytes = 0;

  for (auto& folderid : daemon_->state()->folderList()) {
    QJsonObject traffic_stats = daemon_->state()->getFolderValue(folderid, "traffic_stats").toObject();
    up_bandwidth += traffic_stats["up_bandwidth"].toDouble();
    down_bandwidth += traffic_stats["down_bandwidth"].toDouble();
    up_bytes += traffic_stats["up_bytes"].toDouble();
    down_bytes += traffic_stats["down_bytes"].toDouble();
  }
  refreshBandwidth(up_bandwidth, down_bandwidth, up_bytes, down_bytes);
}

QFrame* StatusBar::create_separator() const {
  QFrame* separator = new QFrame(container_);
  separator->setFrameStyle(QFrame::VLine);
  return separator;
}

void StatusBar::refreshBandwidth(float up_bandwidth, float down_bandwidth, double up_bytes, double down_bytes) {
  up_label_->setText(QStringLiteral("\u2191 %1 (%2)").arg(human_bandwidth(up_bandwidth)).arg(human_size(up_bytes)));
  down_label_->setText(
      QStringLiteral("\u2193 %1 (%2)").arg(human_bandwidth(down_bandwidth)).arg(human_size(down_bytes)));
}

void StatusBar::showDHTMenu(const QPoint& pos) {
  QMenu context_menu("DHT", bar_);

  QAction dht_enabled(tr("Enable DHT"), bar_);
  dht_enabled.setCheckable(true);
  dht_enabled.setChecked(daemon_->config()->getGlobal("mainline_dht_enabled").toBool());
  connect(&dht_enabled, &QAction::toggled,
          [this](bool checked) { daemon_->config()->setGlobal("mainline_dht_enabled", checked); });
  context_menu.addAction(&dht_enabled);

  context_menu.exec(dht_label_->mapToGlobal(pos));
}

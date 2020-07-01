#pragma once
#include <QObject>
#include <QSettings>

class StartupInterface : public QObject {
  Q_OBJECT

 public:
  StartupInterface(QObject* parent = 0);
  ~StartupInterface();

  bool isSupported();
  bool isEnabled() const;

 public slots:
  void setEnabled(bool enabled) {
    if (enabled)
      enable();
    else
      disable();
  };
  void enable();
  void disable();

 protected:
  void* interface_impl_;
};

#pragma once
#include <QObject>

class Updater : public QObject {
  Q_OBJECT

 public:
  Updater(QObject* parent);
  virtual ~Updater();

  bool supportsUpdate() const;
  bool enabled() const;

 public slots:
  void checkUpdates();
  void checkUpdatesSilently();
  void setEnabled(bool enable);

 private:
  struct Impl;
  Impl* impl_;
};

#pragma once
#include <QObject>

class TestBase58 : public QObject {
  Q_OBJECT

 private slots:
  void testEncode();
  void testDecode();
};

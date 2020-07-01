#pragma once
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

class Translator : public QObject {
  Q_OBJECT

 public:
  Translator(QCoreApplication* app);

  QString getLocaleSetting();
  QMap<QString, QString> availableLocales();

 public slots:
  void setLocale(QString locale);
  void applyLocale();

 private:
  QCoreApplication* app_;

  // Translation
  QTranslator translator_;
  QTranslator qt_translator_;
};

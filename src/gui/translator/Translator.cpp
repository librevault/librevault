#include "Translator.h"

#include <QLibraryInfo>
#include <QSettings>

Translator::Translator(QCoreApplication* app) : QObject(app), app_(app) { applyLocale(); }

QString Translator::getLocaleSetting() {
  QSettings lang_settings;
  return lang_settings.value("locale", QStringLiteral("auto")).toString();
}

QMap<QString, QString> Translator::availableLocales() {
  return {{"en", "English"}, {"ru", QString::fromUtf16(u"\u0420\u0443\u0441\u0441\u043A\u0438\u0439")}};
}

void Translator::setLocale(QString locale) {
  QSettings lang_settings;
  lang_settings.setValue("locale", locale);
  applyLocale();
}

void Translator::applyLocale() {
  QSettings lang_settings;
  QString locale = lang_settings.value("locale", QStringLiteral("auto")).toString();
  if (locale == "auto") locale = QLocale::system().name();

  qt_translator_.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  app_->installTranslator(&qt_translator_);
  translator_.load(QStringLiteral(":/lang/librevault_") + locale);
  app_->installTranslator(&translator_);
}

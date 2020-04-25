/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
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

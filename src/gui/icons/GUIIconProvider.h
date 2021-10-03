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
 */
#pragma once
#include <QtGui>
#ifdef Q_OS_WIN
#include <QtWin>
#endif

class GUIIconProvider {
 public:
  enum ICON_ID {
    SETTINGS_GENERAL,
    SETTINGS_ACCOUNT,
    SETTINGS_NETWORK,
    SETTINGS_ADVANCED,

    FOLDER_ADD,
    FOLDER_DELETE,
    LINK_OPEN,
    SETTINGS,
    TRAYICON,
    REVEAL_FOLDER
  };

  QIcon icon(ICON_ID id) const;

#ifdef Q_OS_MAC
  class MacIcon : public QIconEngine {
   public:
    MacIcon(const QString& name) : QIconEngine(), name_(name) {}
    virtual void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
                       QIcon::State state) override { /* no-op */
    }
    virtual QIconEngine* clone() const override { /* no-op */
      return nullptr;
    }

   private:
    QString name_;
    virtual QString iconName() const override { return name_; }
  };
#endif

#ifdef Q_OS_WIN
  static QIcon get_shell_icon(LPCTSTR dll, WORD num) {
    return QIcon(QtWin::fromHICON(LoadIcon(LoadLibrary(dll), MAKEINTRESOURCE(num))));
  }
#endif
};

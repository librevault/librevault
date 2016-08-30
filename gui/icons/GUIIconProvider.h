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
#pragma once
#include "pch.h"
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
		TRAYICON
	};

	QIcon get_icon(ICON_ID id) const;

	static GUIIconProvider* get_instance() {
		static GUIIconProvider* provider = nullptr;
		if(provider == nullptr)
			provider = new GUIIconProvider();
		return provider;
	}

protected:
	GUIIconProvider();

#ifdef Q_OS_MAC
	class MacIcon : public QIconEngine {
	public:
		MacIcon(const QString& name) : QIconEngine(), name_(name) {}
		virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override {/* no-op */}
		virtual QIconEngine* clone() const override {/* no-op */ return nullptr;}
	private:
		QString name_;
		virtual QString iconName() const override {return name_;}
	};
#endif

#ifdef Q_OS_WIN
	static QIcon get_shell_icon(LPCTSTR dll, WORD num) {
		return QIcon(QtWin::fromHICON(LoadIcon(LoadLibrary(dll), MAKEINTRESOURCE(num))));
	}
#endif
};

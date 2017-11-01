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
#include <QJsonDocument>
#include <QObject>
#include <QVariant>

namespace librevault {

class AbstractConfig : public QObject {
	Q_OBJECT

signals:
	void globalChanged(QString key, QVariant value);
	void folderAdded(QVariantMap fconfig);
	void folderRemoved(QByteArray folderid);

public:
	/* Global configuration */
	virtual QVariant getGlobal(QString name) = 0;
	virtual void setGlobal(QString name, QVariant value) = 0;

	/* Folder configuration */
	virtual void addFolder(QVariantMap fconfig) = 0;
	virtual void removeFolder(QByteArray folderid) = 0;

	virtual QVariantMap getFolder(QByteArray folderid) = 0;
	virtual QVariant getFolderValue(QByteArray folderid, QString name) {return getFolder(folderid).value(name);}
	virtual QList<QByteArray> listFolders() = 0;

	/* Export/Import */
	virtual QJsonDocument exportGlobals() = 0;
	virtual QJsonDocument exportFolders() = 0;
};

} /* namespace librevault */

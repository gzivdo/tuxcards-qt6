/***************************************************************************
                          configgroup.h  -  description
                             -------------------
    begin                : Fri Apr 28 2000
    copyright            : (C) 2000 by Alexander Theel
    email                : alex.theel@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONFIGGROUP_H
#define CONFIGGROUP_H

#include <QString>
#include <QList>
#include <QPair>

class ConfigGroup {
public:
	ConfigGroup(QString name);
	~ConfigGroup();

	QString getName();

	void addEntry(QString, QString);
	void changeEntry(QString, QString);
	QString getValue(QString);

	QString toString();

protected:
	QString name;
	QList<QPair<QString, QString>> entries;
};

#endif

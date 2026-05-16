/***************************************************************************
                          configgroup.cpp  -  description
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

#include "../../global.h"
#include "configgroup.h"
#include <iostream>

ConfigGroup::ConfigGroup(QString groupName)
{
	name = groupName;
}


ConfigGroup::~ConfigGroup() {}

QString ConfigGroup::getName(){ return name; }

void ConfigGroup::addEntry(QString name, QString value){
	entries.append(qMakePair(name, value));
}

void ConfigGroup::changeEntry(QString name, QString value){
	for (int i = 0; i < entries.size(); ++i) {
		if (entries[i].first == name) {
			entries[i].second = value;
			return;
		}
	}
	addEntry(name, value);
}

/**
 * returns the value of the entry 'name'
 * if it is not found '-1none' is returned
 */
QString ConfigGroup::getValue(QString name){
	for (const auto& kv : entries) {
		if (kv.first == name)
			return kv.second;
	}
	return QString("-1none");
}

QString ConfigGroup::toString(){
	QString s = "[" + name + "]\n";
	for (const auto& kv : entries) {
		s.append(kv.first);
		s.append("=");
		s.append(kv.second);
		s.append("\n");
	}
	return s;
}

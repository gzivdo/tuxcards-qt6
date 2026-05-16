/***************************************************************************
                          configparser.cpp  -  description
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
#include "configparser.h"
#include <iostream>

ConfigParser::ConfigParser(QString fileName, bool writeAtOnce){
	this->fileName=fileName;
	this->writeAtOnce=writeAtOnce;

	ready=false; currentGroup=0;

	QFile f(fileName);
	if(f.exists() && f.open(QIODevice::ReadOnly) ) {
		QTextStream t( &f );
		QString s;

		while ( !t.atEnd() ) {
			QString s=t.readLine();

			if( !((s.left(1)=="#") || (s.left(1)==" ")) ){
				if (s.left(1)=="["){
					int i=s.indexOf(']');
					if (i==-1) i=s.length()-1;

					ready=true;
					currentGroup=new ConfigGroup(s.mid(1, i-1));

					list.append(currentGroup);
				}else{
					if(currentGroup){
						int i=s.indexOf('=');
						if (i==-1) i=s.length()-2;

						currentGroup->addEntry( s.left(i), s.mid(i+1) );
					}
				}
 			}
		}
		f.close();
	}
}

ConfigParser::~ConfigParser(){
	qDeleteAll(list);
	list.clear();
	currentGroup = nullptr;
}

bool ConfigParser::correct(){ return ready; }

void ConfigParser::setGroup(QString g){
	ConfigGroup* found = nullptr;
	for (ConfigGroup* x : list) {
		if (x->getName() == g) { found = x; break; }
	}

	if(!found){
		found = new ConfigGroup(g);
		list.append(found);
		if (writeAtOnce) writeChanges();
	}
	currentGroup = found;
}

QString ConfigParser::getCurrentGroup(){
	return currentGroup->getName();
}

QString ConfigParser::readEntry(QString name, QString alternative){
	QString s=currentGroup->getValue(name);
	return (s=="-1none" ? alternative : s);
}

int ConfigParser::readNumEntry(QString name, int alternative){
	QString s=currentGroup->getValue(name);
	return (s=="-1none" ? alternative : s.toInt() );
}

void ConfigParser::changeEntry(QString name, QString value){
	currentGroup->changeEntry(name, value);

	if (writeAtOnce) writeChanges();
}

void ConfigParser::changeEntry(QString name, int value){
	currentGroup->changeEntry(name, QString::number(value));

	if (writeAtOnce) writeChanges();
}

void ConfigParser::writeChanges(){
	QFile f(fileName);

	if ( f.open(QIODevice::WriteOnly) ) {
		QTextStream t( &f );
		t<<toString();

		f.close();
	}
}


QString ConfigParser::toString(){
  QString s="#\n"
            "# TuxCards Configuration File\n"
            "#\n";
	for (ConfigGroup* x : list)
		s.append(x->toString());

	return s;
}

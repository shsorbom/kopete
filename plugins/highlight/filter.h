/***************************************************************************
                    filter.h  -  filter for the highlight plugin
                             -------------------
    begin                : mar 14 2003
    copyright            : (C) 2003 by Olivier Goffart
    email                : ogoffart@tiscalinet.be
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILTER_H
#define FILTER_H


#include <qstring.h>
#include <qcolor.h>

/**
 * @author Olivier Goffart <ogoffart@tiscalinet.be>
 **/
class Filter
{
public:
    Filter();
    ~Filter();

	QString displayName;
	QString search;
	bool caseSensitive;
	bool isRegExp;
	
	bool setImportance;
	unsigned int importance;
	
	bool setFG;
	QColor FG;
	
	bool setBG;
	QColor BG;
	
	bool playSound;
	QString soundFN;
};

#endif

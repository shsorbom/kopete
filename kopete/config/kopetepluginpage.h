/*
    kopetepluginpage.h - Kopete Plugin Loader KCM

    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2001-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEPLUGINPAGE_H
#define KOPETEPLUGINPAGE_H

#include <ksettings/pluginpage.h>

/**
 * Plugin selector. See KSettings::PluginPage in kdelibs for documentation.
 *
 * @author Martijn Klingens <klingens@kde.org>
 */
class KopetePluginConfig : public KSettings::PluginPage
{

public:
	KopetePluginConfig( QWidget *parent, const char *name, const QStringList &args );
};

#endif // KOPETEPLUGINPAGE_H

// vim: set noet ts=4 sts=4 sw=4:


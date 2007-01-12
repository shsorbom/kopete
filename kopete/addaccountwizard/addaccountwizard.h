/*
    addaccountwizard.h - Kopete Add Account Wizard

    Copyright (c) 2003      by Olivier Goffart       <ogoffart@kde.org>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef ADDACCOUNTWIZARD_H
#define ADDACCOUNTWIZARD_H

#include <qmap.h>

#include <kassistantdialog.h>

#include <kopete_export.h>

#include "ui_addaccountwizardpage1.h"
#include "ui_addaccountwizardpage2.h"

class QTreeWidgetItem;

class KPluginInfo;
class KVBox;
class KPageWidgetItem;

namespace Kopete
{
class Protocol;
}

class KopeteEditAccountWidget;

/**
 * @author Olivier Goffart <ogoffart@kde.org>
 */
class KOPETE_EXPORT AddAccountWizard : public KAssistantDialog
{
	Q_OBJECT

public:
	AddAccountWizard( QWidget *parent = 0, bool firstRun = false );
	~AddAccountWizard();

private slots:
	void slotProtocolListClicked();
	void slotProtocolListDoubleClicked();

protected slots:
	virtual void back();
	virtual void next();
	virtual void accept();
	virtual void reject();

private:
	class Private;
	Private *d;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:


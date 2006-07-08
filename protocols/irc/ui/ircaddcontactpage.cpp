/*
    ircaddcontactpage.cpp - IRC Add Contact Widget

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>

    Kopete    (c) 2002      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "ircadd.h"
#include "ircaddcontactpage.h"
#include "channellist.h"

#include "kircengine.h"

#include "ircaccount.h"

#include <qlayout.h>
#include <qlineedit.h>
#include <q3frame.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

class IRCAddContactPagePrivate
{
public:
//	ircdata
	ChannelList *search;
	IRCAccount *account;
};

IRCAddContactPage::IRCAddContactPage( QWidget *parent, IRCAccount *a ) : AddContactPage(parent, 0)
{
	(new Q3VBoxLayout(this))->setAutoAdd(true);
//	ircdata = new Ui::ircAddUI(this);
//	d->search = new ChannelList( (QWidget*)ircdata->hbox, a->engine() );
	d->account = a;

	connect( d->search, SIGNAL( channelSelected( const QString & ) ),
		this, SLOT( slotChannelSelected( const QString & ) ) );

	connect( d->search, SIGNAL( channelDoubleClicked( const QString & ) ),
		this, SLOT( slotChannelDoubleClicked( const QString & ) ) );
}

IRCAddContactPage::~IRCAddContactPage()
{
}

void IRCAddContactPage::slotChannelSelected( const QString &channel )
{
//	ircdata->addID->setText( channel );
}

void IRCAddContactPage::slotChannelDoubleClicked( const QString &channel )
{
//	ircdata->addID->setText( channel );
//	ircdata->tabWidget3->setCurrentPage(0);
}

bool IRCAddContactPage::apply(Kopete::Account *account , Kopete::MetaContact *m)
{
//	QString name = ircdata->addID->text();
//	return account->addContact(name, m, Kopete::Account::ChangeKABC );
	return false;
}

bool IRCAddContactPage::validateData()
{
/*
	QString name = ircdata->addID->text();
	if (name.isEmpty() == true)
	{
		KMessageBox::sorry(this, i18n("<qt>You need to specify a channel to join, or query to open.</qt>"), i18n("You Must Specify a Channel"));
		return false;
	}
	return true;
*/
	return false;
}

#include "ircaddcontactpage.moc"


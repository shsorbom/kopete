/*  *************************************************************************
    *   copyright: (C) 2003 Richard L�rk�ng <nouseforaname@home.se>         *
    *   copyright: (C) 2003 Gav Wood <gav@kde.org>                          *
    *************************************************************************
*/

/*  *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#undef KDE_NO_COMPAT
#include <kaction.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "smsaccount.h"
#include "smsprotocol.h"
#include "smscontact.h"

SMSAccount::SMSAccount( SMSProtocol *parent, const QString &accountID, const char *name )
	: KopeteAccount( parent, accountID, name )
{
	setMyself( new SMSContact(this, accountID, accountID, 0L) );
}

SMSAccount::~SMSAccount()
{
}

void SMSAccount::loaded()
{
	loadConfig();
}

void SMSAccount::loadConfig()
{
	theSubEnable = pluginData(protocol(), "SubEnable") == "true";
	theSubCode = pluginData(protocol(), "SubCode");
	theLongMsgAction = (SMSMsgAction)pluginData(protocol(), "MsgAction").toInt();
}

void SMSAccount::translateNumber(QString &theNumber)
{
	if(theNumber[0] == QChar('0') && theSubEnable)
		theNumber.replace(0, 1, theSubCode);
}

const bool SMSAccount::splitNowMsgTooLong(int max, int msgLength)
{
	if(theLongMsgAction == ACT_CANCEL) return false;
	if(theLongMsgAction == ACT_SPLIT) return true;
	if(KMessageBox::questionYesNo(0L, i18n("This message is longer than the maximum length (%1). Should it be divided to %2 messages?").arg(max).arg(msgLength / max + 1),
		i18n("Message Too Long")) == KMessageBox::Yes)
		return true;
	else
		return false;
}

void SMSAccount::setAway( bool /*away*/, const QString &)
{
}

void SMSAccount::connect()
{
//	myself()->setOnlineStatus( SMSOnline );

	// FIXME: Set all contacts to SMSUnknown here
}

KActionMenu* SMSAccount::actionMenu()
{
	KActionMenu *theActionMenu = new KActionMenu(accountId(), myself()->onlineStatus().iconFor(this) , this);
	theActionMenu->popupMenu()->insertTitle(myself()->icon(), i18n("SMS (%1)").arg(accountId()));

	return theActionMenu;
}

void SMSAccount::disconnect()
{
//	myself()->setOnlineStatus( SMSOffline );

	// FIXME: Set all contacts to SMSOffline here
}

bool SMSAccount::addContactToMetaContact( const QString &contactId, const QString &displayName,
	KopeteMetaContact * parentContact )
{
	if (new SMSContact(this, contactId, displayName, parentContact))
		return true;
	else
		return false;
}

#include "smsaccount.moc"

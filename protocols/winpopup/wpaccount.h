/***************************************************************************
     wpaccount.h  -  Base class for the Kopete WP account
                             -------------------
    begin                : Fri Apr 26 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@indigoarchive.net

    Based on code from   : (C) 2002 by Duncan Mac-Vicar Prett
    email                : duncan@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __WPACCOUNT_H
#define __WPACCOUNT_H


// QT Includes
#include <qpixmap.h>

// KDE Includes

// Kopete Includes
#include "kopetemetacontact.h"
#include "kopeteaccount.h"
#include "kopeteonlinestatus.h"

// Local Includes
#include "libwinpopup.h"
#include "wpcontact.h"
#include "wpaddcontact.h"

class KPopupMenu;
class KActionMenu;
class KAction;
class WPContact;
class WPProtocol;
class KopeteWinPopup;

/**
 * The actual Account class used by Kopete.
 */
class WPAccount : public KopeteAccount
{
	Q_OBJECT

// KopeteAccount overloading
public:
	WPAccount(WPProtocol *parent, const QString& accountID, const char *name = 0);
	~WPAccount();

	virtual KActionMenu* actionMenu();			// Per-protocol actions for the systray and the status bar
	virtual void setAway(bool status, const QString &);	// Set user away

public slots:
	virtual void connect();						// Connect to server
	virtual void disconnect();					// Disconnect from server

	void goAvailable() { setAway(false, QString::null); }		// Two convenience slots
	void goAway() { setAway(true, QString::null); }			// for available and away

// Stuff used internally & by colleague classes
public:
	const QStringList getGroups();
	const QStringList getHosts(const QString &Group);
	const QStringList getHostDetails( const QString &Host );

// Stuff used by WPContact
public:
	/**
	 * Returns whether or not the named host is online.
	 */
	bool checkHost(const QString &Name);

public slots:
	/**
	 * Dispatches said message to the destination.
	 */
	void slotSendMessage(const QString &Body, const QString &Destination);

protected:
	virtual bool addContactToMetaContact(const QString &contactId, const QString &displayName, KopeteMetaContact *parentContact);

private slots:
	/**
	 * Called when a new message arrives with the message's data.
	 */
	void slotGotNewMessage(const QString &Body, const QDateTime &Arrival, const QString &From);
//	void updateAccountId();

private:
	QString theAwayMessage;			// The message to give when the user is away
	KopeteWinPopup *theInterface;		// Our KopeteWinPopup instance
};

#endif


/*
    yahooconferencemessagemanager.h - Yahoo Conference Message Manager

    Copyright (c) 2003 by Duncan Mac-Vicar <duncan@kde.org>
    Copyright (c) 2005 by André Duffeck        <andre@duffeck.de>

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kconfig.h>

#include <kopetecontactaction.h>
#include <kopetecontactlist.h>
#include <kopetecontact.h>
#include <kopetechatsessionmanager.h>
#include <kopeteuiglobal.h>
#include <kicon.h>

#include "yahooconferencemessagemanager.h"
#include "yahoocontact.h"
#include "yahooaccount.h"
#include "yahooinvitelistimpl.h"
#include <kactioncollection.h>

YahooConferenceChatSession::YahooConferenceChatSession( const QString & yahooRoom, Kopete::Protocol *protocol, const Kopete::Contact *user,
	Kopete::ContactPtrList others )
: Kopete::ChatSession( user, others, protocol )
{

	Kopete::ChatSessionManager::self()->registerChatSession( this );
	setComponentData(protocol->componentData());

	connect ( this, SIGNAL( messageSent ( Kopete::Message &, Kopete::ChatSession * ) ),
			  SLOT( slotMessageSent ( Kopete::Message &, Kopete::ChatSession * ) ) );

	m_yahooRoom = yahooRoom;

	m_actionInvite = new KAction( KIcon("kontact_contacts"), i18n( "&Invite others" ), this );
        actionCollection()->addAction( "yahooInvite", m_actionInvite );
	connect ( m_actionInvite, SIGNAL( triggered ( bool ) ), this, SLOT( slotInviteOthers() ) );

	setXMLFile("yahooconferenceui.rc");
}

YahooConferenceChatSession::~YahooConferenceChatSession()
{
	emit leavingConference( this );
}

YahooAccount *YahooConferenceChatSession::account()
{
	return static_cast< YahooAccount *>( Kopete::ChatSession::account() );
}

const QString &YahooConferenceChatSession::room()
{
	return m_yahooRoom;
}

void YahooConferenceChatSession::joined( YahooContact *c )
{
	addContact( c );
}

void YahooConferenceChatSession::left( YahooContact *c )
{
	removeContact( c );
}

void YahooConferenceChatSession::slotMessageSent( Kopete::Message & message, Kopete::ChatSession * )
{
	kDebug ( YAHOO_GEN_DEBUG ) << k_funcinfo;

	YahooAccount *acc = dynamic_cast< YahooAccount *>( account() );
	if( acc )
		acc->sendConfMessage( this, message );
	appendMessage( message );
	messageSucceeded();
}

void YahooConferenceChatSession::slotInviteOthers()
{
	QStringList buddies;

	QHash<QString, Kopete::Contact*>::ConstIterator it, itEnd = account()->contacts().constEnd();
	Kopete::Contact *myself = account()->myself();
	for( it = account()->contacts().constBegin(); it != itEnd; ++it )
	{
		if( it.value() != myself && !members().contains( it.value() ) )
			buddies.push_back( it.value()->contactId() );
	}

	YahooInviteListImpl *dlg = new YahooInviteListImpl( Kopete::UI::Global::mainWidget() );
	QObject::connect( dlg, SIGNAL( readyToInvite( const QString &, const QStringList &, const QStringList &, const QString & ) ),
				account(), SLOT( slotAddInviteConference( const QString &, const QStringList &, const QStringList &, const QString & ) ) );
	dlg->setRoom( m_yahooRoom );
	dlg->fillFriendList( buddies );
	for( QList<Kopete::Contact*>::ConstIterator it = members().constBegin(); it != members().constEnd(); it++ )
		dlg->addParticipant( (*it)->contactId() );
	dlg->show();
}

#include "yahooconferencemessagemanager.moc"

// vim: set noet ts=4 sts=4 sw=4:


/*
    meanwhileprotocol.cpp - the meanwhile protocol 

    Copyright (c) 2003-2004 by Sivaram Gottimukkala  <suppandi@gmail.com>

    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#include "meanwhileprotocol.h"
#include "meanwhileaddcontactpage.h"
#include "meanwhileeditaccountwidget.h"
#include "meanwhileserver.h"
#include "meanwhileaccount.h"
#include <kgenericfactory.h>
#include "kopeteaccountmanager.h"
#include "kopeteglobal.h"

MeanwhileProtocol *MeanwhileProtocol::s_protocol = 0L;

typedef KGenericFactory<MeanwhileProtocol> MeanwhileProtocolFactory;
K_EXPORT_COMPONENT_FACTORY( 
                kopete_meanwhile, 
                MeanwhileProtocolFactory( "kopete_meanwhile" ))

MeanwhileProtocol::MeanwhileProtocol( QObject* parent, 
                                      const char *name, 
                                      const QStringList &/*args*/)
    : Kopete::Protocol(MeanwhileProtocolFactory::instance(), 
                     parent, name ),
      meanwhileOffline( Kopete::OnlineStatus::Offline, 
                        25, this, 0, QString::null,
                        i18n( "Go Offline" ),
                        i18n( "Offline" ) ),
      meanwhileOnline( Kopete::OnlineStatus::Online, 
                        25, this, 1, QString::null,
                        i18n( "Go Online" ),
                        i18n( "Online" ) ),
      meanwhileAway( Kopete::OnlineStatus::Away, 
                        25, this, 2, "meanwhile_away",
                        i18n( "Go Away" ),
                        i18n( "Away" ) ),
      meanwhileBusy( Kopete::OnlineStatus::Away, 
                        25, this, 3, "meanwhile_dnd",
                        i18n( "Mark as Busy" ),
                        i18n( "Busy" ) ),
      meanwhileIdle( Kopete::OnlineStatus::Away, 
                        25, this, 4, "meanwhile_idle",
                        i18n( "Marked as Idle" ),
                        i18n( "Idle" ) ),
      meanwhileUnknown( Kopete::OnlineStatus::Offline, 
                        25, this, 5, "meanwhile_unknown",
                        i18n( "Where am I" ),
                        i18n( "Catch me if you can" ) ),
      statusMessage(QString::fromLatin1("statusMessage"), i18n("Status Message"),QString::null,false,true),
	  awayMessage(Kopete::Global::Properties::self()->awayMessage())
{
//    LOG("MeanwhileProtocol()");
    s_protocol = this;
}

MeanwhileProtocol::~MeanwhileProtocol()
{
}

AddContactPage * MeanwhileProtocol::createAddContactWidget(
                                    QWidget *parent, 
                                    Kopete::Account * account )
{
	return new MeanwhileAddContactPage(parent, account);
}

KopeteEditAccountWidget * MeanwhileProtocol::createEditAccountWidget( 
                                    Kopete::Account *account, 
                                    QWidget *parent )
{
//    LOG("createEditAccountWidget");
	return new MeanwhileEditAccountWidget( parent, account, this );
}

Kopete::Account *MeanwhileProtocol::createNewAccount( 
                                    const QString &accountId )
{
	return new MeanwhileAccount( this, accountId, accountId.ascii() );
}

MeanwhileProtocol *MeanwhileProtocol::protocol()
{
    return s_protocol;
}

Kopete::Contact *MeanwhileProtocol::deserializeContact( 
                            Kopete::MetaContact *metaContact,
                            const QMap<QString, 
                            QString> &serializedData, 
                            const QMap<QString, QString> & /* addressBookData */ )
{
    QString contactId = serializedData[ "contactId" ];
    QString accountId = serializedData[ "accountId" ];

    MeanwhileAccount *theAccount = 
            static_cast<MeanwhileAccount*>(
                            Kopete::AccountManager::self()->
                                    findAccount(protocol()->pluginId(), accountId));

    if(!theAccount)
    {
        return 0;
    }

    theAccount->addContact(contactId, metaContact, Kopete::Account::DontChangeKABC);
    return theAccount->contacts()[contactId];
}


#include "meanwhileprotocol.moc"

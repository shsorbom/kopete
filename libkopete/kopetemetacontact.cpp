/*
    kopetemetacontact.cpp - Kopete Meta Contact

    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2004 by Olivier Goffart        <ogoffart@tiscalinet.be>
    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetemetacontact.h"

#include <qtimer.h>

#include <kapplication.h>

#include <kabc/addressbook.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>

#include <kdebug.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdeversion.h>

#include "accountselector.h"
#include "kopetecontactlist.h"
#include "kopeteaccountmanager.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"
#include "kopetepluginmanager.h"
#include "kopetegroup.h"
#include "kopeteuiglobal.h"
#include "kopeteglobal.h"

struct KopeteMetaContactPrivate
{
	QPtrList<KopeteContact> contacts;

	QString displayName;

	bool trackChildNameChanges;

	KopeteGroupList groups;

	QMap<QString, QMap<QString, QString> > addressBook;

	bool temporary;
//	bool dirty;
	QString metaContactId;
	KopeteOnlineStatus::OnlineStatus onlineStatus;
	static bool s_addrBookWritePending;
};

KABC::AddressBook* KopeteMetaContact::m_addressBook = 0L;
bool KopeteMetaContactPrivate::s_addrBookWritePending = false;

KopeteMetaContact::KopeteMetaContact()
: KopetePluginDataObject( KopeteContactList::contactList() ), KopeteNotifyDataObject()
{
	d = new KopeteMetaContactPrivate;

	d->trackChildNameChanges = true;
	d->temporary = false;

	d->onlineStatus = KopeteOnlineStatus::Offline;

	connect( this, SIGNAL( pluginDataChanged() ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( iconChanged( KopetePluginDataObject::IconState, const QString & ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( useCustomIconChanged( bool ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( displayNameChanged( const QString &, const QString & ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( movedToGroup( KopeteMetaContact *, KopeteGroup *, KopeteGroup * ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( removedFromGroup( KopeteMetaContact *, KopeteGroup * ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( addedToGroup( KopeteMetaContact *, KopeteGroup * ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( contactAdded( KopeteContact * ) ), SLOT( emitPersistentDataChanged() ) );
	connect( this, SIGNAL( contactRemoved( KopeteContact * ) ), SLOT( emitPersistentDataChanged() ) );

	// make sure KopeteMetaContact is at least in one group
	addToGroup( KopeteGroup::topLevel() );
}

void KopeteMetaContact::emitPersistentDataChanged()
{
	emit persistentDataChanged( this );
}

KopeteMetaContact::~KopeteMetaContact()
{
	delete d;
}

void KopeteMetaContact::addContact( KopeteContact *c )
{
	if( d->contacts.contains( c ) )
	{
		kdWarning(14010) << "Ignoring attempt to add duplicate contact " << c->contactId() << "!" << endl;
	}
	else
	{
		d->contacts.append( c );

		connect( c, SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ),
			SLOT( slotContactStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

		connect( c, SIGNAL( propertyChanged( KopeteContact *, const QString &, const QVariant &, const QVariant & ) ),
			this, SLOT( slotPropertyChanged( KopeteContact *, const QString &, const QVariant &, const QVariant & ) ) ) ;

		connect( c, SIGNAL( contactDestroyed( KopeteContact * ) ),
			this, SLOT( slotContactDestroyed( KopeteContact * ) ) );

		connect( c, SIGNAL( idleStateChanged( KopeteContact * ) ),
			this, SIGNAL( contactIdleStateChanged( KopeteContact * ) ) );

		if( d->displayName.isNull() )
		{
			/*kdDebug(14010) << k_funcinfo <<
				"empty displayname, using contacts display" << endl;*/
			QString nick=c->property(Kopete::Global::Properties::self()->nickName()).value().toString();
			setDisplayName( nick.isEmpty() ? c->contactId() : nick );
			d->trackChildNameChanges = true;
		}

		if( d->contacts.count() > 1 )
		{
			/*kdDebug(14010) << k_funcinfo << "Disabling trackChildNameChanges,"
				" more than ONE Contact in MetaContact" << endl;*/
			d->trackChildNameChanges = false;
		}

		/* for( QStringList::ConstIterator it = groups.begin(); it != groups.end(); ++it )
			addToGroup(*it); */
		emit contactAdded(c);

		// Save the changed contact to KABC, if using KABC
/*		if ( !isTemporary() )
			updateKABC();
			// If the new contact is NOT in the pluginData
			kdDebug(14010) << k_funcinfo << " contactId PluginData " << pluginData( c->protocol(), QString::fromLatin1( "contactId" ) ) << endl;;
			if ( pluginData( c->protocol(), QString::fromLatin1( "contactId" ) ).contains( c->contactId() ) == 0 )
			{
				kdDebug(14010) << k_funcinfo << " didn't find " << c->contactId() << ", new contact, write KABC" << endl;
				slotUpdateKABC();
			}
			else
				kdDebug(14010) << k_funcinfo << " found " << c->contactId() << ", don't write." << endl;
			}*/
	}
	updateOnlineStatus();
}

void KopeteMetaContact::updateOnlineStatus()
{
	KopeteOnlineStatus::OnlineStatus newStatus = KopeteOnlineStatus::Unknown;
	KopeteOnlineStatus mostSignificantStatus;

	for ( QPtrListIterator<KopeteContact> it( d->contacts ); it.current(); ++it )
	{
		// find most significant status
		if ( it.current()->onlineStatus() > mostSignificantStatus )
			mostSignificantStatus = it.current()->onlineStatus();
	}

	newStatus = KopeteOnlineStatus::OnlineStatus( mostSignificantStatus.status() );

	if( newStatus != d->onlineStatus )
	{
		d->onlineStatus = newStatus;
		emit onlineStatusChanged( this, d->onlineStatus );
	}
}

unsigned long int KopeteMetaContact::idleTime() const
{
	unsigned long int time = 0;
	QPtrListIterator<KopeteContact> it( d->contacts );
	for( ; it.current(); ++it )
	{
		unsigned long int i = it.current()->idleTime();
		if( i < time || time == 0 )
		{
			time = i;
		}
	}
	return time;
}

void KopeteMetaContact::removeContact(KopeteContact *c, bool deleted)
{
	if( !d->contacts.contains( c ) )
	{
		kdDebug(14010) << "KopeteMetaContact::removeContact: Contact is not in this metaContact " << endl;
	}
	else
	{
		d->contacts.remove( c );

		if(!deleted)
		{  //If this function is tell by slotContactRemoved, c is maybe just a QObject
			disconnect( c, SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ),
				this, SLOT( slotContactStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

			connect( c, SIGNAL( propertyChanged( KopeteContact *, const QString &, const QVariant &, const QVariant & ) ),
			this, SLOT( slotPropertyChanged( KopeteContact *, const QString &, const QVariant &, const QVariant & ) ) ) ;


			disconnect( c, SIGNAL( contactDestroyed( KopeteContact * ) ),
				this, SLOT( slotContactDestroyed( KopeteContact * ) ) );

			disconnect( c, SIGNAL( idleStateChanged( KopeteContact * ) ),
				this, SIGNAL( contactIdleStateChanged( KopeteContact *) ) );

			kdDebug( 14010 ) << k_funcinfo << "Contact disconnected" << endl;
		}

		// Reparent the contact
		removeChild( c );

		emit contactRemoved( c );
	}
	updateOnlineStatus();
}

bool KopeteMetaContact::isTopLevel() const
{
	return( d->groups.contains( KopeteGroup::topLevel() ) );
}

KopeteContact *KopeteMetaContact::findContact( const QString &protocolId, const QString &accountId, const QString &contactId )
{
	//kdDebug( 14010 ) << k_funcinfo << "Num contacts: " << d->contacts.count() << endl;
	QPtrListIterator<KopeteContact> it( d->contacts );
	for( ; it.current(); ++it )
	{
		//kdDebug( 14010 ) << k_funcinfo << "Trying " << it.current()->contactId() << ", proto "
		//<< it.current()->protocol()->pluginId() << ", account " << it.current()->accountId() << endl;
		if( ( it.current()->contactId() == contactId ) && ( it.current()->protocol()->pluginId() == protocolId ) )
		{
			if ( accountId.isEmpty() )
				return it.current();

			if(it.current()->account())
			{
				if(it.current()->account()->accountId() == accountId)
					return it.current();
			}
		}
	}

	// Contact not found
	return 0L;
}

KopeteContact *KopeteMetaContact::sendMessage()
{
	KopeteContact *c = preferredContact();

	if( !c )
	{
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "This user is not reachable at the moment. Please make sure you are connected and using a protocol that supports offline sending, or wait "
			"until this user comes online." ), i18n( "User is Not Reachable" ) );
	}
	else
	{
		c->sendMessage();
		return c;
	}
	return 0L;
}

KopeteContact *KopeteMetaContact::startChat()
{
	KopeteContact *c = preferredContact();

	if( !c )
	{
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "This user is not reachable at the moment. Please make sure you are connected and using a protocol that supports offline sending, or wait "
			"until this user comes online." ), i18n( "User is Not Reachable" ) );
	}
	else
	{
		c->startChat();
		return c;
	}
	return 0L;
}

KopeteContact *KopeteMetaContact::preferredContact()
{
	/*
		This function will determine what contact will be used to reach the contact.

		The prefered contact is choose with the following criterias:  (in that order)
		1) If a contact was an open chatwindow already, we will use that one.
		2) The contact with the better online status is used. But if that
		    contact is not reachable, we prefer return no contact.
		3) If all the criterias aboxe still gives ex-eaquo, we use the preffered
		    account as selected in the account preferances (with the arrows)
	*/

	KopeteContact *contact = 0;
	bool hasOpenView=false; //has the selected contact already an open chatwindow

	for ( QPtrListIterator<KopeteContact> it( d->contacts ); it.current(); ++it )
	{
		KopeteContact *c=it.current();

		//Has the contact an open chatwindow?
		 if( c->manager(false) /*&& c->manager()->view(false)*/)
		 {      //there is no need of having a view() i consider already having a manager
		        // is enough to give the priority to that contact
		 	if( !hasOpenView )
			{ //the selected contact has not an openview
				contact=c;
				hasOpenView=true;
				if( c->isOnline() )
					continue;
			} //else, several contact might have an open view, uses following criterias
		 }
		 else if( hasOpenView && contact->isOnline() )
		 	continue; //This contact has not open view, but the selected contact has, and is reachable


		// FIXME: The isConnected call should be handled in KopeteContact::isReachable
		//        after KDE 3.2 - Martijn
		if ( !c->account() || !c->account()->isConnected() || !c->isReachable() )
			continue; //if this contact is not reachable, we ignore it.

		if ( !contact )
		{  //this is the first contact.
			contact= c;
		    continue;
		}

		if( c->onlineStatus().status() > contact->onlineStatus().status()  )
			contact=c; //this contact has a better status
		else if ( c->onlineStatus().status() == contact->onlineStatus().status() )
		{
			if( c->account()->priority() > contact->account()->priority() )
				contact=c;
			else if(  c->account()->priority() == contact->account()->priority()
					&& c->onlineStatus().weight() > contact->onlineStatus().weight() )
				contact = c;  //the weight is not supposed to follow the same scale for each protocol
		}
	}

	return contact;
}

KopeteContact *KopeteMetaContact::execute()
{
	switch ( KopetePrefs::prefs()->interfacePreference() )
	{
		case KopetePrefs::EmailWindow:
			return sendMessage();
			break;
		case KopetePrefs::ChatWindow:
		default:
			return startChat();
			break;
	}
}

QString KopeteMetaContact::statusIcon() const
{
	switch( status() )
	{
		case KopeteOnlineStatus::Online:
			if( useCustomIcon() )
				return icon( KopetePluginDataObject::Online );
			else
				return QString::fromLatin1( "metacontact_online" );

		case KopeteOnlineStatus::Away:
			if( useCustomIcon() )
				return icon( KopetePluginDataObject::Away );
			else
				return QString::fromLatin1( "metacontact_away" );

		case KopeteOnlineStatus::Unknown:
			if( useCustomIcon() )
				return icon( KopetePluginDataObject::Unknown );
			else
				return QString::fromLatin1( "metacontact_unknown" );

		case KopeteOnlineStatus::Offline:
		default:
			if( useCustomIcon() )
				return icon( KopetePluginDataObject::Offline );
			else
				return QString::fromLatin1( "metacontact_offline" );
	}
}

QString KopeteMetaContact::statusString() const
{
	switch( status() )
	{
		case KopeteOnlineStatus::Online:
			return i18n( "Online" );
		case KopeteOnlineStatus::Away:
			return i18n( "Away" );
		case KopeteOnlineStatus::Offline:
			return i18n( "Offline" );
		case KopeteOnlineStatus::Unknown:
		default:
			return i18n( "Status not available" );
	}
}

KopeteOnlineStatus::OnlineStatus KopeteMetaContact::status() const
{
	return d->onlineStatus;
}

bool KopeteMetaContact::isOnline() const
{
	QPtrListIterator<KopeteContact> it( d->contacts );
	for( ; it.current(); ++it )
	{
		if( it.current()->isOnline() )
			return true;
	}
	return false;
}

bool KopeteMetaContact::isReachable() const
{
	if ( isOnline() )
		return true;

	for ( QPtrListIterator<KopeteContact> it( d->contacts ); it.current(); ++it )
	{
		if ( it.current()->account()->isConnected() && it.current()->isReachable() )
			return true;
	}
	return false;
}

//Determine if we are capable of accepting file transfers
bool KopeteMetaContact::canAcceptFiles() const
{
	if( !isOnline() )
		return false;

	QPtrListIterator<KopeteContact> it( d->contacts );
	for( ; it.current(); ++it )
	{
		if( it.current()->canAcceptFiles() )
			return true;
	}
	return false;
}

//Slot for sending files
void KopeteMetaContact::sendFile( const KURL &sourceURL, const QString &altFileName, unsigned long fileSize )
{
	//If we can't send any files then exit
	if( d->contacts.isEmpty() || !canAcceptFiles() )
		return;

	//Find the highest ranked protocol that can accept files
	KopeteContact *contact = d->contacts.first();
	for( QPtrListIterator<KopeteContact> it( d->contacts ) ; it.current(); ++it )
	{
		if( ( *it )->onlineStatus() > contact->onlineStatus() && ( *it )->canAcceptFiles() )
			contact = *it;
	}

	//Call the sendFile slot of this protocol
	contact->sendFile( sourceURL, altFileName, fileSize );
}

void KopeteMetaContact::slotContactStatusChanged( KopeteContact * c, const KopeteOnlineStatus &status, const KopeteOnlineStatus &oldstatus  )
{
	updateOnlineStatus();
	emit contactStatusChanged( c, status );
}

void KopeteMetaContact::setDisplayName( const QString &name )
{
	/*kdDebug( 14010 ) << k_funcinfo << "Change displayName from " << d->displayName <<
		" to " << name  << ", d->trackChildNameChanges=" << d->trackChildNameChanges << endl;
	kdDebug(14010) << kdBacktrace(6) << endl;*/

	if( name == d->displayName )
		return;

	const QString old = d->displayName;
	d->displayName = name;

	//The name is set by the user, disable tracking
	d->trackChildNameChanges = false;

	emit displayNameChanged( old , name );

	for( QPtrListIterator<KopeteContact> it( d->contacts ) ; it.current(); ++it )
		( *it )->syncGroups();


}

QString KopeteMetaContact::displayName() const
{
	return d->displayName;
}

bool KopeteMetaContact::trackChildNameChanges() const
{
	return d->trackChildNameChanges;
}

void KopeteMetaContact::setTrackChildNameChanges( bool  track  )
{
	if (track && (d->contacts.count() == 1))
	{
		QString nick=d->contacts.first()->property(Kopete::Global::Properties::self()->nickName()).value().toString();
		setDisplayName( nick.isEmpty() ? d->contacts.first()->contactId() : nick );

		d->trackChildNameChanges = true;
	}
	else
	{
		d->trackChildNameChanges = false;
	}
	emitPersistentDataChanged();
}

void KopeteMetaContact::slotPropertyChanged( KopeteContact*, const QString &key,
		const QVariant&, const QVariant &newValue  )
{
	if( key == Kopete::Global::Properties::self()->nickName().key() )
	{
		QString newNick=newValue.toString();
		if( (d->trackChildNameChanges || d->displayName.isEmpty()) && !newNick.isEmpty() )
		{
			setDisplayName( newNick );
			//because d->trackChildNameChanges is set to false in setDisplayName
			d->trackChildNameChanges = true;
		}
	//TODO:  chack if the property was persistent, and emit, not only when it's the displayname
	emitPersistentDataChanged();
	}  // <<<<===  move that  one line before when ready :-)
}

void KopeteMetaContact::moveToGroup( KopeteGroup *from, KopeteGroup *to, GroupSyncMode syncMode )
{
	if ( !from || !d->groups.contains( from ) || ( !isTopLevel() && from->type() == KopeteGroup::TopLevel ) )
	{
		// We're adding, not moving, because 'from' is illegal
		addToGroup( to );
		return;
	}

	if ( !to || d->groups.contains( to ) || ( isTopLevel() && to->type() == KopeteGroup::TopLevel ) )
	{
		// We're removing, not moving, because 'to' is illegal
		removeFromGroup( from );
		return;
	}

	if ( isTemporary() && to->type() != KopeteGroup::Temporary )
		return;


	//kdDebug( 14010 ) << k_funcinfo << from->displayName() << " => " << to->displayName() << endl;

	d->groups.remove( from );
	d->groups.append( to );

	if ( syncMode == SyncGroups )
	{
		for( KopeteContact *c = d->contacts.first(); c ; c = d->contacts.next() )
			c->syncGroups();
	}

	emit movedToGroup( this, from, to );
}

void KopeteMetaContact::removeFromGroup( KopeteGroup *group, GroupSyncMode syncMode )
{
	if ( !group || !d->groups.contains( group ) || ( !isTopLevel() && group->type() == KopeteGroup::TopLevel ) ||
		( isTemporary() && group->type() == KopeteGroup::Temporary ) )
	{
		return;
	}

	d->groups.remove( group );

	// make sure KopeteMetaContact is at least in one group
	if ( d->groups.isEmpty() )
	{
		d->groups.append( KopeteGroup::topLevel() );
		emit addedToGroup( this, KopeteGroup::topLevel() );
	}

	if ( syncMode == SyncGroups )
	{
		for( KopeteContact *c = d->contacts.first(); c ; c = d->contacts.next() )
			c->syncGroups();
	}

	emit removedFromGroup( this, group );
}

void KopeteMetaContact::addToGroup( KopeteGroup *to, GroupSyncMode syncMode )
{
	if ( !to || d->groups.contains( to ) || ( to->type() == KopeteGroup::TopLevel && isTopLevel() ) )
		return;

	if ( d->temporary && to->type() != KopeteGroup::Temporary )
		return;

	if ( isTopLevel() )
	{
		d->groups.remove( KopeteGroup::topLevel() );
		emit removedFromGroup( this, KopeteGroup::topLevel() );
	}

	d->groups.append( to );

	if ( syncMode == SyncGroups )
	{
		for( KopeteContact *c = d->contacts.first(); c ; c = d->contacts.next() )
			c->syncGroups();
	}

	emit addedToGroup( this, to );
}

KopeteGroupList KopeteMetaContact::groups() const
{
	return d->groups;
}

void KopeteMetaContact::slotContactDestroyed( KopeteContact *contact )
{
	removeContact(contact,true);
}

const QDomElement KopeteMetaContact::toXML()
{
	// This causes each KopeteProtocol subclass to serialise its contacts' data into the metacontact's plugin data and address book data
	emit aboutToSave(this);

	QDomDocument metaContact;
	metaContact.appendChild( metaContact.createElement( QString::fromLatin1( "meta-contact" ) ) );
	metaContact.documentElement().setAttribute( QString::fromLatin1( "contactId" ), metaContactId() );

	QDomElement displayName = metaContact.createElement( QString::fromLatin1("display-name" ) );
	displayName.setAttribute( QString::fromLatin1("trackChildNameChanges"),
		QString::fromLatin1( d->trackChildNameChanges ? "1":"0" ) );
	displayName.appendChild( metaContact.createTextNode( d->displayName ) );
	metaContact.documentElement().appendChild( displayName );

	// Store groups
	if ( !d->groups.isEmpty() )
	{
		QDomElement groups = metaContact.createElement( QString::fromLatin1("groups") );
		KopeteGroup *g;
		for ( g = d->groups.first(); g; g = d->groups.next() )
		{
			QDomElement group = metaContact.createElement( QString::fromLatin1("group") );
			group.setAttribute( QString::fromLatin1("id"), g->groupId() );
			groups.appendChild( group );
		}
		metaContact.documentElement().appendChild( groups );
	}

	// Store other plugin data
	QValueList<QDomElement> pluginData = KopetePluginDataObject::toXML();
	for( QValueList<QDomElement>::Iterator it = pluginData.begin(); it != pluginData.end(); ++it )
		metaContact.documentElement().appendChild( metaContact.importNode( *it, true ) );

	// Store custom notification data
	QDomElement notifyData = KopeteNotifyDataObject::notifyDataToXML();
	if ( notifyData.hasChildNodes() )
		metaContact.documentElement().appendChild( metaContact.importNode( notifyData, true ) );
	return metaContact.documentElement();
}

bool KopeteMetaContact::fromXML( const QDomElement& element )
{
	if( !element.hasChildNodes() )
		return false;

	QString strContactId = element.attribute( QString::fromLatin1("contactId") );
	if( !strContactId.isEmpty() )
		d->metaContactId = strContactId;

	QDomElement contactElement = element.firstChild().toElement();
	while( !contactElement.isNull() )
	{
		if( contactElement.tagName() == QString::fromLatin1( "display-name" ) )
		{
			if ( contactElement.text().isEmpty() )
				return false;
			d->displayName = contactElement.text();

			d->trackChildNameChanges =
				( contactElement.attribute( QString::fromLatin1( "trackChildNameChanges" ),
				QString::fromLatin1( "0" ) ) == QString::fromLatin1( "1" ) );
		}
		else if( contactElement.tagName() == QString::fromLatin1( "groups" ) )
		{
			QDomNode group = contactElement.firstChild();
			while( !group.isNull() )
			{
				QDomElement groupElement = group.toElement();

				if( groupElement.tagName() == QString::fromLatin1( "group" ) )
				{
					QString strGroupId = groupElement.attribute( QString::fromLatin1("id") );
					if( !strGroupId.isEmpty() )
						addToGroup( KopeteContactList::contactList()->getGroup( strGroupId.toUInt() ) );
					else //kopete 0.6 contactlist
						addToGroup( KopeteContactList::contactList()->getGroup( groupElement.text() ) );
				}
				else if( groupElement.tagName() == QString::fromLatin1( "top-level" ) ) //kopete 0.6 contactlist
					addToGroup( KopeteGroup::topLevel() );

				group = group.nextSibling();
			}
		}
		else if( contactElement.tagName() == QString::fromLatin1( "address-book-field" ) )
		{
			QString app = contactElement.attribute( QString::fromLatin1( "app" ), QString::null );
			QString key = contactElement.attribute( QString::fromLatin1( "key" ), QString::null );
			QString val = contactElement.text();
			d->addressBook[ app ][ key ] = val;
		}
		else if( contactElement.tagName() == QString::fromLatin1( "custom-notifications" ) )
		{
			KopeteNotifyDataObject::notifyDataFromXML( contactElement );
		}
		else //if( groupElement.tagName() == QString::fromLatin1( "plugin-data" ) || groupElement.tagName() == QString::fromLatin1("custom-icons" ))
		{
			KopetePluginDataObject::fromXML(contactElement);
		}
		contactElement = contactElement.nextSibling().toElement();
	}

	// If a plugin is loaded, load data cached
	connect( KopetePluginManager::self(), SIGNAL( pluginLoaded(KopetePlugin*) ),
		this, SLOT( slotPluginLoaded(KopetePlugin*) ) );

	// track changes only works if ONE Contact is inside the MetaContact
//	if (d->contacts.count() > 1) // Does NOT work as intended
//		d->trackChildNameChanges=false;

//	kdDebug(14010) << "[KopeteMetaContact] END fromXML(), d->trackChildNameChanges=" << d->trackChildNameChanges << "." << endl;
	return true;
}

QString KopeteMetaContact::addressBookField( KopetePlugin * /* p */, const QString &app, const QString & key ) const
{
	return d->addressBook[ app ][ key ];
}

void KopeteMetaContact::setAddressBookField( KopetePlugin * /* p */, const QString &app, const QString &key, const QString &value )
{
	d->addressBook[ app ][ key ] = value;
}

bool KopeteMetaContact::isTemporary() const
{
	return d->temporary;
}

void KopeteMetaContact::setTemporary( bool isTemporary, KopeteGroup *group )
{
	d->temporary = isTemporary;
	KopeteGroup *temporaryGroup = KopeteGroup::temporary();
	if ( d->temporary )
	{
		addToGroup (temporaryGroup);
		KopeteGroup *g;
		for( g = d->groups.first(); g; g = d->groups.next() )
		{
			if(g != temporaryGroup)
				removeFromGroup(g);
		}
	}
	else
		moveToGroup(temporaryGroup, group ? group : KopeteGroup::topLevel());
}

void KopeteMetaContact::slotPluginLoaded( KopetePlugin *p )
{
	if( !p )
		return;

	QMap<QString, QString> map= pluginData( p );
	if(!map.isEmpty())
	{
		p->deserialize(this,map);
	}
}

QString KopeteMetaContact::metaContactId() const
{
	return d->metaContactId;
}

void KopeteMetaContact::setMetaContactId( const QString& newMetaContactId )
{
	if(newMetaContactId == d->metaContactId)
		return;

	// 1) Check the Id is not already used by another contact
	// 2) cause a kabc write ( only in response to kopetemetacontactLVIProps calling this, or will
	//      write be called twice when creating a brand new MC? )
	// 3) What about changing from one valid kabc to another, are kabc fields removed?
	// 4) May be called with Null to remove an invalid kabc uid by KMC::toKABC()
	// 5) Is called when reading the saved contact list

	removeKABC();
	d->metaContactId = newMetaContactId;
	updateKABC();
	emit onlineStatusChanged( this, d->onlineStatus );
	emitPersistentDataChanged();
}

void KopeteMetaContact::updateKABC()
{
	// Save any changes in each contact's addressBookFields to KABC
	KABC::AddressBook* ab = addressBook();

	// Wipe out the existing addressBook entries
	d->addressBook.clear();
	// This causes each KopeteProtocol subclass to serialise its contacts' data into the metacontact's plugin data and address book data
	emit aboutToSave(this);

	// If the metacontact is linked to a kabc entry
	if ( !d->metaContactId.isEmpty() )
	{
		//kdDebug( 14010 ) << k_funcinfo << "looking up Addressee for " << displayName() << "..." << endl;
		// Look up the address book entry
		KABC::Addressee theAddressee = ab->findByUid( metaContactId() );
		// Check that if addressee is not deleted or if the link is spurious
		// (inherited from Kopete < 0.8, where all metacontacts had random ids)

		// FIXME: this no longer gets called when reading all contacts but we need something similar to update from 0.7
		if ( theAddressee.isEmpty() )
		{
			// remove the link
			d->metaContactId=QString::null;
		}
		else
		{
			//kdDebug( 14010 ) << k_funcinfo << "...FOUND ONE!" << endl;
			// Store address book fields
			QMap<QString, QMap<QString, QString> >::ConstIterator appIt = d->addressBook.begin();
			for( ; appIt != d->addressBook.end(); ++appIt )
			{
				QMap<QString, QString>::ConstIterator addrIt = appIt.data().begin();
				for( ; addrIt != appIt.data().end(); ++addrIt )
				{
					// FIXME: This assumes Kopete is the only app writing these fields
					// Note if nothing ends up in the KABC data, this is because insertCustom does nothing if any param is empty.
					kdDebug( 14010 ) << k_funcinfo << "Writing: " << appIt.key() << ", " << addrIt.key() << ", " << addrIt.data() << endl;
					theAddressee.insertCustom( appIt.key(), addrIt.key(), addrIt.data() );
				}
			}
			ab->insertAddressee( theAddressee );

			writeAddressBook();
		}
	}
}

void KopeteMetaContact::removeKABC()
{
	// remove any data this KMC has written to the KDE address book
	// Save any changes in each contact's addressBookFields to KABC
	KABC::AddressBook* ab = addressBook();

	// Wipe out the existing addressBook entries
	d->addressBook.clear();
	// This causes each KopeteProtocol subclass to serialise its contacts' data into the metacontact's plugin data and address book data
	emit aboutToSave(this);

	// If the metacontact is linked to a kabc entry
	if ( !d->metaContactId.isEmpty() )
	{
		//kdDebug( 14010 ) << k_funcinfo << "looking up Addressee for " << displayName() << "..." << endl;
		// Look up the address book entry
		KABC::Addressee theAddressee = ab->findByUid( metaContactId() );

		if ( theAddressee.isEmpty() )
		{
			// remove the link
			//kdDebug( 14010 ) << k_funcinfo << "...not found." << endl;
			d->metaContactId=QString::null;
		}
		else
		{
			//kdDebug( 14010 ) << k_funcinfo << "...FOUND ONE!" << endl;
			// Remove address book fields
			QMap<QString, QMap<QString, QString> >::ConstIterator appIt = d->addressBook.begin();
			for( ; appIt != d->addressBook.end(); ++appIt )
			{
				QMap<QString, QString>::ConstIterator addrIt = appIt.data().begin();
				for( ; addrIt != appIt.data().end(); ++addrIt )
				{
					// FIXME: This assumes Kopete is the only app writing these fields
					kdDebug( 14010 ) << k_funcinfo << "Removing: " << appIt.key() << ", " << addrIt.key() << endl;
					theAddressee.removeCustom( appIt.key(), addrIt.key() );
				}
			}
			ab->insertAddressee( theAddressee );

			writeAddressBook();
		}
	}
//	kdDebug(14010) << k_funcinfo << kdBacktrace() <<endl;
}

QPtrList<KopeteContact> KopeteMetaContact::contacts() const
{
	return d->contacts;
}

KABC::AddressBook* KopeteMetaContact::addressBook()
{
	if ( m_addressBook == 0L )
	{
		m_addressBook = KABC::StdAddressBook::self();
		KABC::StdAddressBook::setAutomaticSave( false );
	}
	return m_addressBook;
}

void KopeteMetaContact::writeAddressBook()
{
	if ( !KopeteMetaContactPrivate::s_addrBookWritePending )
	{
		KopeteMetaContactPrivate::s_addrBookWritePending = true;
		QTimer::singleShot( 2000, this, SLOT( slotWriteAddressBook() ) );
	}
}

void KopeteMetaContact::slotWriteAddressBook()
{
	KABC::AddressBook* ab = addressBook();

	KABC::Ticket *ticket = ab->requestSaveTicket();
	if ( !ticket )
		kdWarning( 14010 ) << k_funcinfo << "WARNING: Resource is locked by other application!" << endl;
	else
	{
		if ( !ab->save( ticket ) )
		{
			kdWarning( 14010 ) << k_funcinfo << "ERROR: Saving failed!" << endl;
			ab->releaseSaveTicket( ticket );
		}
	}
	//kdDebug( 14010 ) << k_funcinfo << "Finished writing KABC" << endl;
	KopeteMetaContactPrivate::s_addrBookWritePending = false;
}

bool KopeteMetaContact::syncWithKABC()
{
	kdDebug(14010) << k_funcinfo << endl;
	bool contactAdded = false;
	// check whether the dontShowAgain was checked
	if ( !d->metaContactId.isEmpty() ) // if we are associated with KABC
	{
		KABC::AddressBook* ab = addressBook();
		KABC::Addressee addr  = ab->findByUid( metaContactId() );
		// load the set of addresses from KABC
		QStringList customs = addr.customs();

		QStringList::ConstIterator it;
		for ( it = customs.begin(); it != customs.end(); ++it )
		{
			QString app, name, value;
			splitField( *it, app, name, value );
			kdDebug( 14010 ) << "app=" << app << " name=" << name << " value=" << value << endl;

			if ( app.startsWith( QString::fromLatin1( "messaging/" ) ) )
			{
				if ( name == QString::fromLatin1( "All" ) )
				{
					kdDebug( 14010 ) << " syncing \"" << app << ":" << name << " with contactlist " << endl;
					// Get the protocol name from the custom field
					// by chopping the 'messaging/' prefix from the custom field app name
					QString protocolName = app.right( app.length() - 10 );
					// munge Jabber hack
					if ( protocolName == QString::fromLatin1( "xmpp" ) )
						protocolName = QString::fromLatin1( "jabber" );

					// Check Kopete supports it
					KopeteProtocol * proto = dynamic_cast<KopeteProtocol*>( KopetePluginManager::self()->loadPlugin( QString::fromLatin1( "kopete_" ) + protocolName ) );
					if ( !proto )
					{
						KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
								i18n( "<qt>\"%1\" is not supported by Kopete.</qt>" ).arg( protocolName ),
								i18n( "Could Not Sync with KDE Address Book" )  );
						continue;
					}

					// See if we need to add each contact in this protocol
					QStringList addresses = QStringList::split( QChar( 0xE000 ), value );
					QStringList::iterator end = addresses.end();
					for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
					{
						// check whether each one is present in Kopete
						// Is it in the contact list?
						// First discard anything after an 0xE120, this is used by IRC to separate nick and server group name, but
						// IRC doesn't support this properly yet, so the user will have to select an appropriate account manually
						int separatorPos = (*it).find( QChar( 0xE120 ) );
						if ( separatorPos != -1 )
							*it = (*it).left( separatorPos );

						QDict<KopeteAccount> accounts = KopeteAccountManager::manager()->accounts( proto );
						QDictIterator<KopeteAccount> acs(accounts);
						KopeteMetaContact *mc = 0;
						for ( acs.toFirst(); acs.current(); ++acs )
							if ( ( mc = KopeteContactList::contactList()->findContact(
									proto->pluginId(), acs.current()->accountId(), *it ) ) )
								break;

						if ( mc ) // Is it in another metacontact?
						{
							// Is it already in this metacontact? If so, we needn't do anything
							if ( mc == this )
							{
								kdDebug( 14010 ) << *it << " already a child of this metacontact." << endl;
								continue;
							}
							kdDebug( 14010 ) << *it << " already exists in OTHER metacontact, move here?" << endl;
							// find the KopeteContact and attempt to move it to this metacontact.
							mc->findContact( proto->pluginId(), QString::null, *it )->setMetaContact( this );
						}
						else
						{
							// if not, prompt to add it
							kdDebug( 14010 ) << proto->pluginId() << "://" << *it << " was not found in the contact list.  Prompting to add..." << endl;
							if ( KMessageBox::Yes == KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
															 i18n( "<qt>An address was added to this contact by another application.<br>Would you like to use it in Kopete?<br><b>Protocol:</b> %1<br><b>Address:</b> %2</qt>" ).arg( proto->displayName() ).arg( *it ), i18n( "Import Address From Address Book" ), i18n("&Yes"), i18n("&No"), QString::fromLatin1( "ImportFromKABC" ) ) )
							{
								// Check the accounts for this protocol are all connected
								// Most protocols do not allow you to add contacts while offline
								// Would be better to have a virtual bool KopeteAccount::readyToAddContact()
								bool allAccountsConnected = true;
								for ( acs.toFirst(); acs.current(); ++acs )
									if ( !acs.current()->isConnected() )
									{	allAccountsConnected = false;
										break;
									}
								if ( !allAccountsConnected )
								{
									KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
										i18n( "<qt>One or more of your accounts using %1 are offline.  Most systems have to be connected to add contacts.  Please connect these accounts and try again.</qt>" ).arg( protocolName ),
										i18n( "Not Connected" )  );
									continue;
								}

								// we have got a contact to add, our accounts are connected, so add it.
								// Do we need to choose an account
								KopeteAccount *chosen = 0;
								if ( accounts.count() > 1 )
								{	// if we have >1 account in this protocol, prompt for the protocol.
									KDialogBase *chooser = new KDialogBase(0, "chooser", true,
										i18n("Choose Account"), KDialogBase::Ok|KDialogBase::Cancel,
										KDialogBase::Ok, false);
									AccountSelector *accSelector = new AccountSelector(proto, chooser,
										"accSelector");
									chooser->setMainWidget(accSelector);
									if ( chooser->exec() == QDialog::Rejected )
										continue;
									chosen = accSelector->selectedItem();

									delete chooser;
								}
								else if ( accounts.isEmpty() )
								{
									KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
										i18n( "<qt>You do not have an account configured for <b>%1</b> yet.  Please create an account, connect it, and try again.</qt>" ).arg( protocolName ),
										i18n( "No Account Found" )  );
									continue;
								}
								else // if we have 1 account in this protocol, choose it
								{
									chosen = acs.toFirst();
								}

								// add the contact to the chosen account
								if ( chosen )
								{
									kdDebug( 14010 ) << "Adding " << *it << " to " << chosen->accountId() << endl;
									if ( chosen->addContact( *it, QString::null, this ) )
										contactAdded = true;
									else
										KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
											i18n( "<qt>It was not possible to add the contact. Please see the debug messages for details.</qt>" ),
											i18n( "Could Not Add Contact") ) ;
								}
							}
							else
								kdDebug( 14010 ) << " user declined to add " << *it << " to contactlist " << endl;
						}
					}
					kdDebug( 14010 ) << " all " << addresses.count() << " contacts in " << proto->pluginId() << " checked " << endl;
				}
				else
					kdDebug( 14010 ) << "not interested in name=" << name << endl;

			}
			else
				kdDebug( 14010 ) << "not interested in app=" << app << endl;
		}
	}
	return contactAdded;
}

// FIXME: Remove when IM address API is in KABC (KDE 4)
void KopeteMetaContact::splitField( const QString &str, QString &app, QString &name, QString &value )
{
  int colon = str.find( ':' );
  if ( colon != -1 ) {
    QString tmp = str.left( colon );
    value = str.mid( colon + 1 );

    int dash = tmp.find( '-' );
    if ( dash != -1 ) {
      app = tmp.left( dash );
      name = tmp.mid( dash + 1 );
    }
  }
}

#include "kopetemetacontact.moc"

// vim: set noet ts=4 sts=4 sw=4:

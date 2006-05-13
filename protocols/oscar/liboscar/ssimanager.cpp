/*
	Kopete Oscar Protocol
	ssimanager.cpp - SSI management

	Copyright ( c ) 2004 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
	Copyright ( c ) 2004 Matt Rogers <mattr@kde.org>

	Kopete ( c ) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

	based on ssidata.h and ssidata.cpp ( c ) 2002 Tom Linsky <twl6@po.cwru.edu>

	*************************************************************************
	*                                                                       *
	* This library is free software; you can redistribute it and/or         *
	* modify it under the terms of the GNU Lesser General Public            *
	* License as published by the Free Software Foundation; either          *
	* version 2 of the License, or ( at your option ) any later version.    *
	*                                                                       *
	*************************************************************************
*/

#include "ssimanager.h"
#include <kdebug.h>
#include "oscarutils.h"

// -------------------------------------------------------------------

class SSIManagerPrivate
{
public:
	QList<Oscar::SSI> SSIList;
	WORD lastModTime;
	WORD maxContacts;
	WORD maxGroups;
	WORD maxVisible;
	WORD maxInvisible;
	WORD maxIgnore;
	WORD nextContactId;
	WORD nextGroupId;
};

SSIManager::SSIManager( QObject *parent )
 : QObject(parent)
{
	d = new SSIManagerPrivate;
	d->lastModTime = 0;
	d->nextContactId = 0;
	d->nextGroupId = 0;
	d->maxContacts = 999;
	d->maxGroups = 999;
	d->maxIgnore = 999;
	d->maxInvisible = 999;
	d->maxVisible = 999;
}


SSIManager::~SSIManager()
{
	clear();
	delete d;
}

void SSIManager::clear()
{
	//delete all SSIs from the list
	if ( d->SSIList.count() > 0 )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Clearing the SSI list" << endl;
		QList<Oscar::SSI>::iterator it = d->SSIList.begin();

		while ( it != d->SSIList.end() && d->SSIList.count() > 0 )
			it = d->SSIList.erase( it );
	};
}

WORD SSIManager::nextContactId()
{
	d->nextContactId++;
	return d->nextContactId;
}

WORD SSIManager::nextGroupId()
{
	d->nextGroupId++;
	return d->nextGroupId;
}

WORD SSIManager::numberOfItems() const
{
	return d->SSIList.count();
}

DWORD SSIManager::lastModificationTime() const
{
	return d->lastModTime;
}

void SSIManager::setLastModificationTime( DWORD lastTime )
{
	d->lastModTime = lastTime;
}

void SSIManager::setParameters( WORD maxContacts, WORD maxGroups, WORD maxVisible, WORD maxInvisible, WORD maxIgnore )
{
	//I'm not using k_funcinfo for these debug statements because of
	//the function's long signature
	QString funcName = QString::fromLatin1( "[void SSIManager::setParameters] " );
	kDebug(OSCAR_RAW_DEBUG) << funcName << "Max number of contacts allowed in SSI: "
		<< maxContacts << endl;
	kDebug(OSCAR_RAW_DEBUG) << funcName << "Max number of groups allowed in SSI: "
		<< maxGroups << endl;
	kDebug(OSCAR_RAW_DEBUG) << funcName << "Max number of contacts allowed on visible list: "
		<< maxVisible << endl;
	kDebug(OSCAR_RAW_DEBUG) << funcName << "Max number of contacts allowed on invisible list: "
		<< maxInvisible << endl;
	kDebug(OSCAR_RAW_DEBUG) << funcName << "Max number of contacts allowed on ignore list: "
		<< maxIgnore << endl;

	d->maxContacts = maxContacts;
	d->maxGroups = maxGroups;
	d->maxInvisible = maxInvisible;
	d->maxVisible = maxVisible;
	d->maxIgnore = maxIgnore;
}

void SSIManager::loadFromExisting( const QList<Oscar::SSI*>& newList )
{
	Q_UNUSED( newList );
	//FIXME: NOT Implemented!
}

bool SSIManager::hasItem( const Oscar::SSI& item ) const
{
	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it != listEnd; ++it )
	{
		Oscar::SSI s = ( *it );
		if ( s == item )
			return true;
	}

	return false;
}

Oscar::SSI SSIManager::findGroup( const QString &group ) const
{
	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_GROUP && (*it ).name().toLower() == group.toLower() )
			return ( *it );


	return m_dummyItem;
}

Oscar::SSI SSIManager::findGroup( int groupId ) const
{
	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_GROUP && (*it ).gid() == groupId )
			return ( *it );

	return m_dummyItem;
}

Oscar::SSI SSIManager::findContact( const QString &contact, const QString &group ) const
{

	if ( contact.isNull() || group.isNull() )
	{
		kWarning(OSCAR_RAW_DEBUG) << k_funcinfo <<
			"Passed NULL name or group string, aborting!" << endl;

		return m_dummyItem;
	}

	Oscar::SSI gr = findGroup( group ); // find the parent group
	if ( gr.isValid() )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "gr->name= " << gr.name() <<
			", gr->gid= " << gr.gid() <<
			", gr->bid= " << gr.bid() <<
			", gr->type= " << gr.type() << endl;

		QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();

		for ( it = d->SSIList.begin(); it != listEnd; ++it )
		{
			if ( ( *it ).type() == ROSTER_CONTACT && (*it ).name() == contact && (*it ).gid() == gr.gid() )
			{
				//we have found our contact
				kDebug(OSCAR_RAW_DEBUG) << k_funcinfo <<
					"Found contact " << contact << " in SSI data" << endl;
				 return ( *it );
			}
		}
	}
	else
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo <<
			"ERROR: Group '" << group << "' not found!" << endl;
	}
	return m_dummyItem;
}

Oscar::SSI SSIManager::findContact( const QString &contact ) const
{

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_CONTACT && (*it ).name() == contact )
			return ( *it );

	return m_dummyItem;
}

Oscar::SSI SSIManager::findContact( int contactId ) const
{
	QList<Oscar::SSI>::const_iterator it,  listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it!= listEnd; ++it )
		if ( ( *it ).type() == ROSTER_CONTACT && ( *it ).bid() == contactId )
			return ( *it );

	return m_dummyItem;
}

Oscar::SSI SSIManager::findItemForIcon( QByteArray iconHash ) const
{
	QList<Oscar::SSI>::const_iterator it,  listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it!= listEnd; ++it )
	{
		if ( ( *it ).type() == ROSTER_BUDDYICONS )
		{
			TLV t = Oscar::findTLV( ( *it ).tlvList(), 0x00D5 );
			Buffer b(t.data);
			b.skipBytes(1); //don't care about flags
			BYTE iconSize = b.getByte();
			QByteArray hash( b.getBlock( iconSize ) );
			if ( hash == iconHash )
			{
				Oscar::SSI s = ( *it );
				return s;
			}
		}
	}
	return m_dummyItem;
}

Oscar::SSI SSIManager::findItemForIconByRef( int ref ) const
{
	QList<Oscar::SSI>::const_iterator it,  listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it!= listEnd; ++it )
	{
		if ( ( *it ).type() == ROSTER_BUDDYICONS )
		{
			if ( ( *it ).name().toInt() == ref )
			{
				Oscar::SSI s = ( *it );
				return s;
			}
		}
	}
	return m_dummyItem;
}

Oscar::SSI SSIManager::findItem( const QString &contact, int type ) const
{
	QList<Oscar::SSI>::const_iterator it,  listEnd = d->SSIList.end();

	for ( it = d->SSIList.begin(); it!= listEnd; ++it )
		if ( ( *it ).type() == type && ( *it ).name() == contact )
			return ( *it );

	return m_dummyItem;
}

QList<Oscar::SSI> SSIManager::groupList() const
{
	QList<Oscar::SSI> list;

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_GROUP  )
			list.append( ( *it ) );

	return list;
}

QList<Oscar::SSI> SSIManager::contactList() const
{
	QList<Oscar::SSI> list;

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_CONTACT  )
			list.append( ( *it ) );

	return list;
}

QList<Oscar::SSI> SSIManager::visibleList() const
{
	QList<Oscar::SSI> list;

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_VISIBLE  )
			list.append( ( *it ) );

	return list;
}

QList<Oscar::SSI> SSIManager::invisibleList() const
{
	QList<Oscar::SSI> list;

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_INVISIBLE  )
			list.append( ( *it ) );

	return list;
}

QList<Oscar::SSI> SSIManager::contactsFromGroup( const QString &group ) const
{
	QList<Oscar::SSI> list;

	Oscar::SSI gr = findGroup( group );
	if ( gr.isValid() )
	{
		QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
		for ( it = d->SSIList.begin(); it != listEnd; ++it )
			if ( ( *it ).type() == ROSTER_CONTACT && (*it ).gid() == gr.gid() )
				list.append( ( *it ) );
	}
	return list;
}

QList<Oscar::SSI> SSIManager::contactsFromGroup( int groupId ) const
{
	QList<Oscar::SSI> list;

	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
		if ( ( *it ).type() == ROSTER_CONTACT && (*it ).gid() == groupId  )
			list.append( ( *it ) );

	return list;
}

Oscar::SSI SSIManager::visibilityItem() const
{
	Oscar::SSI item = m_dummyItem;
	QList<Oscar::SSI>::const_iterator it, listEnd = d->SSIList.end();
	for ( it = d->SSIList.begin(); it != listEnd; ++it )
	{
		if ( ( *it ).type() == 0x0004 )
		{
			kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Found visibility setting" << endl;
			item = ( *it );
			return item;
		}
	}

	return item;
}

bool SSIManager::listComplete() const
{
	//if last modification time is zero, we're not done
	//getting the list
	return d->lastModTime != 0;
}

bool SSIManager::newGroup( const Oscar::SSI& group )
{
	//trying to find the group by its ID
	QList<Oscar::SSI>::iterator it, listEnd = d->SSIList.end();
	if ( findGroup( group.name() ).isValid() )
		return false;

	if ( !group.name().isEmpty() ) //avoid the group with gid 0 and bid 0
	{	// the group is really new
		kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Adding group '" << group.name() << "' to SSI list" << endl;
		if ( group.gid() > d->nextGroupId )
			d->nextGroupId = group.gid();

		d->SSIList.append( group );
		emit groupAdded( group );
		return true;
	}
	return false;
}

bool SSIManager::removeGroup( const Oscar::SSI& group )
{
	QString groupName = group.name();
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Removing group " << group.name() << endl;
	int remcount = d->SSIList.removeAll( group );
	if ( remcount == 0 )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "No groups removed" << endl;
		return false;
	}

	emit groupRemoved( groupName );
	return true;
}

bool SSIManager::removeGroup( const QString &group )
{
	Oscar::SSI gr = findGroup( group );

	if ( gr.isValid() && removeGroup( gr )  )
	{
		return true;
	}
	else
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Group " << group << " not found." << endl;

	return false;
}

bool SSIManager::newContact( const Oscar::SSI& contact )
{
	//what to validate?
	if ( contact.bid() > d->nextContactId )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Setting next contact ID to " << contact.bid() << endl;
		d->nextContactId = contact.bid();
	}

	if ( d->SSIList.indexOf( contact ) == -1 )
	{
		kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Adding contact '" << contact.name() << "' to SSI list" << endl;
		d->SSIList.append( contact );
		emit contactAdded( contact );
	}
	else
		return false;
	return true;
}

bool SSIManager::removeContact( const Oscar::SSI& contact )
{
	QString contactName = contact.name();
	int remcount = d->SSIList.removeAll( contact );

	if ( remcount == 0 )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "No contacts were removed." << endl;
		return false;
	}

	emit contactRemoved( contactName );
	return true;
}

bool SSIManager::removeContact( const QString &contact )
{
	Oscar::SSI ct = findContact( contact );

	if ( ct.isValid() && removeContact( ct ) )
		return true;
	else
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Contact " << contact << " not found." << endl;

	return false;
}

bool SSIManager::newItem( const Oscar::SSI& item )
{
	if ( item.bid() > d->nextContactId )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Setting next contact ID to " << item.bid() << endl;
		d->nextContactId = item.bid();
	}

	//no error checking for now
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Adding item " << item.toString() << endl;
	d->SSIList.append( item );
	return true;
}

bool SSIManager::removeItem( const Oscar::SSI& item )
{
	d->SSIList.removeAll( item );
	return true;
}

#include "ssimanager.moc"

//kate: tab-width 4; indent-mode csands;

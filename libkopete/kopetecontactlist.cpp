/*
    kopetecontactlist.cpp - Kopete's Contact List backend

    Copyright (c) 2002 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontactlist.h"

#include <qdom.h>
#include <qfile.h>
#include <qregexp.h>
#include <qstylesheet.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "pluginloader.h"
#include "kopetegroup.h"


KopeteContactList *KopeteContactList::s_contactList = 0L;

KopeteContactList *KopeteContactList::contactList()
{
	if( !s_contactList )
		s_contactList = new KopeteContactList;

	return s_contactList;
}

KopeteContactList::KopeteContactList()
: QObject( kapp, "KopeteContactList" )
{
}

KopeteContactList::~KopeteContactList()
{
// save is currently called in ~kopete (before the deletion of plugins)
//	save();
}

KopeteMetaContact *KopeteContactList::findContact( const QString &protocolId,
	const QString &identityId, const QString &contactId )
{
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if( it.current()->findContact( protocolId, identityId, contactId ) )
			return it.current();
	}
	//kdDebug(14010) << k_funcinfo << "*** Not found!" << endl;
	return 0L;
}

void KopeteContactList::addMetaContact( KopeteMetaContact *mc )
{
	m_contacts.append( mc );

/*	connect( mc,
		SIGNAL( removedFromGroup( KopeteMetaContact *, const QString & ) ),
		SLOT( slotRemovedFromGroup( KopeteMetaContact *, const QString & ) ) );*/

	emit metaContactAdded( mc );
}

/*void KopeteContactList::slotRemovedFromGroup( KopeteMetaContact *mc,
	const QString &  )
{
	if( mc->groups().isEmpty() )
	{
		kdDebug(14010) << "KopeteContactList::slotRemovedFromGroup: "
			<< "contact removed from all groups: now toplevel." << endl;
		//m_contacts.remove( mc );
		//mc->deleteLater();
	}
}                  */

void KopeteContactList::loadXML()
{
	addGroup( KopeteGroup::toplevel );

	QString filename = locateLocal( "appdata", "contactlist.xml" );
	if( filename.isEmpty() )
		return ;

	QDomDocument contactList( "kopete-contact-list" );

	QFile contactListFile( filename );
	contactListFile.open( IO_ReadOnly );
	contactList.setContent( &contactListFile );

	QDomElement list = contactList.documentElement();

	QString versionString = list.attribute( "version", QString::null );
	uint version = 0;
	if( QRegExp( "[0-9]+\\.[0-9]" ).exactMatch( versionString ) )
		version = versionString.replace( '.', QString::null ).toUInt();

	if( version < ContactListVersion )
	{
		// The version string is invalid, or we're using an older version.
		// Convert first and reparse the file afterwards
		kdDebug() << k_funcinfo << "Contact list version " << version
			<< " is older than current version " << ContactListVersion
			<< ". Converting first." << endl;

		contactListFile.close();

		convertContactList( filename, version, ContactListVersion );

		contactList = QDomDocument ( "kopete-contact-list" );

		contactListFile.open( IO_ReadOnly );
		contactList.setContent( &contactListFile );

		list = contactList.documentElement();
	}

	QDomNode node = list.firstChild();
	while( !node.isNull() )
	{
		QDomElement element = node.toElement();
		if( !element.isNull() )
		{
			if( element.tagName() == "meta-contact" )
			{
				//TODO: id isn't used
				//QString id = element.attribute( "id", QString::null );
				KopeteMetaContact *metaContact = new KopeteMetaContact();

				QDomNode contactNode = node.firstChild();
				if ( !metaContact->fromXML( contactNode ) )
				{
					delete metaContact;
					metaContact = 0;
				}
				else
				{
					KopeteContactList::contactList()->addMetaContact(
						metaContact );
				}
			}
			else if( element.tagName() == "kopete-group" )
			{
				KopeteGroup *group = new KopeteGroup();
				if( !group->fromXML( node.firstChild() ) )
				{
					delete group;
					group = 0;
				}
				else
				{
					KopeteContactList::contactList()->addGroup( group );
				}
			}
			else
			{
				kdWarning(14010) << "KopeteContactList::loadXML: "
					  << "Unknown element '" << element.tagName()
					  << "' in contact list!" << endl;
			}

		}
		node = node.nextSibling();
	}
	contactListFile.close();
}

void KopeteContactList::convertContactList( const QString &fileName, uint /* fromVersion */, uint /* toVersion */ )
{
	// For now, ignore fromVersion and toVersion. These are meant for future
	// changes to allow incremental (multi-pass) conversion so we don't have
	// to rewrite the whole conversion code for each change.

	QDomDocument contactList( "messaging-contact-list" );
	QFile contactListFile( fileName );
	contactListFile.open( IO_ReadOnly );
	contactList.setContent( &contactListFile );

	QDomElement oldList = contactList.documentElement();

	QDomDocument newList( "kopete-contact-list" );
	newList.appendChild( newList.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

	QDomElement newRoot = newList.createElement( "kopete-contact-list" );
	newList.appendChild( newRoot );
	newRoot.setAttribute( "version", "1.0" );

	QDomNode oldNode = oldList.firstChild();
	while( !oldNode.isNull() )
	{
		QDomElement oldElement = oldNode.toElement();
		if( !oldElement.isNull() )
		{
			if( oldElement.tagName() == "meta-contact" )
			{
				// Ignore ID, it is not used in the current list anyway
				QDomElement newMetaContact = newList.createElement( "meta-contact" );
				newRoot.appendChild( newMetaContact );

				// Plugin data is stored completely different, and requires
				// some bookkeeping to convert properly
				QMap<QString, QDomElement> pluginData;
				QStringList icqData;
				QStringList gaduData;

				QDomNode oldContactNode = oldNode.firstChild();
				while( !oldContactNode.isNull() )
				{
					QDomElement oldContactElement = oldContactNode.toElement();
					if( !oldContactElement.isNull() )
					{
						if( oldContactElement.tagName() == "display-name" )
						{
							QDomElement displayName = newList.createElement( "display-name" );
							displayName.appendChild( newList.createTextNode( oldContactElement.text() ) );
							newMetaContact.appendChild( displayName );
						}
						else if( oldContactElement.tagName() == "groups" )
						{
							QDomElement groups = newList.createElement( "groups" );
							newMetaContact.appendChild( groups );

							QDomNode oldGroup = oldContactElement.firstChild();
							while( !oldGroup.isNull() )
							{
								QDomElement oldGroupElement = oldGroup.toElement();
								if ( oldGroupElement.tagName() == "group" )
								{
									QDomElement group = newList.createElement( "group" );
									group.appendChild( newList.createTextNode( oldGroupElement.text() ) );
									groups.appendChild( group );
								}
								else if ( oldGroupElement.tagName() == "top-level" )
								{
									QDomElement group = newList.createElement( "top-level" );
									groups.appendChild( group );
								}

								oldGroup = oldGroup.nextSibling();
							}
						}

						else if( oldContactElement.tagName() == "address-book-field" )
						{
							// Convert address book fields.
							// Jabber will be called "xmpp", Aim/Toc and Aim/Oscar both will
							// be called "aim". MSN, AIM, IRC, Oscar and SMS don't use address
							// book fields yet; Gadu and ICQ can be converted as-is.
							// As Yahoo is unfinished we won't try to convert at all.
							QString id   = oldContactElement.attribute( "id", QString::null );
							QString data = oldContactElement.text();

							QString app, key, val;
							QChar separator = ',';
							if( id == "messaging/gadu" )
								separator = '\n';
							else if( id == "messaging/icq" )
								separator = ';';
							else if( id == "messaging/jabber" )
								id = "messaging/xmpp";

							if( id == "messaging/gadu" || id == "messaging/icq" ||
								id == "messaging/winpopup" || id == "messaging/xmpp" )
							{
								app = id;
								key = "All";
								val = data.replace( separator, QChar( 0xE000 ) );
							}

							if( !app.isEmpty() )
							{
								QDomElement addressBookField = newList.createElement( "address-book-field" );
								newMetaContact.appendChild( addressBookField );

								addressBookField.setAttribute( "app", app );
								addressBookField.setAttribute( "key", key );

								addressBookField.appendChild( newList.createTextNode( val ) );

								// ICQ didn't store the contactId locally, only in the address
								// book fields, so we need to be able to access it later
								if( id == "messaging/icq" )
									icqData = QStringList::split( QChar( 0xE000 ), val );
								else if( id == "messaging/gadu" )
									gaduData = QStringList::split( QChar( 0xE000 ), val );
							}
						}
						else if( oldContactElement.tagName() == "plugin-data" )
						{
							// Convert the plugin data
							QString id   = oldContactElement.attribute( "plugin-id", QString::null );
							QString data = oldContactElement.text();

							bool convertOldAim = false;
							uint fieldCount = 1;
							QString addressBookLabel;
							if( id == "MSNProtocol" )
							{
								fieldCount = 3;
								addressBookLabel = "msn";
							}
							else if( id == "IRCProtocol" )
							{
								fieldCount = 3;
								addressBookLabel = "irc";
							}
							else if( id == "OscarProtocol" )
							{
								fieldCount = 2;
								addressBookLabel = "aim";
							}
							else if( id == "AIMProtocol" )
							{
								id = "OscarProtocol";
								convertOldAim = true;
								addressBookLabel = "aim";
							}
							else if( id == "ICQProtocol" || id == "WPProtocol" || id == "GaduProtocol" )
							{
								fieldCount = 1;
							}
							else if( id == "JabberProtocol" )
							{
								fieldCount = 4;
							}
							else if( id == "SMSProtocol" )
							{
								// SMS used a variable serializing using a dot as delimiter.
								// The minimal count is three though (id, name, delimiter).
								fieldCount = 2;
								addressBookLabel = "sms";
							}

							if( pluginData[ id ].isNull() )
							{
								pluginData[ id ] = newList.createElement( "plugin-data" );
								pluginData[ id ].setAttribute( "plugin-id", id );
								newMetaContact.appendChild( pluginData[ id ] );
							}

							// Do the actual conversion
							if( id == "MSNProtocol" || id == "OscarProtocol" || id == "AIMProtocol" || id == "IRCProtocol" ||
								id == "ICQProtocol" || id == "JabberProtocol" || id == "SMSProtocol" || id == "WPProtocol" ||
								id == "GaduProtocol" )
							{
								QStringList strList = QStringList::split( "||", data );
								// Unescape '||'
								for( QStringList::iterator it = strList.begin(); it != strList.end(); ++it )
									( *it ).replace( QRegExp( "\\\\\\|;" ), "|" ).replace( QRegExp( "\\\\\\\\" ), "\\" );

								uint idx = 0;
								while( idx < strList.size() )
								{
									QDomElement dataField;

									dataField = newList.createElement( "plugin-data-field" );
									pluginData[ id ].appendChild( dataField );
									dataField.setAttribute( "key", "contactId" );
									if( id == "ICQProtocol" )
										dataField.appendChild( newList.createTextNode( icqData[ idx ] ) );
									if( id == "GaduProtocol" )
										dataField.appendChild( newList.createTextNode( gaduData[ idx ] ) );
									else if( id == "JabberProtocol" )
										dataField.appendChild( newList.createTextNode( strList[ idx + 1 ] ) );
									else
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );

									dataField = newList.createElement( "plugin-data-field" );
									pluginData[ id ].appendChild( dataField );
									dataField.setAttribute( "key", "displayName" );
									if( convertOldAim || id == "ICQProtocol" || id == "WPProtocol" || id == "GaduProtocol" )
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );
									else if( id == "JabberProtocol" )
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									else
										dataField.appendChild( newList.createTextNode( strList[ idx + 1 ] ) );

									if( id == "MSNProtocol" )
									{
										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "groups" );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									}
									else if( id == "IRCProtocol" )
									{
										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "serverName" );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									}
									else if( id == "JabberProtocol" )
									{
										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "identityId" );
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );

										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "groups" );
										dataField.appendChild( newList.createTextNode( strList[ idx + 3 ] ) );
									}
									else if( id == "SMSProtocol" && ( idx + 2 < strList.size() ) && strList[ idx + 2 ] != '.' )
									{
										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "serviceName" );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );

										dataField = newList.createElement( "plugin-data-field" );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( "key", "servicePrefs" );
										dataField.appendChild( newList.createTextNode( strList[ idx + 3 ] ) );

										// Add extra fields
										idx += 2;
									}

									// MSN, AIM, IRC, Oscar and SMS didn't store address book fields up
									// to now, so create one
									if( id != "ICQProtocol" && id != "JabberProtocol" && id != "WPProtocol" && id != "GaduProtocol" )
									{
										QDomElement addressBookField = newList.createElement( "address-book-field" );
										newMetaContact.appendChild( addressBookField );

										addressBookField.setAttribute( "app", "messaging/" + addressBookLabel );
										addressBookField.setAttribute( "key", "All" );
										addressBookField.appendChild( newList.createTextNode( strList[ idx ] ) );
									}

									idx += fieldCount;
								}
							}
							else if( id == "ContactNotesPlugin" || id == "CryptographyPlugin" || id == "TranslatorPlugin" )
							{
								QDomElement dataField = newList.createElement( "plugin-data-field" );
								pluginData[ id ].appendChild( dataField );
								if( id == "ContactNotesPlugin" )
									dataField.setAttribute( "key", "notes" );
								else if( id == "CryptographyPlugin" )
									dataField.setAttribute( "key", "gpgKey" );
								else if( id == "TranslatorPlugin" )
									dataField.setAttribute( "key", "languageKey" );

								dataField.appendChild( newList.createTextNode( data ) );
							}
						}
					}
					oldContactNode = oldContactNode.nextSibling();
				}
			}
			else if( oldElement.tagName() == "kopete-group" )
			{
				QDomElement newGroup = newList.createElement( "kopete-group" );
				newRoot.appendChild( newGroup );

				QDomNode oldGroupNode = oldNode.firstChild();
				while( !oldGroupNode.isNull() )
				{
					QDomElement oldGroupElement = oldGroupNode.toElement();

					if( oldGroupElement.tagName() == "display-name" )
					{
						QDomElement displayName = newList.createElement( "display-name" );
						displayName.appendChild( newList.createTextNode( oldGroupElement.text() ) );
						newGroup.appendChild( displayName );
					}
					if( oldGroupElement.tagName() == "type" )
					{
						if( oldGroupElement.text() == "Temporary" )
							newGroup.setAttribute( "type", "temporary" );
						else if( oldGroupElement.text() == "TopLevel" )
							newGroup.setAttribute( "type", "top-level" );
						else
							newGroup.setAttribute( "type", "standard" );
					}
					if( oldGroupElement.tagName() == "view" )
					{
						if( oldGroupElement.text() == "collapsed" )
							newGroup.setAttribute( "view", "collapsed" );
						else
							newGroup.setAttribute( "view", "expanded" );
					}
					else if( oldGroupElement.tagName() == "plugin-data" )
					{
						// Per-group plugin data
						// FIXME: This needs updating too, ideally, convert this in a later
						//        contactlist.xml version
						QDomElement groupPluginData = newList.createElement( "plugin-data" );
						newGroup.appendChild( groupPluginData );

						groupPluginData.setAttribute( "plugin-id", oldGroupElement.attribute( "plugin-id", QString::null ) );
						groupPluginData.appendChild( newList.createTextNode( oldGroupElement.text() ) );
					}

					oldGroupNode = oldGroupNode.nextSibling();
				}
			}
			else
			{
				kdWarning( 14010 ) << k_funcinfo << "Unknown element '" << oldElement.tagName()
					<< "' in contact list!" << endl;
			}
		}
		oldNode = oldNode.nextSibling();
	}

	// Close the file, and save the new file
	contactListFile.close();

	kdDebug() << k_funcinfo << "XML output:\n" << newList.toString( 2 ) << endl;

	contactListFile.open( IO_WriteOnly );
	QTextStream stream( &contactListFile );
	stream.setEncoding( QTextStream::UnicodeUTF8 );
	stream << newList.toString( 2 );
	contactListFile.flush();
	contactListFile.close();
}

void KopeteContactList::saveXML()
{
	QString contactListFileName = locateLocal( "appdata", "contactlist.xml" );

	//kdDebug(14010) << "KopeteContactList::saveXML: Contact List File: "
	//	<< contactListFileName << endl;

	KSaveFile contactListFile( contactListFileName );
	if( contactListFile.status() == 0 )
	{
		QTextStream *stream = contactListFile.textStream();
		stream->setEncoding( QTextStream::UnicodeUTF8 );

		*stream << toXML();

		if ( !contactListFile.close() )
		{
			kdDebug(14010) << "failed to write contactlist, error code is: " << contactListFile.status() << endl;
		}
	}
	else
	{
		kdWarning(14010) << "Couldn't open contact list file "
			<< contactListFileName << ". Contact list not saved." << endl;
	}

/*	QFile contactListFile( contactListFileName );
	if( contactListFile.open( IO_WriteOnly ) )
	{
		QTextStream stream( &contactListFile );
		stream.setEncoding( QTextStream::UnicodeUTF8 );

		stream << toXML();

		contactListFile.close();
	}
	else
	{
		kdWarning(14010) << "Couldn't open contact list file "
			<< contactListFileName << ". Contact list not saved." << endl;
	}
*/
}

QString KopeteContactList::toXML()
{
	QString xml =
		"<?xml version=\"1.0\"?>\n"
		"<!DOCTYPE kopete-contact-list>\n"
		"<kopete-contact-list version=\"1.0\">\n";

	// Save group information. ie: Open/Closed, pehaps later icons? Who knows.
	KopeteGroup *groupIt;
	for( groupIt = m_groupList.first(); groupIt; groupIt = m_groupList.next() )
		xml += groupIt->toXML();

	// Save metacontact information.
	QPtrListIterator<KopeteMetaContact> metaContactIt( m_contacts );
	for( ; metaContactIt.current(); ++metaContactIt )
	{
		if(!(*metaContactIt)->isTemporary())
		{
//			kdDebug(14010) << "KopeteContactList::toXML: Saving meta contact "
//				<< ( *metaContactIt )->displayName() << endl;
			xml +=  ( *metaContactIt)->toXML();
		}
	}

	xml += "</kopete-contact-list>\n";

	return xml;
}


QStringList KopeteContactList::contacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		contacts.append( it.current()->displayName() );
	}
	return contacts;
}

QStringList KopeteContactList::contactStatuses() const
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		meta_contacts.append( QString( "%1 (%2)" ).arg(
			it.current()->displayName() ).arg(
			it.current()->statusString() ) );
	}
	return meta_contacts;
}

QStringList KopeteContactList::reachableContacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isReachable() )
			contacts.append( it.current()->displayName() );
	}
	return contacts;
}

QStringList KopeteContactList::onlineContacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isOnline() )
			contacts.append( it.current()->displayName() );
	}
	return contacts;
}

QStringList KopeteContactList::onlineContacts( const QString &protocolId ) const
{
	QStringList onlineContacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		// FIXME: This loop is not very efficient :(
		if ( it.current()->isOnline() )
		{
			QPtrList<KopeteContact> contacts = it.current()->contacts();
			QPtrListIterator<KopeteContact> cit( contacts );
			for( ; cit.current(); ++cit )
			{
				if ( cit.current()->protocol()->pluginId() == protocolId )
					onlineContacts.append( it.current()->displayName() );
			}
		}
	}
	return onlineContacts;
}

QStringList KopeteContactList::fileTransferContacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->canAcceptFiles() )
			contacts.append( it.current()->displayName() );
	}
	return contacts;
}

void KopeteContactList::sendFile( const QString &displayName, const KURL &sourceURL, 
	const QString &altFileName, const long unsigned int fileSize)
{	/*
	 * FIXME: We should be using either some kind of unique ID (kabc ID?)
	 * here, or force user to only enter unique display names. A
	 * unique identifier is needed for external DCOP refs like this!
	 */
	 
//	kdDebug(14010) << "Send To Display Name: " << displayName << "\n";

	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
//		kdDebug(14010) << "Display Name: " << it.current()->displayName() << "\n";
		if( it.current()->displayName() == displayName ) {
			it.current()->sendFile( sourceURL, altFileName, fileSize );
			return;
		}
	}
}

QStringList KopeteContactList::contactFileProtocols(QString displayName)
{
//	kdDebug(14010) << "Get contacts for: " << displayName << "\n";
	QStringList protocols;

	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if( it.current()->displayName() == displayName ) {
//			kdDebug(14010) << "Found them!" << endl;
			QPtrList<KopeteContact> mContacts = it.current()->contacts();
			kdDebug(14010) << mContacts.count() << endl;
			QPtrListIterator<KopeteContact> jt( mContacts );
			for ( ; jt.current(); ++jt )
			{
				kdDebug(14010) << "1" << jt.current()->protocol()->pluginId() << "\n";
				if( jt.current()->canAcceptFiles() ) {
					kdDebug(14010) << jt.current()->protocol()->pluginId() << "\n";
					protocols.append ( jt.current()->protocol()->pluginId() );
				}
			}
			return protocols;
		}
	}
	return QStringList();
}


KopeteGroupList KopeteContactList::groups() const
{
	return m_groupList;
}

void KopeteContactList::removeMetaContact(KopeteMetaContact *m)
{
	for(KopeteContact *c = m->contacts().first(); c ; c = m->contacts().next() )
	{
		c->slotDeleteContact();
	}
	emit metaContactDeleted( m );
	m_contacts.remove( m );
	delete m;
}

QPtrList<KopeteMetaContact> KopeteContactList::metaContacts() const
{
	return m_contacts;
}

void KopeteContactList::addGroup( KopeteGroup * g)
{
	if(!m_groupList.contains(g) )
	{
		m_groupList.append( g );
		emit groupAdded( g );
		connect( g , SIGNAL ( renamed(KopeteGroup* , const QString & )) , this , SIGNAL ( groupRenamed(KopeteGroup* , const QString & )) ) ;
	}
}

void KopeteContactList::removeGroup( KopeteGroup *g)
{
	m_groupList.remove( g );
	emit groupRemoved( g );
	delete g;
}

bool KopeteContactList::dcopAddContact( const QString &protocolName, const QString &contactId,
	const QString &displayName, KopeteMetaContact *parentContact, const QString &groupName, bool isTemporary )
{
	//Get the protocol instance
	KopeteProtocol *myProtocol = (KopeteProtocol*) LibraryLoader::pluginLoader()->searchByName( protocolName );
	
	if( myProtocol != 0L ) 
	{
		QString contactName;
		
		//If the nickName isn't specified we need to display the userId in the prompt
		if( displayName.isEmpty() || displayName.isNull() )
			contactName = contactId;
		else
			contactName = displayName;
		
		//Confirm with the user before we add the contact
		if( KMessageBox::questionYesNo( 0, i18n("An external application is attempting to add the "
				" %1 contact \"%2\" to your contact list. Do you want to allow this?"
				).arg(protocolName).arg(contactName), i18n("Allow contact?")) == 3) // Yes == 3
		{
			//User said Yes
			myProtocol->addContact( contactId, displayName, parentContact, groupName, isTemporary );
			return true;
		} else {
			//User said No
			return false;
		}
		
	} else {
		//This protocol is not loaded
		KMessageBox::error( 0, i18n("An external application has attempted to add a contact using "
				" the %1 protocol, which does not exist, or is not loaded.").arg( protocolName ),
				i18n("Missing Protocol"));
		
		return false;
	}
}

KopeteGroup * KopeteContactList::getGroup(const QString& displayName, KopeteGroup::GroupType type)
{
	if( type == KopeteGroup::Temporary )
		return KopeteGroup::temporary;
	
	KopeteGroup *groupIterator;;
	for ( groupIterator = m_groupList.first(); groupIterator; groupIterator = m_groupList.next() )
	{
		if( groupIterator->type() == type && groupIterator->displayName() == displayName )
			return groupIterator;
	}

	KopeteGroup *newGroup = new KopeteGroup( displayName, type );
	addGroup( newGroup );
	return  newGroup;
}

#include "kopetecontactlist.moc"

// vim: set noet ts=4 sts=4 sw=4:


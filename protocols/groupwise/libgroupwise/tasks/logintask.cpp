//
// C++ Implementation: logintask
//
// Description: 
//
//
// Author: SUSE AG (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "client.h"
#include "request.h"
#include "requestfactory.h"
#include "response.h"
#include "iostream.h"

#include "logintask.h"

LoginTask::LoginTask( Task * parent )
 : RequestTask( parent )
{
}

LoginTask::~LoginTask()
{
}

void LoginTask::initialise()
{
	QCString command("login");
	Request * loginRequest = client()->requestFactory()->request( command );
	Field::FieldList lst;
	lst.append( new Field::SingleField( NM_A_SZ_USERID, 0, NMFIELD_TYPE_UTF8, client()->userId() ) );
	lst.append( new Field::SingleField( NM_A_SZ_CREDENTIALS, 0, NMFIELD_TYPE_UTF8, client()->password() ) );
	lst.append( new Field::SingleField( NM_A_SZ_USER_AGENT, 0, NMFIELD_TYPE_UTF8, client()->userAgent() ) );
	lst.append( new Field::SingleField( NM_A_UD_BUILD, 0, NMFIELD_TYPE_UDWORD, 2 ) );
	lst.append( new Field::SingleField( NM_A_IP_ADDRESS, 0, NMFIELD_TYPE_UTF8, client()->ipAddress() ) );
	loginRequest->setFields( lst );
	setTransactionId( loginRequest->transactionId() );
	setTransfer( loginRequest );
}

void LoginTask::onGo()
{
	cout << "LoginTask::onGo() - sending login fields" << endl;
	send( static_cast<Request *>( transfer() ) );
}

bool LoginTask::take( Transfer * transfer )
{
	if ( !forMe( transfer ) )
		return false;
	Response * response = dynamic_cast<Response *>( transfer );
	if ( !response )
		return false;
	
	// read in myself()'s metadata fields and emit signal
	Field::FieldList loginResponseFields = response->fields();
	emit gotMyself( loginResponseFields );
	
	// create contact list
	// locate contact list
	Field::MultiField * contactList = static_cast<Field::MultiField *>( 
			*( loginResponseFields.find( NM_A_FA_CONTACT_LIST ) ) );
	// extract folder fields 
	// find a field in the contact list containing a folder
	Field::FieldListIterator it = contactList->fields().find( NM_A_FA_FOLDER );
	// get the field
	Field::MultiField * folderContainer = static_cast<Field::MultiField *>( *it );
	while ( folderContainer )
	{
		GWFolderItem folder;
		// object id
		Field::FieldListIterator currentIt = folderContainer->fields().find( NM_A_SZ_OBJECT_ID );
		Field::SingleField * current = static_cast<Field::SingleField * >( *currentIt );
		folder.id = current->value().toInt();
		// sequence number
		currentIt = folderContainer->fields().find( NM_A_SZ_SEQUENCE_NUMBER );
		current = static_cast<Field::SingleField * >( *currentIt );
		folder.sequence = current->value().toInt();
		// name 
		currentIt = folderContainer->fields().find( NM_A_SZ_DISPLAY_NAME );
		current = static_cast<Field::SingleField * >( *currentIt );
		folder.name = current->value().toString();
		cout << "Got folder: " << folder.name.ascii() << ", obj: " << folder.id << ", seq:" << folder.sequence << endl;
		// tell the world about it
		emit gotFolder( folder );
		// and look for the next folder
		it = contactList->fields().find( it, NM_A_FA_FOLDER );
		folderContainer = static_cast<Field::MultiField *>( *( ++it ) );
		// THIS LINE IS NOT SAFE - WE'RE OFF THE END OF THE FIELD LIST OR SOMETHING
	}
	// extract contact fields 
/*	while ( findContact() )
	{
		emit gotContact( fields );
		if ( findUserDetails( fields );
			emit gotContactUserRecord( fields );
	}
	
	// create privacy list
	*/
	return true;
}

#include "logintask.moc"

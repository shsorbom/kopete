/*
    msnchatsession.cpp - MSN Message Manager

    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart at kde.org>

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

#include "msnchatsession.h"

#include <qlabel.h>
#include <qimage.h>
#include <qmap.h>
#include <qpair.h>
#include <qtooltip.h>
#include <qfile.h>
#include <qiconset.h>
#include <qbuffer.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kmainwindow.h>
#include <ktoolbar.h>
#include <krun.h>
#include <ktempfile.h>
#include "kopetecontactaction.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetechatsessionmanager.h"
#include "kopeteuiglobal.h"
#include "kopeteglobal.h"
#include "kopeteview.h"
#include "private/kopeteemoticons.h"

#include "msncontact.h"
#include "msnfiletransfersocket.h"
#include "msnaccount.h"
#include "msnswitchboardsocket.h"
#include "msnnotifysocket.h"
#include "sessionclient.h"
#include "switchboardbridge.h"
#include "transport.h"

#include "config.h"

#if !defined NDEBUG
#include "msndebugrawcmddlg.h"
#endif

class MSNChatSession::MSNChatSessionPrivate
{
	public:
		QMap<QString, QPair<QString, KTempFile*> > emoticons;
};

MSNChatSession::MSNChatSession(Kopete::Protocol *protocol, const Kopete::Contact *user, Kopete::ContactPtrList others, const char *name) : Kopete::ChatSession(user, others, protocol,  name), d(new MSNChatSessionPrivate())
{
	Kopete::ChatSessionManager::self()->registerChatSession( this );
	m_chatService = 0l;
	m_timeoutTimer =0L;
	m_newSession = true;
	m_connectionTry=0;

	setInstance(protocol->instance());

	connect( this, SIGNAL( messageSent( Kopete::Message&,
		Kopete::ChatSession* ) ),
		this, SLOT( slotMessageSent( Kopete::Message&,
		Kopete::ChatSession* ) ) );

	connect( this, SIGNAL( invitation(MSNInvitation*& ,  const QString & , long unsigned int , MSNChatSession*  , MSNContact*  ) ) ,
		protocol,  SIGNAL( invitation(MSNInvitation*& ,  const QString & , long unsigned int , MSNChatSession*  , MSNContact*  ) ) );


	m_actionInvite = new KActionMenu( i18n( "&Invite" ), "kontact_contacts", actionCollection(), "msnInvite" );
	connect ( m_actionInvite->popupMenu() , SIGNAL( aboutToShow() ) , this , SLOT(slotActionInviteAboutToShow() ) ) ;

	#if !defined NDEBUG
	new KAction( i18n( "Send Raw C&ommand..." ), 0, this, SLOT( slotDebugRawCommand() ), actionCollection(), "msnDebugRawCommand" ) ;
	#endif


	m_actionNudge=new KAction( i18n( "Send Nudge" ), "bell", 0, this, SLOT(slotSendNudge() ), actionCollection(), "msnSendNudge" ) ;
#if MSN_WEBCAM
	// Invite to receive webcam action
	m_actionWebcamReceive=new KAction( i18n( "View Contact's Webcam" ), "webcamreceive",  0, this, SLOT(slotWebcamReceive()), actionCollection(), "msnWebcamReceive" ) ;

	//Send webcam action
	m_actionWebcamSend=new KAction( i18n( "Send Webcam" ), "webcamsend",  0, this, SLOT(slotWebcamSend()), actionCollection(), "msnWebcamSend" ) ;
#endif

	new KAction( i18n( "Send File" ),"attach", 0, this, SLOT( slotSendFile() ), actionCollection(), "msnSendFile" );

	MSNContact *c = static_cast<MSNContact*>( others.first() );
	(new KAction( i18n( "Request Display Picture" ), "image", 0,  this, SLOT( slotRequestPicture() ), actionCollection(), "msnRequestDisplayPicture" ))->setEnabled(!c->object().isEmpty());

	if ( !c->object().isEmpty() )
	{

		connect( c, SIGNAL( displayPictureChanged() ), this, SLOT( slotDisplayPictureChanged() ) );
		m_image = new QLabel( 0L, "kde toolbar widget" );
		new KWidgetAction( m_image, i18n( "MSN Display Picture" ), 0, this, SLOT( slotRequestPicture() ), actionCollection(), "msnDisplayPicture" );
		if(c->hasProperty(Kopete::Global::Properties::self()->photo().key())  )
		{
			//if the view doesn't exist yet, we will be unable to get the size of the toolbar
			// so when the view will exist, we will show the displaypicture.
			//How to know when a our view is created?  We can't.
			// but chances are the next created view will be for this KMM
			// And if it is not?  never mind. the icon will just be sized 22x22
			connect( Kopete::ChatSessionManager::self() , SIGNAL(viewActivated(KopeteView* )) , this, SLOT(slotDisplayPictureChanged()) );
			//it's viewActivated and not viewCreated because the view get his mainwindow only when it is shown.
		}
	}
	else
	{
		m_image = 0L;
	}

	MSNNotifySocket *notificationService = static_cast<MSNAccount*>( myself()->account() )->notifySocket();

	transport = new PeerToPeer::Transport(this);
	bridge = transport->createIndirectBridge();
	// Connect the signal/slot
// 	QObject::connect(bridge, SIGNAL(requestSwitchboard()), this,
// 	SLOT(onRequestSwitchboard()));
	bridge->connectTo(this);

	QMap<QString, QVariant> properties;
	properties["localIpAddress"] = notificationService->getLocalIP();
	properties["connectionType"] = QString::fromLatin1("Unknown-NAT");
	// properties["connectionType"] = notificationService->getEchoedClientConnectionType();
	properties["externalIpAddress"] = notificationService->localIP();

	Kopete::Contact *me = const_cast<Kopete::Contact*>(user);
	Kopete::Contact *peer = const_cast<Kopete::Contact*>(members().getFirst());

	client = new PeerToPeer::SessionClient(properties, me, peer, transport, this);
	// Connect the signal/slot
	QObject::connect(client, SIGNAL(imageReceived(KTempFile*)), this,
	SLOT(onImageReceived(KTempFile*)));
	QObject::connect(client, SIGNAL(objectReceived(const QString&, KTempFile*)), this,
	SLOT(onObjectReceived(const QString&, KTempFile*)));

	setXMLFile("msnchatui.rc");

	setMayInvite( true );
}

MSNChatSession::~MSNChatSession()
{
	delete m_image;
	//force to disconnect the switchboard
	//not needed since the m_chatService has us as parent
	//	if(m_chatService)
	//		delete m_chatService;

	QMap<unsigned long int, MSNInvitation*>::Iterator it;
	for( it = m_invitations.begin(); it != m_invitations.end() ; it = m_invitations.begin())
	{
		delete *it;
		m_invitations.remove( it );
	}

	delete d;
}

void MSNChatSession::createChat( const QString &handle,
	const QString &address, const QString &auth, const QString &ID )
{
	/* disabled because i don't want to reopen a chatwindow if we just closed it
	 * and the contact take much time to type his message
	 m_newSession= !(ID.isEmpty());
	*/

	if( m_chatService )
	{
		kdDebug(14140) << k_funcinfo << "Service already exists, disconnect them." << endl;
		delete m_chatService;
	}

//	uncomment this line if you don't want to the peer know when you close the window
//	setCanBeDeleted( false );

	m_chatService = new MSNSwitchBoardSocket( static_cast<MSNAccount*>( myself()->account() ) , this);
	m_chatService->setUseHttpMethod( static_cast<MSNAccount*>( myself()->account() )->useHttpMethod() );
	m_chatService->setHandle( myself()->account()->accountId() );
	m_chatService->setMsgHandle( handle );
	m_chatService->connectToSwitchBoard( ID, address, auth );

	connect( m_chatService, SIGNAL( userJoined(const QString&,const QString&,bool)),
		this, SLOT( slotUserJoined(const QString&,const QString&,bool) ) );
	connect( m_chatService, SIGNAL( userLeft(const QString&,const QString&)),
		this, SLOT( slotUserLeft(const QString&,const QString&) ) );
	connect( m_chatService, SIGNAL( msgReceived( Kopete::Message & ) ),
		this, SLOT( slotMessageReceived( Kopete::Message & ) ) );
	connect( m_chatService, SIGNAL( switchBoardClosed() ),
		this, SLOT( slotSwitchBoardClosed() ) );
	connect( m_chatService, SIGNAL( receivedTypingMsg( const QString &, bool ) ),
		this, SLOT( receivedTypingMsg( const QString &, bool ) ) );
	KConfig *config = KGlobal::config();
	config->setGroup( "MSN" );
	if(config->readBoolEntry( "SendTypingNotification" , true) )
	{
		connect( this, SIGNAL( myselfTyping( bool ) ),
			m_chatService, SLOT( sendTypingMsg( bool ) ) );
	}
	connect( m_chatService, SIGNAL( msgAcknowledgement(unsigned int, bool) ),
		this, SLOT( slotAcknowledgement(unsigned int, bool) ) );
	connect( m_chatService, SIGNAL( invitation( const QString&, const QString& ) ),
		this, SLOT( slotInvitation( const QString&, const QString& ) ) );
	connect( m_chatService, SIGNAL( nudgeReceived(const QString&) ),
		this, SLOT( slotNudgeReceived(const QString&) ) );
	connect( m_chatService, SIGNAL( errorMessage(int, const QString& ) ), static_cast<MSNAccount *>(myself()->account()), SLOT( slotErrorMessageReceived(int, const QString& ) ) );

	if(!m_timeoutTimer)
	{
		m_timeoutTimer=new QTimer(this);
		connect( m_timeoutTimer , SIGNAL(timeout()), this , SLOT(slotConnectionTimeout() ) );
	}
	m_timeoutTimer->start(20000,true);

	QObject::connect(m_chatService, SIGNAL(p2pData(const QString&, const QByteArray&)), this, SLOT(onP2pData(const QString&, const QByteArray&)));
	QObject::connect(m_chatService, SIGNAL(incomingEmoticon(const QString&, const QString&)), this,
	SLOT(onEmoticonKeyAndObjectReceive(const QString&, const QString&)));
}

void MSNChatSession::slotUserJoined( const QString &handle, const QString &publicName, bool IRO )
{
	delete m_timeoutTimer;
	m_timeoutTimer=0L;

	if( !account()->contacts()[ handle ] )
		account()->addContact( handle, QString::null, 0L, Kopete::Account::Temporary);

	MSNContact *c = static_cast<MSNContact*>( account()->contacts()[ handle ] );

	c->setProperty( Kopete::Global::Properties::self()->nickName() , publicName);

	if(c->clientFlags() & MSNProtocol::MSNC4 )
	{
		m_actionNudge->setEnabled(true);
	}
#if MSN_WEBCAM
	if(c->clientFlags() & MSNProtocol::SupportWebcam )
	{
		m_actionWebcamReceive->setEnabled(true);
	}
#endif

	if (members().getFirst()->contactId() == handle)
	{
		bridge->connect();
	}

	addContact(c , IRO); // don't show notificaions when we join wesalef
	if(!m_messagesQueue.empty() || !m_invitations.isEmpty())
		sendMessageQueue();

	KConfig *config = KGlobal::config();
	config->setGroup( "MSN" );
	if ( members().count()==1 && config->readNumEntry( "DownloadPicture", 1 ) >= 1 && !c->object().isEmpty() && !c->hasProperty(Kopete::Global::Properties::self()->photo().key()))
		slotRequestPicture();
}

void MSNChatSession::slotUserLeft( const QString &handle, const QString& reason )
{
	MSNContact *c = static_cast<MSNContact*>( myself()->account()->contacts()[ handle ] );
	if(c)
		removeContact(c, reason );
}



void MSNChatSession::slotSwitchBoardClosed()
{
	//kdDebug(14140) << "MSNChatSession::slotSwitchBoardClosed"  << endl;
	m_chatService->deleteLater();
	m_chatService=0l;

	bridge->disconnect();

	cleanMessageQueue( i18n("Connection closed") );

	if(m_invitations.isEmpty() && !client->isActive())
		setCanBeDeleted( true );
}

void MSNChatSession::slotMessageSent(Kopete::Message &message, Kopete::ChatSession *)
{
	m_newSession=false;
 	if(m_chatService)
	{
		if (members().count() == 0 || m_chatService->onlineStatus() != MSNSwitchBoardSocket::Connected)
		{
			m_messagesQueue.append(message);
			kdDebug(14140) << k_funcinfo << "Message added to the queue" <<endl;
			return;
		}

		Q_INT32 id = m_chatService->sendMsg(message);
		// Send the message.
// 		id = sendMessage(message);

		m_messagesSent.insert( id, message );
		message.setBg(QColor()); // clear the bgColor
		message.setBody(message.plainBody() , Kopete::Message::PlainText ); //clear every custom tag which are not sent

		appendMessage(message);
	}
	else // There's no switchboard available, so we must create a new one!
	{
		startChatSession();
		m_messagesQueue.append(message);
	}
}

void MSNChatSession::slotMessageReceived( Kopete::Message &msg )
{
	m_newSession=false;
	if( msg.plainBody().startsWith( "AutoMessage: " ) )
	{
		//FIXME: HardCodded color are not so good
		msg.setFg( QColor( "SlateGray3" ) );
		QFont f;
		f.setItalic( true );
		msg.setFont( f );
	}
	appendMessage( msg );
}

void MSNChatSession::slotActionInviteAboutToShow()
{
	// We can't simply insert  KAction in this menu bebause we don't know when to delete them.
	//  items inserted with insert items are automatically deleted when we call clear

	m_inviteactions.setAutoDelete(true);
	m_inviteactions.clear();

	m_actionInvite->popupMenu()->clear();


	QDictIterator<Kopete::Contact> it( account()->contacts() );
	for( ; it.current(); ++it )
	{
		if( !members().contains( it.current() ) && it.current()->isOnline() && it.current() != myself() )
		{
			KAction *a=new KopeteContactAction( it.current(), this,
				SLOT( slotInviteContact( Kopete::Contact * ) ), m_actionInvite );
			m_actionInvite->insert( a );
			m_inviteactions.append( a ) ;
		}
	}
	KAction *b=new KAction( i18n ("Other..."), 0, this, SLOT( slotInviteOtherContact() ), m_actionInvite, "actionOther" );
	m_actionInvite->insert( b );
	m_inviteactions.append( b ) ;
}

void MSNChatSession::slotCloseSession()
{
	kdDebug(14140) << k_funcinfo  << m_chatService <<endl;
	if(m_chatService)
		m_chatService->slotCloseSession();
}

void MSNChatSession::slotInviteContact( Kopete::Contact *contact )
{
	if(contact)
		inviteContact( contact->contactId() );
}

void MSNChatSession::inviteContact(const QString &contactId)
{
	if( m_chatService )
		m_chatService->slotInviteContact( contactId );
	else
		startChatSession();
}

void MSNChatSession::slotInviteOtherContact()
{
	bool ok;
	QString handle = KInputDialog::getText(i18n( "MSN Plugin" ),
			i18n( "Please enter the email address of the person you want to invite:" ),
			QString::null, &ok );
	if( !ok )
		return;

	if( handle.contains('@') != 1 || handle.contains('.') <1)
	{
			KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
					i18n("<qt>You must enter a valid email address.</qt>"), i18n("MSN Plugin"));
			return;
	}

	inviteContact(handle);
}


void MSNChatSession::sendMessageQueue()
{
	if(!m_chatService)
	{
		kdDebug(14140) <<k_funcinfo << "Service doesn't exist" <<endl;
		return;
	}
//	kdDebug(14140) << "MSNChatSession::sendMessageQueue: " << m_messagesQueue.count() <<endl;
	for ( QValueList<Kopete::Message>::iterator it = m_messagesQueue.begin(); it!=m_messagesQueue.end(); it = m_messagesQueue.begin() )
	{
		//m_chatService->sendMsg( *it)  ;
		slotMessageSent(*it , this);
		m_messagesQueue.remove(it);
	}


	QMap<unsigned long int, MSNInvitation*>::Iterator it;
	for( it = m_invitations.begin(); it != m_invitations.end() ; ++it)
	{
		if(! (*it)->incoming() && (*it)->state()<MSNInvitation::Invited)
		{
			m_chatService->sendCommand( "MSG" , "N", true, (*it)->invitationHead().utf8() );
			(*it)->setState(MSNInvitation::Invited);
		}
	}
}

void MSNChatSession::slotAcknowledgement(unsigned int id, bool ack)
{
	if ( !m_messagesSent.contains( id ) )
	{
		// This is maybe a ACK/NAK for a non-messaging message
		if (ack)
		{
			emit onSend(id);
		}
		return;
	}

	if ( !ack )
	{
		Kopete::Message m = m_messagesSent[ id ];
		QString body = i18n( "The following message has not been sent correctly:\n%1" ).arg( m.plainBody() );
		Kopete::Message msg = Kopete::Message( m.to().first(), members(), body, Kopete::Message::Internal, Kopete::Message::PlainText );
		appendMessage( msg );
		//stop the stupid animation
		messageSucceeded();
	}
	else
	{
		messageSucceeded();
	}

	m_messagesSent.remove( id );
}

void MSNChatSession::slotInvitation(const QString &handle, const QString &msg)
{
	//FIXME! a contact from another account can send a file
	MSNContact *c = static_cast<MSNContact*>( myself()->account()->contacts()[ handle ] );
	if(!c)
		return;

	QRegExp rx("Invitation-Cookie: ([0-9]*)");
	rx.search(msg);
	long unsigned int cookie=rx.cap(1).toUInt();

	if(m_invitations.contains(cookie))
	{
		MSNInvitation *msnI=m_invitations[cookie];
		msnI->parseInvitation(msg);
	}
	else if( msg.contains("Invitation-Command: INVITE") )
	{
		if( msg.contains(MSNFileTransferSocket::applicationID()) )
		{
			MSNFileTransferSocket *MFTS=new MSNFileTransferSocket(myself()->account()->accountId(),c,true,this);
			connect(MFTS, SIGNAL( done(MSNInvitation*) ) , this , SLOT( invitationDone(MSNInvitation*) ));
			m_invitations.insert( cookie  , MFTS);
			MFTS->parseInvitation(msg);
			setCanBeDeleted(false);
		}
		else
		{
			MSNInvitation *i=0l;
			emit invitation( i , msg, cookie, this, c );
			if(i)
			{
				m_invitations.insert( cookie  , i );
				//don't delete this if all invitation are not done
				setCanBeDeleted(false);
			}
			else
			{
				rx=QRegExp("Application-Name: ([^\\r\\n]*)");
				rx.search(msg);
				QString inviteName = rx.cap( 1 );

				QString body = i18n(
					"%1 has sent an unimplemented invitation, the invitation was rejected.\n"
					"The invitation was: %2" )
						.arg( c->property( Kopete::Global::Properties::self()->nickName()).value().toString(), inviteName );
				Kopete::Message tmpMsg = Kopete::Message( c , members() , body , Kopete::Message::Internal, Kopete::Message::PlainText);
				appendMessage(tmpMsg);

				m_chatService->sendCommand( "MSG" , "N", true, MSNInvitation::unimplemented(cookie) );
			}
		}
	}
}

void MSNChatSession::invitationDone(MSNInvitation* MFTS)
{
	kdDebug(14140) << k_funcinfo <<endl;
	m_invitations.remove(MFTS->cookie());
//	MFTS->deleteLater();
	delete MFTS;
	if(!m_chatService && m_invitations.isEmpty())
		setCanBeDeleted(true);
}

void MSNChatSession::sendFile(const QString &fileLocation, const QString &/*fileName*/,
	long unsigned int fileSize)
{
	Q_UNUSED(fileSize);

	client->sendFile(fileLocation);
}

void MSNChatSession::initInvitation(MSNInvitation* invitation)
{
	connect(invitation->object(), SIGNAL( done(MSNInvitation*) ) , this , SLOT( invitationDone(MSNInvitation*) ));
	m_invitations.insert( invitation->cookie() , invitation);

	if(m_chatService)
	{
		m_chatService->sendCommand( "MSG" , "N", true, invitation->invitationHead().utf8() );
		invitation->setState(MSNInvitation::Invited);
	}
	else
	{
		startChatSession();
	}
}

void MSNChatSession::slotRequestPicture()
{
	QPtrList<Kopete::Contact> mb=members();
	MSNContact *c = static_cast<MSNContact*>( mb.first() );
	if(!c)
	 return;

	if( !c->hasProperty(Kopete::Global::Properties::self()->photo().key()))
	{
		if(m_chatService)
		{
			if( !c->object().isEmpty() )
			{
				client->requestObject(c->object());
			}
		}
		else if(myself()->onlineStatus().isDefinitelyOnline()  && myself()->onlineStatus().status() != Kopete::OnlineStatus::Invisible )
			startChatSession();
	}
	else
	{ //we already have the picture, just show it.
		KRun::runURL( KURL::fromPathOrURL( c->property(Kopete::Global::Properties::self()->photo()).value().toString() ), "image/png" );
	}

}

void MSNChatSession::slotDisplayPictureChanged()
{
	QPtrList<Kopete::Contact> mb=members();
	MSNContact *c = static_cast<MSNContact *>( mb.first() );
	if ( c && m_image )
	{
		if(c->hasProperty(Kopete::Global::Properties::self()->photo().key()))
		{
			int sz=22;
			// get the size of the toolbar were the aciton is plugged.
			//  if you know a better way to get the toolbar, let me know
			KMainWindow *w= view(false) ? dynamic_cast<KMainWindow*>( view(false)->mainWidget()->topLevelWidget() ) : 0L;
			if(w)
			{
				//We connected that in the constructor.  we don't need to keep this slot active.
				disconnect( Kopete::ChatSessionManager::self() , SIGNAL(viewActivated(KopeteView* )) , this, SLOT(slotDisplayPictureChanged()) );

				QPtrListIterator<KToolBar>  it=w->toolBarIterator() ;
				KAction *imgAction=actionCollection()->action("msnDisplayPicture");
				if(imgAction)  while(it)
				{
					KToolBar *tb=*it;
					if(imgAction->isPlugged(tb))
					{
						sz=tb->iconSize();
						//ipdate if the size of the toolbar change.
						disconnect(tb, SIGNAL(modechange()), this, SLOT(slotDisplayPictureChanged()));
						connect(tb, SIGNAL(modechange()), this, SLOT(slotDisplayPictureChanged()));
						break;
					}
					++it;
				}
			}
			QString imgURL=c->property(Kopete::Global::Properties::self()->photo()).value().toString();
			QImage scaledImg = QPixmap( imgURL ).convertToImage().smoothScale( sz, sz );
			if(!scaledImg.isNull())
				m_image->setPixmap( scaledImg );
			else
			{ //the image has maybe not been transfered correctly..  force to download again
				c->removeProperty(Kopete::Global::Properties::self()->photo());
				//slotDisplayPictureChanged(); //don't do that or we might end in a infinite loop
			}
			QToolTip::add( m_image, "<qt><img src=\"" + imgURL + "\"></qt>" );

		}
		else
		{
			KConfig *config = KGlobal::config();
			config->setGroup( "MSN" );
			if ( config->readNumEntry( "DownloadPicture", 1 ) >= 1 && !c->object().isEmpty() )
				slotRequestPicture();
		}
	}
}

void MSNChatSession::slotDebugRawCommand()
{
#if !defined NDEBUG
	if ( !m_chatService )
		return;

	MSNDebugRawCmdDlg *dlg = new MSNDebugRawCmdDlg( 0L );
	int result = dlg->exec();
	if( result == QDialog::Accepted && m_chatService )
	{
		m_chatService->sendCommand( dlg->command(), dlg->params(),
					dlg->addId(), dlg->msg().replace("\n","\r\n").utf8() );
	}
	delete dlg;
#endif
}


void MSNChatSession::receivedTypingMsg( const QString &contactId, bool b )
{
	MSNContact *c = dynamic_cast<MSNContact *>( account()->contacts()[ contactId ] );
	if(c && m_newSession &&  !view(false))
	{
		//this was originaly in MSNAccount::slotCreateChat
		KGlobal::config()->setGroup( "MSN" );
		bool notifyNewChat = KGlobal::config()->readBoolEntry( "NotifyNewChat", false );
		if (  notifyNewChat  )
		{
			// this internal message should open the window if they not exist
			QString body = i18n( "%1 has started a chat with you" ).arg( c->metaContact()->displayName() );
			Kopete::Message tmpMsg = Kopete::Message( c, members(), body, Kopete::Message::Internal, Kopete::Message::PlainText );
			appendMessage( tmpMsg );
		}
	}
	m_newSession=false;
	if(c)
		Kopete::ChatSession::receivedTypingMsg(c,b);
}

void MSNChatSession::slotSendNudge()
{
	if(m_chatService)
	{
		m_chatService->sendNudge();
		Kopete::Message msg = Kopete::Message( myself(), members() , i18n ( "has sent a nudge" ),  Kopete::Message::Outbound,
											   Kopete::Message::PlainText, QString(), Kopete::Message::TypeAction );
		appendMessage( msg );

	}
}


void MSNChatSession::slotNudgeReceived(const QString& handle)
{
	Kopete::Contact *c = account()->contacts()[ handle ] ;
	if(!c)
		c=members().getFirst();
	Kopete::Message msg = Kopete::Message(c, myself(), i18n ( "has sent you a nudge" ), Kopete::Message::Inbound,
										  Kopete::Message::PlainText, QString(), Kopete::Message::TypeAction );
	appendMessage( msg );
	// Emit the nudge/buzz notification (configured by user).
	emitNudgeNotification();
}


void MSNChatSession::slotWebcamReceive()
{
#if MSN_WEBCAM
// 	if(m_chatService && members().getFirst())
// 	{
// 		m_chatService->PeerDispatcher()->startWebcam(myself()->contactId() , members().getFirst()->contactId() , true);
// 	}
#endif
}

void MSNChatSession::slotWebcamSend()
{
#if MSN_WEBCAM
	kdDebug(14140) << k_funcinfo << endl;
// 	if(m_chatService && members().getFirst())
// 	{
// 		m_chatService->PeerDispatcher()->startWebcam(myself()->contactId() , members().getFirst()->contactId() , false);
// 	}
#endif
}


void MSNChatSession::slotSendFile()
{
	QPtrList<Kopete::Contact>contacts = members();
	static_cast<MSNContact *>(contacts.first())->sendFile();
}

void MSNChatSession::startChatSession()
{
	QPtrList<Kopete::Contact> mb=members();
	static_cast<MSNAccount*>( account() )->slotStartChatSession( mb.first()->contactId() );

	if(!m_timeoutTimer)
	{
		m_timeoutTimer=new QTimer(this);
		connect( m_timeoutTimer , SIGNAL(timeout()), this , SLOT(slotConnectionTimeout() ) );
	}
	m_timeoutTimer->start(20000, true);
}


void MSNChatSession::cleanMessageQueue( const QString & reason )
{
	delete m_timeoutTimer;
	m_timeoutTimer=0L;

	uint nb=m_messagesQueue.count()+m_messagesSent.count();
	if(nb==0)
		return;
	else if(nb==1)
	{
		Kopete::Message m;
		if(m_messagesQueue.count() == 1)
			m=m_messagesQueue.first();
		else
			m=m_messagesSent.begin().data();

		QString body=i18n("The following message has not been sent correctly  (%1): \n%2").arg(reason, m.plainBody());
		Kopete::Message msg = Kopete::Message(m.to().first() , members() , body , Kopete::Message::Internal, Kopete::Message::PlainText);
		appendMessage(msg);
	}
	else
	{
		Kopete::Message m;
		QString body=i18n("These messages have not been sent correctly (%1): <br /><ul>").arg(reason);
		for ( QMap<unsigned int , Kopete::Message>::iterator it = m_messagesSent.begin(); it!=m_messagesSent.end(); it = m_messagesSent.begin() )
		{
			m=it.data();
			body+= "<li>"+m.escapedBody()+"</li>";
			m_messagesSent.remove(it);
		}
		for ( QValueList<Kopete::Message>::iterator it = m_messagesQueue.begin(); it!=m_messagesQueue.end(); it = m_messagesQueue.begin() )
		{
			m=(*it);
			body+= "<li>"+m.escapedBody()+"</li>";
			m_messagesQueue.remove(it);
		}
		body+="</ul>";
		Kopete::Message msg = Kopete::Message(m.to().first() , members() , body , Kopete::Message::Internal, Kopete::Message::RichText);
		appendMessage(msg);

	}
	m_messagesQueue.clear();
	m_messagesSent.clear();
	messageSucceeded(); //stop stupid animation
}

void MSNChatSession::slotConnectionTimeout()
{
	m_connectionTry++;
	if(m_chatService)
	{
		disconnect(m_chatService , 0 , this , 0 );
		m_chatService->deleteLater();
		m_chatService=0L;
	}

	if( m_connectionTry > 3 )
	{
		cleanMessageQueue( i18n("Impossible to establish the connection") );
		delete m_timeoutTimer;
		m_timeoutTimer=0L;
		return;
	}
	startChatSession();

}














//BEGIN Switchboard Event Handling Functions

void MSNChatSession::onEmoticonKeyAndObjectReceive(const QString& key, const QString& msnobject)
{
	if (!d->emoticons.contains(msnobject) || d->emoticons[msnobject].second == 0l)
	{
		KTempFile *file = 0l;
		d->emoticons.insert(msnobject, qMakePair(key, file));

		client->requestObject(msnobject);
	}
}

// void MSNChatSession::onIncomingTextMessage(const MimeMessage& message, const QString& userFrom)
// {
// 	Kopete::Contact *contact = account()->contacts()[userFrom];
// 	if (!contact)
// 	{
// 		contact = members().getFirst();
// 	}
//	// Get the mime message message id header value.
// 	const QString messageId = message.headers().getValue("Message-ID");
	// Get the mime message body as a string.
// 	const QString textMessage = QString::fromUtf8(message.body());
// 	Kopete::Message m(contact, myself(), members(), textMessage, Kopete::Message::Inbound , Kopete::Message::PlainText);
//
// 	if (message.headers().contains("Chunks"))
// 	{
// 		m.setBody(textMessage, Kopete::Message::PlainText);
// 		d->partialMessages.insert(messageId, m);
// 	}
// 	else
// 	if (message.headers().contains("Chunk") && !d->partialMessages.isEmpty())
// 	{
// 		if (d->partialMessages.contains(messageId))
// 		{
// 			m.setBody(d->partialMessages[messageId].plainBody() + textMessage, Kopete::Message::PlainText);
// 		}
// 	}
// }

//END

//BEGIN Instant Message Functions

QString MSNChatSession::buildIMFormatString(QFont font, QColor color, bool isRightToLeft)
{
	QString format;
	QString space = QString::fromLatin1(" ");
	if (font != QFont())
	{
		/// @note font name cannot be larger than 31 characters.
		format += QString("FN=%1;").arg(font.family().left(31));

		QString effect;
		if (font.strikeOut())
		{
			effect = QString::fromLatin1("S");
		}

		if (font.bold())
		{
			effect = QString::fromLatin1("B");
		}

		if (font.italic())
		{
			effect = QString::fromLatin1("I");
		}

		if (font.underline())
		{
			effect = QString::fromLatin1("U");
		}

		format += QString("EF=%1;").arg(effect);
	}
	else
	{
		/// @bug 82734 Don't know what to set as the default.
		format += QString::fromLatin1("FN=; EF=;");
	}

	format += space;

	if (color.isValid())
	{
		/// @note colors are sent as BGR format not RGB.
		QColor bgr = QColor(color.blue(), color.green(), color.red());
		format += QString("CO=%1;").arg(bgr.name().remove(0, 1));
	}
	else
	{
		format += QString::fromLatin1("CO=0;");
	}

	format += space;
	format += QString::fromLatin1("CS=0; PF=0");
	if (isRightToLeft)
	{
		format += QString::fromLatin1("; RL=1");
	}

	return format;
}

Q_INT32 MSNChatSession::sendMessage(Kopete::Message &message)
{
	// Get the msn plugin's configuration settings.
	KConfig *config = KGlobal::config();
	config->setGroup("MSN");

	// Check whether the sending of emotions is allowed.
	if (config->readBoolEntry("exportEmoticons", false))
	{
		// If so, get the current kopete emoticon collection.
		QMap<QString, QStringList> emotions = Kopete::Emoticons::self()->emoticonAndPicList();
		QMap<QString, QStringList>::ConstIterator it;
		// Check whether the outbound message contains any of
		// the emotions from the kopete emotions collection.
		for(it = emotions.begin(); it != emotions.end(); ++it)
		{
			QStringList::ConstIterator item;
			for(item = it.data().constBegin(); item != it.data().constEnd(); ++item)
			{
				if (message.plainBody().contains(*item))
				{
					// If the message contains emoticons, notify the contact.
// 					sendEmoticonKeyAndObject(*item, it.key());
				}
			}
		}
	}

	// Get the text message body.
	QString textBody = message.plainBody().replace("\n", "\r\n").utf8();

	// Create a new outbound mime message.
	MimeMessage outbound;

	// Get the mime message header collection.
	QMap<QString, QVariant> & headers = outbound.headers();
	// Set the mime message content type header.
	headers["Content-Type"] = QString::fromLatin1("text/plain; charset=UTF-8");
	// Set the mime message IM format header.
	headers["X-MMS-IM-Format"] = buildIMFormatString(message.font(), message.fg(), textBody.isRightToLeft());

	/// @note "User-Agent" is not an official header but GAIM used it.
	// Check whether the sending of the user agent header is allowed.
	if (config->readBoolEntry("SendClientInfo", false))
	{
		// If so, set the mime message user agent header.
		headers["User-Agent"] = "Kopete/" + kapp->aboutData()->version();
	}

	/// @todo Verify what the max chunk size for a chunked text message is.
	const Q_UINT32 chunkSize = 1305;

	// Convert the text message body into a byte array.
	QByteArray messageBody = textBody.utf8();
	// Get the size of the byte array.
	const Q_UINT32 messageSize = messageBody.size();

	Q_INT32 value = -1;

	if (messageSize > chunkSize)
	{
		// Set the mime message message id header.
		headers["Message-ID"] = QUuid::createUuid().toString();

		// Determine the number of chunks that will be sent.
		Q_UINT32 chunks = messageSize / chunkSize;
		if((messageSize % chunkSize) > 0) chunks += 1;

		Q_UINT32 offset = 0, chunkId  = 1;

		while(offset < messageSize)
		{
			headers[offset == 0 ? "Chunks" : "Chunk"] = (offset == 0 ? chunks : chunkId);

			const Q_UINT32 count = messageSize - offset;
			// Determine the size of the message chunk.
			const Q_UINT32 length = QMIN(chunkSize, count);

			QByteArray chunk;
			chunk.duplicate(messageBody.data() + offset, length);
			// Set the mime message body.
			outbound.setBody(chunk);

			// Send the mime message.
			value = sendMessage(outbound, "A");

			offset += length;
			chunkId  += 1;
		}
	}
	else
	{
		// Set the mime message body.
		outbound.setBody(messageBody);

		// Send the mime message.
		value = sendMessage(outbound, "A");
	}

	return value;
}

Q_INT32 MSNChatSession::sendMessage(MimeMessage& message, const QString& type)
{
	return 0;
}

//END

//BEGIN P2P Event Handling Functions

void MSNChatSession::onP2pData(const QString& from, const QByteArray& bytes)
{
	Q_UNUSED(from);

	uint index = 0;
	// Determine the end position of the message header.
	while(index < bytes.size())
	{
		if(bytes[index++] == '\n'){
			if(bytes[index - 3] == '\n')
				break;
		}
	}

	// Retrieve the message header.
	QString messageHeader = QCString(bytes.data(), index);
	QRegExp regex("P2P-Dest: ([^\r\n]*)");
	regex.search(messageHeader);
	QString destination = regex.cap(1);

	if (destination != myself()->contactId())
	{
		kdDebug(14140) <<k_funcinfo << "Got message which is not for me -- ignoring" <<endl;
		return;
	}

	QByteArray buffer;
	buffer.duplicate(index + bytes.data(), bytes.size() - index);
	emit dataReceived(buffer);
}

void MSNChatSession::onImageReceived(KTempFile* temporaryFile)
{
	QString body = i18n("<img src=\"%1\" alt=\"Ink image\" />").arg(temporaryFile->name());
	Kopete::Message message(members().getFirst(), myself(), body, Kopete::Message::Inbound, Kopete::Message::RichText);
	temporaryFile->setAutoDelete(true);
	appendMessage(message);
}

void MSNChatSession::onObjectReceived(const QString& object, KTempFile *temporaryFile)
{
	MSNContact *contact =static_cast<MSNContact*>(members().getFirst());
	if(contact && contact->object() == object)
	{
		contact->setDisplayPicture(temporaryFile);
	}
	else
	if (d->emoticons.contains(object))
	{
		d->emoticons[object].second = temporaryFile;

// 		QValueList<const Kopete::Message>::Iterator it;
// 		for(it = d->messageQueue.begin(); it != d->messageQueue.end(); ++it)
// 		{
// 		 	Kopete::Message message = parseCustomEmoticons(it*);
// 		}
// 		d->messageQueue.clear();
	}
	else
	{
		// Otherwise, delete the temporary file.
		delete temporaryFile;
	}
}

//END

//BEGIN P2P Functions

void MSNChatSession::onRequestSwitchboard()
{
	startChatSession();
}

Q_INT32 MSNChatSession::send(const QByteArray& bytes)
{
	QString h= QString("MIME-Version: 1.0\r\n"
			"Content-Type: application/x-msnmsgrp2p\r\n"
			"P2P-Dest: " + members().getFirst()->contactId() + "\r\n"
			"\r\n");

	QBuffer buffer;
	buffer.open(IO_WriteOnly);
	buffer.writeBlock(h.ascii(), h.length());
	buffer.writeBlock(bytes.data(), bytes.size());

	Q_INT32 tId = -1;
	if (m_chatService)
	{
		tId = m_chatService->sendCommand("MSG", "D", true, buffer.buffer(), true);
	}

	buffer.close();

	return tId;
}

//END

#include "msnchatsession.moc"

// vim: set noet ts=4 sts=4 sw=4:


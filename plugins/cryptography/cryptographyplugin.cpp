/***************************************************************************
                          cryptographyplugin.cpp  -  description
                             -------------------
    begin                : jeu nov 14 2002
    copyright            : (C) 2002 by Olivier Goffart
    email                : ogoffart@tiscalinet.be
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qstylesheet.h>
#include <qtimer.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kaction.h>
#include <kconfig.h>
#include <kgenericfactory.h>

#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetemessagemanagerfactory.h"

#include "cryptographyplugin.h"
#include "cryptographyselectuserkey.h"

#include "kgpginterface.h"
#include "ksettings/dispatcher.h"

typedef KGenericFactory<CryptographyPlugin> CryptographyPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_cryptography, CryptographyPluginFactory( "kopete_cryptography" ) );

CryptographyPlugin::CryptographyPlugin( QObject *parent, const char *name, const QStringList & /* args */ )
: KopetePlugin( CryptographyPluginFactory::instance(), parent, name ),
		m_cachedPass()
{
	if( !pluginStatic_ )
		pluginStatic_=this;

	connect( KopeteMessageManagerFactory::factory(),
		SIGNAL( aboutToDisplay( KopeteMessage & ) ),
		SLOT( slotIncomingMessage( KopeteMessage & ) ) );
	connect( KopeteMessageManagerFactory::factory(),
		SIGNAL( aboutToSend( KopeteMessage & ) ),
		SLOT( slotOutgoingMessage( KopeteMessage & ) ) );

	m_cachedPass_timer = new QTimer(this, "m_cachedPass_timer" );
	QObject::connect(m_cachedPass_timer, SIGNAL(timeout()), this, SLOT(slotForgetCachedPass() ));


	KAction *action=new KAction( i18n("&Select Cryptography Public Key..."), "kgpg", 0, this, SLOT (slotSelectContactKey()), actionCollection() , "contactSelectKey");
	connect ( KopeteContactList::contactList() , SIGNAL( metaContactSelected(bool)) , action , SLOT(setEnabled(bool)));
	action->setEnabled(KopeteContactList::contactList()->selectedMetaContacts().count()==1 );

	setXMLFile("cryptographyui.rc");
    loadSettings();
	KSettings::Dispatcher::self()->registerInstance( instance(), this, SLOT( loadSettings() ) );
}



CryptographyPlugin::~CryptographyPlugin()
{
	pluginStatic_ = 0L;
}

void CryptographyPlugin::loadSettings()
{
	KConfig *config = KGlobal::config();
	config->setGroup("Cryptography Plugin");

	mPrivateKeyID = config->readEntry("PGP_private_key");
	mEncrypt = config->readBoolEntry("Encrypt", false);
	
	if(config->readBoolEntry("Cache_Till_App_Close", false))
	  mCachePassPhrase = Keep;
	if(config->readBoolEntry("Cache_Till_Time", false))
	  mCachePassPhrase = Time;
	if(config->readBoolEntry("Cache_Never", false))
	  mCachePassPhrase = Never;
	mCacheTime = config->readNumEntry("Cache_Time", 15);
	mAskPassPhrase = config->readBoolEntry("No_Passphrase_Handling", false);
}

CryptographyPlugin* CryptographyPlugin::plugin()
{
	return pluginStatic_ ;
}

CryptographyPlugin* CryptographyPlugin::pluginStatic_ = 0L;

QCString CryptographyPlugin::cachedPass()
{
	return pluginStatic_->m_cachedPass;
}

void CryptographyPlugin::setCachedPass(const QCString& p)
{
	if(pluginStatic_->mCacheMode==Never)
		return;
	if(pluginStatic_->mCacheMode==Time)
		pluginStatic_->m_cachedPass_timer->start(pluginStatic_->mCacheTime * 60000, false);

	pluginStatic_->m_cachedPass=p;
}

bool CryptographyPlugin::passphraseHandling()
{
	return !pluginStatic_->mAskPassPhrase;
}


/*KActionCollection *CryptographyPlugin::customChatActions(KopeteMessageManager *KMM)
{
	delete m_actionCollection;

	m_actionCollection = new KActionCollection(this);
	KAction *actionTranslate = new KAction( i18n ("Translate"), 0,
		this, SLOT( slotTranslateChat() ), m_actionCollection, "actionTranslate" );
	m_actionCollection->insert( actionTranslate );

	m_currentMessageManager=KMM;
	return m_actionCollection;
}*/

void CryptographyPlugin::slotIncomingMessage( KopeteMessage& msg )
{
 	QString body=msg.plainBody();
	if(!body.startsWith("-----BEGIN PGP MESSAGE----"))
		return;

	if(msg.direction() != KopeteMessage::Inbound)
	{
		QString plainBody;
		if ( m_cachedMessages.contains( body ) )
		{
			plainBody = m_cachedMessages[ body ];
			m_cachedMessages.remove( body );
		}
		else
		{
			plainBody = KgpgInterface::KgpgDecryptText( body, mPrivateKeyID );
		}

		if(!plainBody.isEmpty())
		{
			//this is the same algoritm as in KopeteMessage::escapedBody();
			QString escapedBody = QStyleSheet::escape( plainBody );
			escapedBody.replace( QString::fromLatin1( "\n" ), QString::fromLatin1( "<br/>" ) )
				.replace( QString::fromLatin1( "\t" ), QString::fromLatin1( "&nbsp;&nbsp;&nbsp;&nbsp;" ) )
				.replace( QRegExp( QString::fromLatin1( "\\s\\s" ) ), QString::fromLatin1( "&nbsp; " ) );

			msg.setBody("<table width=\"100%\" border=0 cellspacing=0 cellpadding=0><tr bgcolor=\"#41FFFF\"><td><font size=\"-1\"><b>"+i18n("Outgoing Encrypted Message: ")+"</b></font></td></tr><tr bgcolor=\"#DDFFFF\"><td>"+escapedBody+" </td></tr></table>"
				,KopeteMessage::RichText);
		}

		//if there are too messages in cache, clear the cache
		if(m_cachedMessages.count()>5)
			m_cachedMessages.clear();
		return;
	}

	body=KgpgInterface::KgpgDecryptText(body, mPrivateKeyID);

	if(!body.isEmpty())
	{
		//this is the same algoritm as in KopeteMessage::escapedBody();
		QString escapedBody = QStyleSheet::escape( body );
		escapedBody.replace( QString::fromLatin1( "\n" ), QString::fromLatin1( "<br/>" ) )
			.replace( QString::fromLatin1( "\t" ), QString::fromLatin1( "&nbsp;&nbsp;&nbsp;&nbsp;" ) )
			.replace( QRegExp( QString::fromLatin1( "\\s\\s" ) ), QString::fromLatin1( "&nbsp; " ) );

		body="<table width=\"100%\" border=0 cellspacing=0 cellpadding=0><tr bgcolor=\"#41FF41\"><td><font size=\"-1\"><b>"+i18n("Incoming Encrypted Message: ")+"</b></font></td></tr><tr bgcolor=\"#DDFFDD\"><td>"+ escapedBody  +" </td></tr></table>";
		msg.setBody(body,KopeteMessage::RichText);
	}

}

void CryptographyPlugin::slotOutgoingMessage( KopeteMessage& msg )
{
	if(msg.direction() != KopeteMessage::Outbound)
		return;

	QStringList keys;
	QPtrList<KopeteContact> contactlist = msg.to();
	for( KopeteContact *c = contactlist.first(); c; c = contactlist.next() )
	{
		QString tmpKey;
		if( c->metaContact() )
			tmpKey = c->metaContact()->pluginData( this, "gpgKey" );
		if( tmpKey.isEmpty() )
		{
			kdDebug( 14303 ) << "CryptographyPlugin::slotOutgoingMessage: no key selected for one contact" <<endl;
			return;
		}
		keys.append( tmpKey );
	}
	// always encrypt to self, too
	if(mEncrypt)
		keys.append( mPrivateKeyID );

	QString key = keys.join( " " );

	if(key.isEmpty())
	{
		kdDebug(14303) << "CryptographyPlugin::slotOutgoingMessage: empty key" <<endl;
		return;
	}

	QString original=msg.plainBody();

	/* Code From KGPG */

	//////////////////              encode from editor
	QString encryptOptions="";

	//if (utrust==true)
		encryptOptions+=" --always-trust ";
	//if (arm==true)
		encryptOptions+=" --armor ";

	/* if (pubcryptography==true)
	{
		if (gpgversion<120) encryptOptions+=" --compress-algo 1 --cipher-algo cast5 ";
		else encryptOptions+=" --cryptography6 ";
	}*/

// if (selec==NULL) {KMessageBox::sorry(0,i18n("You have not choosen an encryption key..."));return;}

	QString resultat=KgpgInterface::KgpgEncryptText(original,key,encryptOptions);
	if (!resultat.isEmpty())
	{
		msg.setBody(resultat,KopeteMessage::PlainText);
		m_cachedMessages.insert(resultat,original);
	}
	else
		kdDebug(14303) << "CryptographyPlugin::slotOutgoingMessage: empty result" <<endl;

}

void CryptographyPlugin::slotSelectContactKey()
{
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if(!m)
		return;
	QString key = m->pluginData( this, "gpgKey" );
	CryptographySelectUserKey *opts = new CryptographySelectUserKey( key, m );
	opts->exec();
	if( opts->result() )
	{
		key = opts->publicKey();
		m->setPluginData( this, "gpgKey", key );
	}
	delete opts;
}

void CryptographyPlugin::slotForgetCachedPass()
{
	m_cachedPass=QCString();
	m_cachedPass_timer->stop();
}

#include "cryptographyplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:


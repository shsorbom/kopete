/*
    historyplugin.cpp

    Copyright (c) 2003-2004 by Olivier Goffart        <ogoffart@kde.org>
    Copyright (c) 2003 by Stefan Gehn                 <metz@gehn.net>
    Copyright (c) 2009 by Kaushik Saurabh        <roideuniverse@gmailcom>

    Kopete    (c) 2003-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "historyplugin.h"
#include <QTimer>

#include <QtCore/QList>

#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kplugininfo.h>
#include <kdeversion.h>
#include <kicon.h>
#include <kactioncollection.h>
#include <Akonadi/Monitor>

#include <qdebug.h>

#include "kopetechatsessionmanager.h"
#include "kopetemetacontact.h"
#include "kopeteview.h"
#include "kopetecontactlist.h"
#include "kopeteuiglobal.h"
#include "kopetemessageevent.h"
#include "kopeteviewplugin.h"

#include "historydialog.h"
#include "historylogger.h"
#include "historyguiclient.h"
#include "historyconfig.h"
#include <Akonadi/CollectionFetchJob>
#include <kopeteprotocol.h>
#include <kopeteaccount.h>

//for migrator.cpp
#include <QtCore/QDirIterator>
#include <QtCore/QDir>
#include <kfiledialog.h>
#include <klocalizedstring.h>
#include <history/historyxmlio.h>
#include <history/history.h>
#include <QBuffer>


typedef KGenericFactory<HistoryPlugin> HistoryPluginFactory;
static const KAboutData aboutdata("kopete_history", 0, ki18n("History") , "1.0" );
K_EXPORT_COMPONENT_FACTORY( kopete_history, HistoryPluginFactory( &aboutdata )  )

HistoryPlugin::HistoryPlugin( QObject *parent, const QStringList & /* args */ )
        : Kopete::Plugin( HistoryPluginFactory::componentData(), parent ), m_loggerFactory( this )
{
    kDebug(14310) << "********History Plugin Constructor\n\n ";
    
    Akonadi::CollectionFetchJob *fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),Akonadi::CollectionFetchJob::Recursive);
    connect(fetch,SIGNAL(finished(KJob*)),this,SLOT(collectionFetch(KJob*)) );
    
    Akonadi::Monitor *monitor = new Akonadi::Monitor;
    monitor->setCollectionMonitored(Akonadi::Collection::root());
    monitor->fetchCollection(true) ;
    connect(monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) ,
	    SLOT(collectionAddedSlot(Akonadi::Collection,Akonadi::Collection)) );
	    
    connect(monitor, SIGNAL(collectionRemoved(Akonadi::Collection)) , 
	    SLOT(collectionRemovedSlot(Akonadi::Collection)) );
	    
    KAction *viewMetaContactHistory = new KAction( KIcon("view-history"), i18n("View &History" ), this );

    actionCollection()->addAction( "viewMetaContactHistory", viewMetaContactHistory );

    viewMetaContactHistory->setShortcut(KShortcut(Qt::CTRL + Qt::Key_H));


    connect(viewMetaContactHistory, SIGNAL(triggered(bool)), this, SLOT(slotViewHistory()));
    viewMetaContactHistory->setEnabled(
        Kopete::ContactList::self()->selectedMetaContacts().count() == 1 );


    connect(Kopete::ContactList::self(), SIGNAL(metaContactSelected(bool)),
            viewMetaContactHistory, SLOT(setEnabled(bool)));


    connect(Kopete::ChatSessionManager::self(), SIGNAL(viewCreated(KopeteView*)),
            this, SLOT(slotViewCreated(KopeteView*)));


    connect(this, SIGNAL(settingsChanged()), this, SLOT(slotSettingsChanged()));

    setXMLFile("historyui.rc");
    if (detectOldHistory())
    {
        if (
            KMessageBox::questionYesNo(Kopete::UI::Global::mainWidget(),
                                       i18n( "Old history files from Kopete 0.6.x or older has been detected.\n"
                                             "Do you want to import and convert them to the new history format?" ),
                                       i18n( "History Plugin" ), KGuiItem( i18n("Import && Convert") ), KGuiItem( i18n("Do Not Import") ) ) == KMessageBox::Yes )
        {
            convertOldHistory();
        }
    }

    // Add GUI action to all existing kmm objects KMM == chat session
    // (Needed if the plugin is enabled while kopete is already running)

    QList<Kopete::ChatSession*> sessions = Kopete::ChatSessionManager::self()->sessions();
    for (QList<Kopete::ChatSession*>::Iterator it= sessions.begin(); it!=sessions.end() ; ++it)
    {
        if (!m_loggers.contains(*it))
        {
            m_loggers.insert(*it, new HistoryGUIClient( *it , this ) );
            connect( *it, SIGNAL(closing(Kopete::ChatSession*)),
                     this, SLOT(slotKMMClosed(Kopete::ChatSession*)));
        }
    }
}


HistoryPlugin::~HistoryPlugin()
{
}


void HistoryMessageLogger::handleMessage( Kopete::MessageEvent *event )
{
    connect(history,SIGNAL(messageDisplayedDoneSignal()), this, SLOT(handleMessage2()) );
    m_event=event;
    if (history)
        history->messageDisplayed( event->message() );
}

void HistoryMessageLogger::handleMessage2()
{
    disconnect(history,SIGNAL(messageDisplayedDoneSignal()),this,SLOT(handleMessage2()));
    if (!m_event.isNull() )
	MessageHandler::handleMessage( m_event );
}

void HistoryPlugin::messageDisplayed(const Kopete::Message &m)
{
    if (m.direction()==Kopete::Message::Internal || !m.manager() ||
            (m.type() == Kopete::Message::TypeFileTransferRequest && m.plainBody().isEmpty() ) )
    {	
        return;
    }
    
    Kopete::Contact * con= m.manager()->members().first();
    
    if (!m_loggers.contains(m.manager()))
    {
	m_loggers.insert(m.manager() , new HistoryGUIClient( m.manager() , this ) );

        kDebug()<<"manager has been inserted\n a GUIclient has been born";

        connect(m.manager(), SIGNAL(closing(Kopete::ChatSession*)),
                this, SLOT(slotKMMClosed(Kopete::ChatSession*)));
    }
    
    m_lastmessage=m;
    
    HistoryLogger *l=m_loggers[m.manager()]->logger();
    if (l)
    {

        QList<Kopete::Contact*> mb = m.manager()->members();
	kDebug() <<"calling append message";
	connect (l,SIGNAL(appendMessageDoneSignal()), this, SIGNAL(messageDisplayedDoneSignal()) );
	
        l->appendMessage(m,mb.first());

    }

}


void HistoryPlugin::slotViewHistory()
{
    kDebug() << "\nHistoryPlugin::slotViewHistory";
    Kopete::MetaContact *m=Kopete::ContactList::self()->selectedMetaContacts().first();
    if (m)
    {
        //int lines = HistoryConfig::number_ChatWindow();
	kDebug() <<"slotviewhistory -- entered if (m)";
        // TODO: Keep track of open dialogs and raise instead of
        // opening a new (duplicated) one
        HistoryDialog* dialog = new HistoryDialog( m , this);
        dialog->setObjectName( QLatin1String("HistoryDialog") );
    }
    else kDebug() << "m not found. will not proceed";
}


void HistoryPlugin::slotViewCreated( KopeteView* v )
{
    kDebug() << "***HistoryPlugin::slotViewCreated\n\n";
    if (v->plugin()->pluginInfo().pluginName() != QString::fromLatin1("kopete_chatwindow") )
        return;  //Email chat windows are not supported.

    bool autoChatWindow = HistoryConfig::auto_chatwindow();
    int nbAutoChatWindow = HistoryConfig::number_Auto_chatwindow();

    KopeteView *m_currentView = v;
    Kopete::ChatSession *m_currentChatSession = v->msgManager();
    
    m_currentChatSessionx=m_currentChatSession;
    m_currentViewx=m_currentView;

    if (!m_currentChatSession)
        return; //i am sorry

    const Kopete::ContactPtrList& mb = m_currentChatSession->members();
    Akonadi::Collection coll;
    Kopete::Contact *con=mb.first();
    
    if (!m_loggers.contains(m_currentChatSession))
    {	
        m_loggers.insert(m_currentChatSession , new HistoryGUIClient( m_currentChatSession , this ) );
        connect( m_currentChatSession, SIGNAL(closing(Kopete::ChatSession*)),
                 this , SLOT(slotKMMClosed(Kopete::ChatSession*)));
    }

    if (!autoChatWindow || nbAutoChatWindow == 0)
        return;

    HistoryLogger *logger = m_loggers[m_currentChatSession]->logger();
    //TODO: @roide , i dont think you need this, m_ loggerx, 
    m_loggerx=logger;

    logger->setPositionToLast();
    
    connect(logger,SIGNAL(readMessagesDoneSignal(QList<Kopete::Message>)), this,SLOT(slotViewCreated2(QList<Kopete::Message>)));
    
    Kopete::Contact * xxx;
    logger->readMessages(nbAutoChatWindow,xxx, HistoryLogger::AntiChronological, true, true);
    
}
void HistoryPlugin::slotViewCreated2(QList<Kopete::Message> msgsx)
{
    disconnect(m_loggerx,SIGNAL(readMessagesDoneSignal(QList<Kopete::Message>)), this,SLOT(slotViewCreated2(QList<Kopete::Message>)));
    QList<Kopete::Message> msgs =msgsx;
    // make sure the last message is not the one which will be appened right
    // after the view is created (and which has just been logged in)
    if (!msgs.isEmpty() && (msgs.last().plainBody() == m_lastmessage.plainBody()) &&
            (m_lastmessage.manager() == m_currentChatSessionx))
    {
        msgs.takeLast();
    }

    m_currentViewx->appendMessages( msgs );

}

void HistoryPlugin::slotKMMClosed( Kopete::ChatSession* kmm)
{
    kDebug() << "\n\nHistoryPlugin::slotKMMClosed\n\n";
    m_loggers[kmm]->deleteLater();
    m_loggers.remove(kmm);
}

void HistoryPlugin::slotSettingsChanged()
{
    kDebug(14310) << "RELOADING CONFIG";
    HistoryConfig::self()->readConfig();
}

void HistoryPlugin::collectionFetch(KJob* job)
{
    kDebug() << "\ncollectiofetch job done";
    
    if (job->error() )
    {
	kWarning() << "collection fetch job failed" <<job->errorString();
	return;
    }
  
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List collList= fetchJob->collections();
    if ( !m_kopeteChat.isValid() )
    {
	kDebug() << "m_kopetechat coll is inavlid";
	foreach ( const Akonadi::Collection &collection, collList)
	{
	    if (collection.name() == "KopeteChat")
	    {
		m_kopeteChat = collection;
	    }
	}
	
	if ( !m_kopeteChat.isValid() ) 
	{
	    kWarning() << " base collection doesnt exist. configure the resource";
	    return;
	}
	
	kDebug() <<"seting up the monitor";
	
    }

    if ( !m_kopeteChat.isValid() )
    {
	kWarning() << "the base collection kopete chat is invalid";
	return;
    }

    foreach ( const Akonadi::Collection &coll, collList)
    {
	if ( m_kopeteChat.id() == coll.parent() )
	{
	    m_collectionHash.insert(coll.name(), coll);
	    kDebug() <<coll.name();
	}
    }
    
    QHashIterator<QString, Akonadi::Collection> i(m_collectionHash);
    
    while ( i.hasNext() )
    {
	i.next();
	Akonadi::Collection c = i.value();
	
	Akonadi::Collection::List collectionList;
	
	foreach ( const Akonadi::Collection &coll, collList)
	{
	    if ( c.id() == coll.parent() )
	    {
		collectionList << coll;
		kDebug() << coll.name();
	    }
	}
	
	m_idCollectionHash.insert(c.id() , collectionList);
    }
    list() ;
}


void HistoryPlugin::collectionAddedSlot(Akonadi::Collection coll , Akonadi::Collection parent )
{
    kDebug() << " in collection adeed slot";
    
    if ( !coll.isValid() )
    {
	kWarning() << " invalid collection monotored";
	return;
    }
    else kDebug() << "collection is valid" << coll.id();
    
    if ( m_kopeteChat.id() == parent.id() )
    {
	m_collectionHash.insert(coll.name(), coll);
	Akonadi::Collection::List list;
	m_idCollectionHash.insert(coll.id(), list);
	kDebug() << "coll is child of kopete chat"<<coll.id();
    }
    else
    {
	if ( m_idCollectionHash.contains( parent.id() ) )
	{
	    Akonadi::Collection::List list;
	    list = m_idCollectionHash.value( parent.id() );
	    list.append(coll);
	    m_idCollectionHash.insert(parent.id() , list );
	    kDebug() << "grandchild" << coll.id() ;
	}
    }
    
    list();
}

void HistoryPlugin::collectionRemovedSlot(Akonadi::Collection coll)
{
    kDebug() << "\n collection removed sot" << coll.name();
    
    if ( m_kopeteChat.id() == coll.parent() )
    {
	m_collectionHash.remove( coll.name() ) ;
	m_idCollectionHash.remove( coll.id() );
	kDebug() << " coll removed. child";
    }
    else
    {
	if ( m_idCollectionHash.contains( coll.parent() ) )
	{
	    int i = m_idCollectionHash.value( coll.parent() ).indexOf( coll );
	    Akonadi::Collection::List list;
	    list = m_idCollectionHash.value( coll.parent() );
	    list.removeAt(i);
	    m_idCollectionHash.insert(coll.parent() , list);
	    kDebug() << " coll removed from list(granchild)";
	}
	
    }
    
    list();	
}

void HistoryPlugin::slotJobDone(KJob * job)
{
  if (job->error() )
    kDebug() << "**from monitorCollection to slot job done" << job->errorString();
  else 
    kDebug() <<"HistoryPlugin::slotJobDone -job has been successful result of monitor";
}

Akonadi::Collection HistoryPlugin::getCollection(QString myId, QString contactId)
{
    Akonadi::Collection coll;
    
    if ( myId.isEmpty() )
	return m_kopeteChat;
    
    if ( !m_collectionHash.contains(myId) )
      return coll;
    
    Akonadi::Collection parent = m_collectionHash.value(myId);
    
    if ( contactId.isEmpty() )
      return parent;
    
    Akonadi::Collection::List list = m_idCollectionHash.value( parent.id() ) ;
    
    foreach( const Akonadi::Collection &collection, list)
    {
	if ( collection.name() == contactId )
	      coll=collection;
    }

    return coll;
}


void HistoryPlugin::list()
{
    kDebug() << "collections in ,m_collectionHash" ;
 
    QHashIterator<QString, Akonadi::Collection> it(m_collectionHash);
    while ( it.hasNext() )
    {
	it.next() ;
	kDebug() << it.key() ;
    }
    
    QHashIterator<Akonadi::Entity::Id, QList<Akonadi::Collection> > x( m_idCollectionHash );
    
    while ( x.hasNext() )
    {
	x.next();
	Akonadi::Collection::List l = x.value() ;
	Akonadi::Collection::Id id= x.key();
	Akonadi::Collection c( id );
	kDebug() << c.remoteId() ;
	foreach ( const Akonadi::Collection coll, l)
	    kDebug() << "\t " << coll.name() ;
    }

}



void HistoryPlugin::migrateKopeteLogsToAkonadi()
{
    kDebug() << " ";
    const QString oldPath = "/home/roide/.kde4/share/apps/kopete";
    KUrl url;
    if ( !oldPath.isEmpty() )
        url = KUrl::fromPath( oldPath );
    else
        url = KUrl::fromPath( QDir::homePath() );

    const QString title = i18nc( "@title:window", "Select a folder or file" );
    const QString newPath = KFileDialog::getExistingDirectory( url, 0, title );    
    QDir dir( newPath );
    migrate(dir);
}

void HistoryPlugin::migrate(const QDir &dir )
{
  QDir directory( dir );
  directory.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );
  const QFileInfoList entries = dir.entryInfoList();
  foreach ( const QFileInfo &file ,entries )
  {
      if ( file.fileName() != "." && file.fileName() != ".." && file.isDir() )
      {
	  QDir subDir( file.absoluteFilePath() );
	  migrate( subDir );
      }
  }
  
  QStringList filters; 
  filters << QLatin1String( "*.xml" );
  const QStringList fileList = directory.entryList( filters, QDir::Files );
  foreach ( const QString &s , fileList )
  {
      QString path = directory.absolutePath() +"/"+ s;
      QFile file( path );
      
      if ( !file.open( QFile::ReadOnly ) ) 
      {
	  kDebug() << "file open error"<<file.errorString();
	  return;
      }
      
      if ( file.error() != QFile::NoError )
      {
	  kDebug() <<"some error " << file.errorString();
	  return;
      }
      
      History history;
      HistoryXmlIo::readHistoryFromXml(&file, history); // this is bool functio, so check if history is read or not
      QBuffer buff;
      buff.open(QBuffer::WriteOnly);
      HistoryXmlIo::writeHistoryToXml(history, &buff);
      kDebug() << buff.data();
  }  
}



#include "historyplugin.moc"

/*
    kopetefileconfirmdialog.cpp

    Copyright (c) 2003-2004 by Olivier Goffart       <ogoffart @ kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <QCloseEvent>

#include <klineedit.h>
#include <kconfig.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <kguiitem.h>

//#include "kopetetransfermanager.h"
#include "kopetefileconfirmdialog.h"

#include "kopetemetacontact.h"
#include "kopetecontact.h"

KopeteFileConfirmDialog::KopeteFileConfirmDialog(const Kopete::FileTransferInfo &info,const QString& description,QWidget *parent )
: KDialog( parent ), m_info( info )
{
	setCaption( i18n( "A User Would Like to Send You a File" ) );
	setButtons( KDialog::User1 | KDialog::User2 );
	setButtonGuiItem( KDialog::User1, i18n("&Refuse") );
	setButtonGuiItem( KDialog::User2, i18n("&Accept") );	

	setEscapeButton( KDialog::User1 );
	setDefaultButton( KDialog::User2 );
	setAttribute( Qt::WA_DeleteOnClose );
	m_emited=false;

	m_view=new QWidget( this );
	m_view->setObjectName( "FileConfirmView" );
	setupUi( m_view );

	m_from->setText( info.contact()->metaContact()->displayName() + QLatin1String( " <" ) +
			info.contact()->contactId() + QLatin1String( "> " ) );
	m_size->setText( KGlobal::locale()->formatNumber( long( info.size() ), 0 ) );
	m_description->setText( description );
	m_filename->setText( info.file() );

	KGlobal::config()->setGroup("File Transfer");
	const QString defaultPath=KGlobal::config()->readEntry("defaultPath" , QDir::homePath() );
	m_saveto->setText(defaultPath  + QLatin1String( "/" ) + info.file() );

	setMainWidget(m_view);

	connect(cmdBrowse, SIGNAL(clicked()), this, SLOT(slotBrowsePressed()));
}

KopeteFileConfirmDialog::~KopeteFileConfirmDialog()
{
}

void KopeteFileConfirmDialog::slotBrowsePressed()
{
	QString saveFileName = KFileDialog::getSaveFileName( m_saveto->text(), QLatin1String( "*" ), 0L , i18n( "File Transfer" ) );
	if ( !saveFileName.isNull())
	{
		m_saveto->setText(saveFileName);
	}
}

void KopeteFileConfirmDialog::slotUser2()
{
	m_emited=true;
	KUrl url = KUrl(m_saveto->text());
	if(url.isValid() && url.isLocalFile() )
	{
		const QString directory=url.directory();
		if(!directory.isEmpty())
		{
			KGlobal::config()->setGroup("File Transfer");
			KGlobal::config()->writeEntry("defaultPath" , directory );
		}

		if(QFile(m_saveto->text()).exists())
		{
			int ret=KMessageBox::warningContinueCancel(this, i18n("The file '%1' already exists.\nDo you want to overwrite it ?", m_saveto->text()) ,
					 i18n("Overwrite File") , KStdGuiItem::save());
			if(ret==KMessageBox::Cancel)
				return;
		}

		emit accepted(m_info,m_saveto->text());
		close();
	}
	else
		KMessageBox::queuedMessageBox (this, KMessageBox::Sorry, i18n("You must provide a valid local filename") );
}

void KopeteFileConfirmDialog::slotUser1()
{
	m_emited=true;
	emit refused(m_info);
	close();
}

void KopeteFileConfirmDialog::closeEvent( QCloseEvent *e)
{
	if(!m_emited)
	{
		m_emited=true;
		emit refused(m_info);
	}
	KDialog::closeEvent(e);
}

#include "kopetefileconfirmdialog.moc"


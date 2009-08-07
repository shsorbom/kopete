/*
    kopetehistorydialog.h - Kopete History Dialog

    Copyright (c) 2002 by  Richard Stellingwerff <remenic@linuxfromscratch.org>
    Copyright (c) 2004 by  Stefan Gehn <metz AT gehn.net>
    Copyright (c) 2009 by Kaushik Saurabh        <roideuniverse@gmailcom>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include <QtCore/QList>

#include <kdialog.h>
#include <kurl.h>
#include <QList>
#include <Akonadi/Collection>
#include <history/history.h>
#include "kopetemessage.h"
#include "historylogger.h"

class QTreeWidgetItem;

class KAction;
class KHTMLView;
class KHTMLPart;
namespace KParts { class BrowserArguments; class OpenUrlArguments; class Part; }

namespace Kopete { class MetaContact; }
namespace Kopete { class Contact; }
namespace Kopete { class XSLT; }

namespace Ui { class HistoryViewer; }

class DMPair
{
	public:
		DMPair() {md = QDate(0, 0, 0); mc = 0; }
		DMPair(QDate d, Kopete::MetaContact *c) { md = d; mc =c; }
		QDate date() const { return md; }
		Kopete::MetaContact* metaContact() const { return mc; }
		bool operator==(const DMPair p1) const { return p1.date() == this->date() && p1.metaContact() == this->metaContact(); }
	private:
		QDate md;
		Kopete::MetaContact *mc;
};

class CHPair
{	
	public:
	      CHPair(Kopete::MetaContact *c, History &his) { m_c =c; m_h=his;}
	      Kopete::MetaContact * contact () const { return m_c; }
	      History history() const { return m_h;}
	
	private:
		History m_h;
		Kopete::MetaContact * m_c;
};
	

/**
 * @author Richard Stellingwerff <remenic@linuxfromscratch.org>
 * @author Stefan Gehn <metz AT gehn.net>
 */
class HistoryDialog : public KDialog
{
	Q_OBJECT

	public:
		explicit HistoryDialog(Kopete::MetaContact *mc,QHash<QString,Akonadi::Collection> &collMap , QWidget* parent=0);
		~HistoryDialog();

		
//		QList<History> getHistorylist(const Kopete::Contact* , const QDate date);
//		void mapContactCollection();

		/**
		 * Calls init(Kopete::Contact *c) for each subcontact of the metacontact
		 */


	signals:
		void closing();

	private slots:
		void slotOpenURLRequest(const KUrl &url, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &);

		// Called when a date is selected in the treeview
		void dateSelected(QTreeWidgetItem *);

		void slotSearch();

		void slotSearchTextChanged(const QString& txt); // To enable/disable search button
		void slotContactChanged(int index);
		void slotFilterChanged(int index);

		void init();
		//int init item job slotContactChanged
		void initItemJobDone(KJob*);
		void progressBarSlot(KJob*);

		void slotLoadDays();
		void slotLoadDays2(QList<int>);

		void slotRightClick(const QString &url, const QPoint &point);
		void slotCopy();
		void slotCopyURL();

		void slotImportHistory();
		
		//in method search, slot when get history job is doneProgressBar
		void getHistoryJobDone(KJob*);
		void searchFinished();
		void transactionDone(KJob*);
		
		/**
		 * Show the messages in the HTML View
		 */
		void setMessages(QList<Kopete::Message>);


	private:
		enum Disabled { Prev=1, Next=2 };
		void refreshEnabled( /*Disabled*/ uint disabled );

		void initProgressBar(const QString& text, int nbSteps);
		void doneProgressBar();
//		void init(Kopete::MetaContact *mc);
//		void init(Kopete::Contact *c);

		void treeWidgetHideElements(bool s);

		/**
		 * We show history dialog to look at the log for a metacontact. Here is this metacontact.
		 */
		Kopete::MetaContact *mMetaContact;

		QList<Kopete::MetaContact*> mMetaContactList;

		// History View
		KHTMLView *mHtmlView;
		KHTMLPart *mHtmlPart;
		Ui::HistoryViewer *mMainWidget;
		Kopete::XSLT *mXsltParser;

		struct Init
		{
			QList<DMPair> dateMCList; // mc for MetaContact
		} mInit;

		bool mSearching;

		KAction *mCopyAct;
		KAction *mCopyURLAct;
		QString mURL;
		
		QHash<QString ,Akonadi::Collection > m_collectionMap;
		HistoryLogger *m_hlogger;
		QMap<QDate, QList<CHPair> > m_dateHistoryList;
};

Q_DECLARE_METATYPE(Kopete::MetaContact *)

#endif

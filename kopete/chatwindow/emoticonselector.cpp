/*
    emoticonselector.cpp

    a button that pops up a list of all emoticons and returns
    the emoticon-string if one is selected in the list

    Copyright (c) 2002      by Stefan Gehn            <metz@gehn.net>
    Copyright (c) 2003      by Martijn Klingens       <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "emoticonselector.h"
#include "kopeteemoticons.h"

#include <math.h>

#include <QMovie>
#include <qlayout.h>
#include <qobject.h>
//Added by qt3to4:
#include <QPixmap>
#include <QMouseEvent>
#include <QLabel>
#include <QGridLayout>
#include <qobject.h>
//Added by qt3to4:
#include <QHideEvent>
#include <QShowEvent>

#include <kdebug.h>

EmoticonLabel::EmoticonLabel(const QString &emoticonText, const QString &pixmapPath, QWidget *parent)
	: QLabel(parent)
{
	QMovie qm(pixmapPath);
	mText = emoticonText;
	setMovie( &qm );
	setAlignment(Qt::AlignCenter);
//	this->setToolTip(emoticonText);
	// Somehow QLabel doesn't tell a reasonable size when you use setMovie
	// although it does it correctly for setPixmap. Therefore here is a little workaround
	// to tell our minimum size.
	QPixmap p(pixmapPath);
    //
    // Some of the custom icons are rather large
    // so lets limit them to a maximum size for this display panel
    //
    if (p.width() > 32 || p.height() > 32)
        p = QPixmap(32, 32);
	setMinimumSize(p.size());
}

void EmoticonLabel::mouseReleaseEvent(QMouseEvent*)
{
	emit clicked(mText);
}

EmoticonSelector::EmoticonSelector(QWidget *parent)
	: QWidget(parent)
{
//	kDebug(14000) << k_funcinfo << "called." << endl;
	lay = 0L;
}

void EmoticonSelector::prepareList(void)
{
//	kDebug(14000) << k_funcinfo << "called." << endl;
	int row = 0;
	int col = 0;
	QMap<QString, QString> list = Kopete::Emoticons::self()->emoticonAndPicList();
	int emoticonsPerRow = static_cast<int>(sqrt(list.count()));
//	kDebug(14000) << "emoticonsPerRow=" << emoticonsPerRow << endl;

	if ( lay )
	{
// FIXME kde4, no clue what to do with that
/*		QObjectList list = queryList( "EmoticonLabel" );
//		kDebug(14000) << k_funcinfo << "There are " << list->count() << " EmoticonLabels to delete." << endl;
		list->setAutoDelete(true);
		list.clear();
		delete list;
		delete lay;
*/	}

	lay = new QGridLayout(this);
	lay->setObjectName("emoticonLayout");
	lay->setSpacing(4);
	lay->setMargin(4);
	movieList.clear();
	for (QMap<QString, QString>::Iterator it = list.begin(); it != list.end(); ++it )
	{
		QWidget *w = new EmoticonLabel(it.key(), it.value(), this);
		movieList.push_back( ((QLabel*)w)->movie() );
		connect(w, SIGNAL(clicked(const QString&)), this, SLOT(emoticonClicked(const QString&)));
//		kDebug(14000) << "adding Emoticon to row=" << row << ", col=" << col << "." << endl;
		lay->addWidget(w, row, col);
		if ( col == emoticonsPerRow )
		{
			col = 0;
			row++;
		}
		else
			col++;
	}
	resize(minimumSizeHint());
}

void EmoticonSelector::emoticonClicked(const QString &str)
{
//	kDebug(14000) << "selected emoticon '" << str << "'" << endl;
	// KDE4/Qt TODO: use qobject_cast instead.
	emit ItemSelected ( str );
	if ( isVisible() && parentWidget() &&
		parentWidget()->inherits("QPopupMenu") )
	{
		parentWidget()->close();
	}
}

void EmoticonSelector::hideEvent( QHideEvent* )
{
	kDebug( 14000 ) << k_funcinfo << endl;
	MovieList::iterator it;
	for( it = movieList.begin(); it != movieList.end(); ++it )
	{
		(*it)->setPaused(true);
	}
}

void EmoticonSelector::showEvent( QShowEvent* )
{
	kDebug( 14000 ) << k_funcinfo << endl;
	MovieList::iterator it;
	for( it = movieList.begin(); it != movieList.end(); ++it )
	{
		(*it)->setPaused(false);
	}
}

#include "emoticonselector.moc"

// vim: set noet ts=4 sts=4 sw=4:


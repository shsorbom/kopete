/*
    contactnotes.cpp  -  description

    Copyright (c) 2002      by Olivier Goffart <ogoffart@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    ***************************************************************************
    *                                                                         *
    *   This program is free software; you can redistribute it and/or modify  *
    *   it under the terms of the GNU General Public License as published by  *
    *   the Free Software Foundation; either version 2 of the License, or     *
    *   (at your option) any later version.                                   *
    *                                                                         *
    ***************************************************************************
*/

#include "contactnotesplugin.h"

#include <QAction>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <qicon.h>

#include "kopetemetacontact.h"
#include "kopetecontactlist.h"

#include "contactnotesedit.h"

#include <kactioncollection.h>

K_PLUGIN_FACTORY(ContactNotesPluginFactory, registerPlugin<ContactNotesPlugin>();
                 )

ContactNotesPlugin::ContactNotesPlugin(QObject *parent, const QVariantList & /* args */)
    : Kopete::Plugin(parent)
{
    if (pluginStatic_) {
        kDebug(14302)<<"ContactNotesPlugin::ContactNotesPlugin : plugin already initialized";
    } else {
        pluginStatic_ = this;
    }

    QAction *m_actionEdit = new QAction(QIcon::fromTheme(QStringLiteral("user-identity")), i18n("&Notes"), this);
    actionCollection()->addAction(QStringLiteral("editContactNotes"), m_actionEdit);
    connect(m_actionEdit, SIGNAL(triggered(bool)), this, SLOT(slotEditInfo()));

    connect(Kopete::ContactList::self(), SIGNAL(metaContactSelected(bool)), m_actionEdit, SLOT(setEnabled(bool)));
    m_actionEdit->setEnabled(Kopete::ContactList::self()->selectedMetaContacts().count() == 1);

    setXMLFile(QStringLiteral("contactnotesui.rc"));
}

ContactNotesPlugin::~ContactNotesPlugin()
{
    pluginStatic_ = 0L;
}

ContactNotesPlugin *ContactNotesPlugin::plugin()
{
    return pluginStatic_;
}

ContactNotesPlugin *ContactNotesPlugin::pluginStatic_ = 0L;

void ContactNotesPlugin::slotEditInfo()
{
    Kopete::MetaContact *m = Kopete::ContactList::self()->selectedMetaContacts().first();
    if (!m) {
        return;
    }
    ContactNotesEdit *e = new ContactNotesEdit(m, this);
    connect(e, SIGNAL(notesChanged(QString,Kopete::MetaContact *)), this,
            SLOT(setNotes(QString,Kopete::MetaContact *)));
    e->show();
}

QString ContactNotesPlugin::notes(Kopete::MetaContact *m)
{
    return m->pluginData(this, QStringLiteral("notes"));
}

void ContactNotesPlugin::setNotes(const QString &n, Kopete::MetaContact *m)
{
    m->setPluginData(this, QStringLiteral("notes"), n);
}

#include "contactnotesplugin.moc"

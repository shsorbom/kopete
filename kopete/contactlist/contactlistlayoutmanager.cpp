/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2009  Roman Jarosz         <kedgedev@gmail.com>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "contactlistlayoutmanager.h"

#include <KMessageBox>
#include <KStandardDirs>
#include <KUrl>
#include <KGlobal>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KStandardDirs>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QStringList>

#include "kopeteitembase.h"

namespace ContactList {

LayoutManager * LayoutManager::s_instance = 0;

LayoutManager * LayoutManager::instance()
{
	if ( s_instance == 0 )
		s_instance = new LayoutManager();

	return s_instance;
}

LayoutManager::LayoutManager()
	: QObject()
{
	m_tokens << ContactListTokenConfig( -1, "placeholder", i18n("Placeholder"), "filename-space-amarok" );
	m_tokens << ContactListTokenConfig( Qt::DisplayRole, "Name", i18n("Name"), "filename-space-amarok" );
	m_tokens << ContactListTokenConfig( Kopete::Items::StatusTitleRole, "StatusTitle", i18n("Status Title"), "filename-space-amarok" );
	m_tokens << ContactListTokenConfig( Kopete::Items::StatusMessageRole, "StatusMessage", i18n("Status Message"), "filename-space-amarok" );
	m_tokens << ContactListTokenConfig( -1, "ProtocolIcons", i18n("Protocol Icons"), "filename-space-amarok" );

	loadDefaultLayouts();
	loadUserLayouts();

	KConfigGroup config( KGlobal::config(), "ContactList Layout" );
	m_activeLayout = config.readEntry( "CurrentLayout", "Default" );
}

LayoutManager::~LayoutManager()
{}

QStringList LayoutManager::layouts()
{
	return m_layouts.keys();
}

void LayoutManager::setActiveLayout( const QString &layout )
{
	kDebug() << layout;
	m_activeLayout = layout;
	KConfigGroup config(KGlobal::config(), "ContactList Layout");
	config.writeEntry( "CurrentLayout", m_activeLayout );
	emit( activeLayoutChanged() );
}

void LayoutManager::setPreviewLayout( const ContactListLayout &layout )
{
	m_activeLayout = "%%PREVIEW%%";
	m_previewLayout = layout;
	emit( activeLayoutChanged() );
}

ContactListLayout LayoutManager::activeLayout()
{
	if ( m_activeLayout == "%%PREVIEW%%" )
		return m_previewLayout;
	return m_layouts.value( m_activeLayout );
}

void LayoutManager::loadUserLayouts()
{
	QDir layoutsDir = QDir( KStandardDirs::locateLocal( "appdata", QString::fromUtf8("contactlistlayouts") ) );

	layoutsDir.setSorting( QDir::Name );

	QStringList filters;
	filters << "*.xml" << "*.XML";
	layoutsDir.setNameFilters(filters);
	layoutsDir.setSorting( QDir::Name );

	QFileInfoList list = layoutsDir.entryInfoList();

	for ( int i = 0; i < list.size(); ++i )
	{
		QFileInfo fileInfo = list.at(i);
		kDebug() << "found user file: " << fileInfo.fileName();
		loadLayouts( layoutsDir.filePath( fileInfo.fileName() ), true );
	}
}

void LayoutManager::loadDefaultLayouts()
{
	loadLayouts( KStandardDirs::locate( "data", "kopete/DefaultContactListLayouts.xml" ), false );
}


void LayoutManager::loadLayouts( const QString &fileName, bool user )
{
	QDomDocument doc( "layouts" );
	if ( !QFile::exists( fileName ) )
	{
		kDebug() << "file " << fileName << "does not exist";
		return;
	}

	QFile file( fileName );
	if( !file.open( QIODevice::ReadOnly ) )
	{
		kDebug() << "error reading file " << fileName;
		return;
	}
	if ( !doc.setContent( &file ) )
	{
		kDebug() << "error parsing file " << fileName;
		return;
	}
	file.close();

	QDomElement layouts_element = doc.firstChildElement( "contactlist_layouts" );
	QDomNodeList layouts = layouts_element.elementsByTagName("layout");

	int index = 0;
	while ( index < layouts.size() )
	{
		QDomNode layout = layouts.item( index );
		index++;

		QString layoutName = layout.toElement().attribute( "name", "" );
		ContactListLayout currentLayout;
		currentLayout.setIsEditable( user );

		currentLayout.setLayout( parseItemConfig( layout.toElement() ) );

		if ( !layoutName.isEmpty() )
			m_layouts.insert( layoutName, currentLayout );
	}
}

LayoutItemConfig LayoutManager::parseItemConfig( const QDomElement &elem )
{
	const bool showMetaContactIcon = ( elem.attribute( "show_metacontact_icon", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );

	LayoutItemConfig config;
	config.setShowIcon( showMetaContactIcon );

	QDomNodeList rows = elem.elementsByTagName("row");

	int index = 0;
	while ( index < rows.size() )
	{
		QDomNode rowNode = rows.item( index );
		index++;

		LayoutItemConfigRow row;

		QDomNodeList elements = rowNode.toElement().elementsByTagName("element");

		int index2 = 0;
		while ( index2 < elements.size() )
		{
			QDomNode elementNode = elements.item( index2 );
			index2++;

			int value = -1;
			QString configName = elementNode.toElement().attribute( "value", "placeholder" );
			for ( int i = 0; i < m_tokens.size(); i++)
			{
				if ( m_tokens.at(i).mConfigName == configName )
				{
					kDebug() << m_tokens.at(i).mConfigName;
					value = i;
					break;
				}
			}

			QString prefix = elementNode.toElement().attribute( "prefix", QString() );
			QString sufix = elementNode.toElement().attribute( "suffix", QString() );
			qreal size = elementNode.toElement().attribute( "size", "0.0" ).toDouble();
			bool bold = ( elementNode.toElement().attribute( "bold", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
			bool italic = ( elementNode.toElement().attribute( "italic", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
			bool small = ( elementNode.toElement().attribute( "small", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
			QString alignmentString = elementNode.toElement().attribute( "alignment", "left" );
			Qt::Alignment alignment;
			

			if ( alignmentString.compare( "left", Qt::CaseInsensitive ) == 0 )
				alignment = Qt::AlignLeft | Qt::AlignVCenter;
			else if ( alignmentString.compare( "right", Qt::CaseInsensitive ) == 0 )
				alignment = Qt::AlignRight| Qt::AlignVCenter;
			else
				alignment = Qt::AlignCenter| Qt::AlignVCenter;

			row.addElement( LayoutItemConfigRowElement( value, size, bold, italic, small, alignment, prefix, sufix ) );
		}

		config.addRow( row );
	}

	return config;
}

ContactListLayout LayoutManager::layout( const QString &layout )
{
	return m_layouts.value( layout );
}

void LayoutManager::addUserLayout( const QString &name, ContactListLayout layout )
{
	layout.setIsEditable( true );
	m_layouts.insert( name, layout );

	QDomDocument doc( "layouts" );
	QDomElement layouts_element = doc.createElement( "contactlist_layouts" );
	QDomElement newLayout = createItemElement( doc, "layout", layout.layout() );
	newLayout.setAttribute( "name", name );

	doc.appendChild( layouts_element );
	layouts_element.appendChild( newLayout );

	emit( layoutListChanged() );

	kDebug() << doc.toString();
	QDir layoutsDir = QDir( KStandardDirs::locateLocal( "appdata", QString::fromUtf8("contactlistlayouts") ) );

	//make sure that this dir exists
	if ( !layoutsDir.exists() )
		layoutsDir.mkpath( KStandardDirs::locateLocal( "appdata", QString::fromUtf8("contactlistlayouts") ) );

	QFile file( layoutsDir.filePath( name + ".xml" ) );
	if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) )
		return;

	QTextStream out( &file );
	out << doc.toString();
}

QDomElement LayoutManager::createItemElement( QDomDocument doc, const QString &name, const LayoutItemConfig & item ) const
{
	QDomElement element = doc.createElement( name );

	QString showIcon = item.showIcon() ? "true" : "false";
	element.setAttribute ( "show_metacontact_icon", showIcon );

	for( int i = 0; i < item.rows(); i++ )
	{
		LayoutItemConfigRow row = item.row( i );

		QDomElement rowElement = doc.createElement( "row" );
		element.appendChild( rowElement );

		for( int j = 0; j < row.count(); j++ ) {
			LayoutItemConfigRowElement element = row.element( j );
			QDomElement elementElement = doc.createElement( "element" );

			elementElement.setAttribute ( "value", m_tokens.at(element.value()).mConfigName );
			elementElement.setAttribute ( "size", QString::number( element.size() ) );
			elementElement.setAttribute ( "bold", element.bold() ? "true" : "false" );
			elementElement.setAttribute ( "italic", element.italic() ? "true" : "false" );
			elementElement.setAttribute ( "small", element.small() ? "true" : "false" );

			QString alignmentString;
			if ( element.alignment() & Qt::AlignLeft )
				alignmentString = "left";
			else  if ( element.alignment() & Qt::AlignRight )
				alignmentString = "right";
			else
				alignmentString = "center";
			
			elementElement.setAttribute ( "alignment", alignmentString );

			rowElement.appendChild( elementElement );
		}
	}

	return element;
}

bool LayoutManager::isDefaultLayout( const QString & layout ) const
{
	if ( m_layouts.keys().contains( layout ) )
		return !m_layouts.value( layout ).isEditable();

	return false;
}

QString LayoutManager::activeLayoutName()
{
	return m_activeLayout;
}

void LayoutManager::deleteLayout( const QString & layout )
{
	//check if layout is editable
	if ( m_layouts.value( layout ).isEditable() )
	{
		QDir layoutsDir = QDir( KStandardDirs::locateLocal( "appdata", QString::fromUtf8("contactlistlayouts") ) );
		QString xmlFile = layoutsDir.path() + '/' + layout + ".xml";
		kDebug() << "deleting file: " << xmlFile;

		if ( !QFile::remove( xmlFile ) )
			kDebug() << "error deleting file....";

		m_layouts.remove( layout );
		emit( layoutListChanged() );

		if ( layout == m_activeLayout )
			setActiveLayout( "Default" );
	}
	else
		KMessageBox::sorry( 0, i18n( "The layout '%1' is one of the default layouts and cannot be deleted.", layout ),
		                    i18n( "Cannot Delete Default Layouts" ) );
}

} //namespace ContactList

#include "contactlistlayoutmanager.moc"

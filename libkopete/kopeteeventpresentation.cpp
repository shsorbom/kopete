/*
    kopeteeventpresentation.cpp - Kopete Custom Notify Data Object

    Copyright (c) 2004 by Will Stephenson     <lists@stevello.free-online.co.uk>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopeteeventpresentation.h"

KopeteEventPresentation::KopeteEventPresentation( const PresentationType type )
{
	m_type = type;
}

KopeteEventPresentation::KopeteEventPresentation( const PresentationType type, 
	const QString &content, const bool singleShot, const bool enabled )
{
	m_type = type;
	m_content = content;
	m_singleShot = singleShot;
	m_enabled = enabled;
}

KopeteEventPresentation::~KopeteEventPresentation()
{
}

KopeteEventPresentation::PresentationType KopeteEventPresentation::type()
{
	return m_type;
}

QString KopeteEventPresentation::content()
{
	return m_content;
}

bool KopeteEventPresentation::enabled()
{
	return m_enabled;
}

bool KopeteEventPresentation::singleShot()
{
	return m_singleShot;
}

void KopeteEventPresentation::setContent( const QString &content )
{
	m_content = content;
}

void KopeteEventPresentation::setEnabled( const bool enabled )
{
	m_enabled = enabled;
}

void KopeteEventPresentation::setSingleShot( const bool singleShot )
{
	m_singleShot = singleShot;
}

QString KopeteEventPresentation::toString()
{
	QString type;
	switch ( m_type )
	{
		case Sound:
			type= QString::fromLatin1("sound");
			break;
		case Message:
			type= QString::fromLatin1("message");
			break;
		case Chat:
			type= QString::fromLatin1("chat");
			break;
	}
	QString stringRep = QString::fromLatin1( "Presentation; type=%1; content=%2; enabled=%3; single shot=%4\n" ).arg(type).arg(m_content).arg(m_enabled).arg(m_singleShot);
	return stringRep;
}

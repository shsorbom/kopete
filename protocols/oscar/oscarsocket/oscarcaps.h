/*
    oscarcaps.h  -  Oscar Protocol, capabilities defines

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
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

#ifndef OSCARCAPS_H
#define OSCARCAPS_H

const int CAP_CHAT				= 0;
const int CAP_VOICE				= 1;
const int CAP_SENDFILE			= 2;
const int CAP_ISICQ				= 3;
const int CAP_IMIMAGE			= 4;
const int CAP_BUDDYICON 		= 5;
const int CAP_SAVESTOCKS		= 6;
const int CAP_GETFILE			= 7;
const int CAP_ICQSERVERRELAY	= 8;
const int CAP_GAMES				= 9;
const int CAP_GAMES2				= 10;
const int CAP_SENDBUDDYLIST	= 11;
const int CAP_RTFMSGS			= 12;
const int CAP_IS_2001			= 13;
const int CAP_TRILLIAN			= 14;
const int CAP_TRILLIANCRYPT	= 15;
const int CAP_APINFO				= 16;
const int CAP_UTF8				= 17;
const int CAP_IS_WEB				= 18;
const int CAP_INTEROPERATE		= 19;
const int CAP_KOPETE				= 20;
const int CAP_MICQ				= 21;
const int CAP_MACICQ				= 22;
const int CAP_SIMOLD				= 23;
const int CAP_SIMNEW				= 24;
const int CAP_XTRAZ				= 25;
const int CAP_STR_2001			= 26;
const int CAP_STR_2002			= 27;
const int CAP_LAST				= 28;

typedef unsigned char cap[16];
const cap oscar_caps[] =
{
	//CAP_CHAT,
	{0x74, 0x8f, 0x24, 0x20, 0x62, 0x87, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	//CAP_VOICE,
	{0x09, 0x46, 0x13, 0x41, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_SENDFILE,
	{0x09, 0x46, 0x13, 0x43, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_ISICQ,
	{0x09, 0x46, 0x13, 0x44, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_IMIMAGE,
	{0x09, 0x46, 0x13, 0x45, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_BUDDYICON,
	{0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_SAVESTOCKS,
	{0x09, 0x46, 0x13, 0x47, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_GETFILE,
	{0x09, 0x46, 0x13, 0x48, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_ICQSERVERRELAY,
	{0x09, 0x46, 0x13, 0x49, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_GAMES,
	{0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_GAMES2,
	{0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
	 0x22, 0x82, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_SENDBUDDYLIST,
	{0x09, 0x46, 0x13, 0x4b, 0x4c, 0x7f, 0x11, 0xd1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_RTFMSGS,
	{0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34,
	 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92},

	// CAP_IS_2001,
	{0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8,
	 0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf},

	// CAP_TRILLIAN
	{0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34,
		0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09},

	// CAP_TRILLIANCRYPT
	{0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb,
		0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00},

	// CAP_APINFO,
	{0xAA, 0x4A, 0x32, 0xB5, 0xF8, 0x84, 0x48, 0xc6,
	 0xA3, 0xD7, 0x8C, 0x50, 0x97, 0x19, 0xFD, 0x5B},

	// CAP_UTF8,
	{0x09, 0x46, 0x13, 0x4E, 0x4C, 0x7F, 0x11, 0xD1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_IS_WEB,
	{0x56, 0x3F, 0xC8, 0x09, 0x0B, 0x6f, 0x41, 0xBD,
	 0x9F, 0x79, 0x42, 0x26, 0x09, 0xDF, 0xA2, 0xF3},

	// CAP_INTEROPERATE,
	{0x09, 0x46, 0x13, 0x4D, 0x4C, 0x7F, 0x11, 0xD1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_KOPETE,
	// last 4 bytes determine version
	// NOTE change with each Kopete Release!
	{'K', 'o', 'p', 'e', 't', 'e', ' ', 'I',
	 'C', 'Q', ' ', ' ', 0, 8, 9, 0},

	// CAP_MICQ
	// last 4 bytes determine version
	{0x6d, 0x49, 0x43, 0x51, 0x20, 0xa9, 0x20, 0x52,
	 0x2e, 0x4b, 0x2e, 0x20, 0x00, 0x00, 0x00, 0x00},

	// CAP_MACICQ
	{0xDD, 0x16, 0xF2, 0x02, 0x84, 0xE6, 0x11, 0xD4,
	 0x90, 0xDB, 0x00, 0x10, 0x4B, 0x9B, 0x4B, 0x7D},

	// CAP_SIMOLD
	// last byte determines version
	// (major + 1) << 6 + minor
	{0x97, 0xB1, 0x27, 0x51, 0x24, 0x3C, 0x43, 0x34,
	 0xAD, 0x22, 0xD6, 0xAB, 0xF7, 0x3F, 0x14, 0x00},

	// CAP_SIMNEW
	// last 4 bytes determine version (US-ASCII encoded)
	{'S', 'I', 'M', ' ', 'c', 'l', 'i', 'e',
	 'n', 't', ' ', ' ',  0 ,  0 ,  0 , 0},

	// CAP_XTRAZ
	{0x1A, 0x09, 0x3C, 0x6C, 0xD7, 0xFD, 0x4E, 0xC5,
	 0x9D, 0x51, 0xA6, 0x47, 0x4E, 0x34, 0xF5, 0xA0},

	// CAP_STR_2001
	{0xA0, 0xE9, 0x3F, 0x37, 0x4C, 0x7F, 0x11, 0xD1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_STR_2002
	{0x10, 0xCF, 0x40, 0xD1, 0x4C, 0x7F, 0x11, 0xD1,
	 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},

	// CAP_LAST,
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

#endif

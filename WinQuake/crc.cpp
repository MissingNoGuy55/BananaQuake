/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2021-2024 Stephen "Missi" Schmiedeberg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* crc.c */

#include "quakedef.h"
#include "crc.h"

CCRCManager* g_CRCManager;

CCRCManager::CCRCManager()
{
}

void CCRCManager::CRC_Init(unsigned short *crcvalue)
{
	*crcvalue = CRC_INIT_VALUE;
}

void CCRCManager::CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
	*crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

unsigned short CCRCManager::CRC_Value(unsigned short crcvalue)
{
	return crcvalue ^ CRC_XOR_VALUE;
}

// Missi: copied from QuakeSpasm (3/28/2022)
unsigned short CCRCManager::CRC_Block(const byte* start, int count)
{
	unsigned short	crc;

	CRC_Init(&crc);
	while (count--)
		crc = (crc << 8) ^ crctable[(crc >> 8) ^ *start++];

	return crc;
}

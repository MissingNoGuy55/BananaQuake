/*
 * cfgfile.h -- misc reads from the config file
 *
 * Copyright (C) 2008-2012  O.Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __CFGFILE_H
#define __CFGFILE_H

class CGameConfig : public CCommon
{
public:
	void CFG_ReadCvars(const char** vars, int num_vars);
	void CFG_ReadCvarOverrides(const char** vars, int num_vars);
	int CFG_OpenConfig(const char* cfg_name);
	void CFG_CloseConfig(void);

	static fshandle_t* cfg_file;
};

extern CGameConfig* g_GameConfig;

#endif	/* __CFGFILE_H */

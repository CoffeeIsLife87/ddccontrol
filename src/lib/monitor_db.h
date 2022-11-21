/*
    ddc/ci interface functions header
    Copyright(c) 2004 Nicolas Boichat (nicolas@boichat.ch)
    Copyright(c) 2004 Oleg I. Vdovikin (oleg@cs.msu.su)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef MONITOR_DB_H
#define MONITOR_DB_H

#include "ddcci.h"

#include <libxml/xmlstring.h>

/* Current database version */
#define DBVERSION 3

enum ControlType {
	CTRL_TYPE_VALUE,
	CTRL_TYPE_COMMAND,
	CTRL_TYPE_LIST
};

enum RefreshType {
	REFRESH_TYPE_NONE,
	REFRESH_TYPE_ALL
};

enum InitType {
	INIT_TYPE_UNKNOWN,
	INIT_TYPE_STANDARD,
	INIT_TYPE_SAMSUNG,
};

struct ValueDB {
	xmlChar* id;
	xmlChar* name;
	unsigned char value;
	
	struct ValueDB* next;
};

struct ControlDB {
	xmlChar* id;
	xmlChar* name;
	unsigned char address;
	int delay; /* -1 indicate default value */
	enum ControlType type;
	enum RefreshType refresh;
	
	struct ControlDB* next;
	struct ValueDB* value_list;
};

struct SubgroupDB {
	xmlChar* name;
	xmlChar* pattern;
	
	struct SubgroupDB* next;
	struct ControlDB* control_list;
};

struct GroupDB {
	xmlChar* name;
	
	struct GroupDB* next;
	struct SubgroupDB* subgroup_list;
};

struct MonitorDB {
	xmlChar* name;
	enum InitType init;
	
	struct GroupDB* group_list;
};

struct MonitorDB* ddcci_create_db(const char* pnpname, struct Caps* caps, int faulttolerance);
void ddcci_free_db(struct MonitorDB* mon_db);

int ddcci_init_db(char* usedatadir);
void ddcci_release_db();

#endif

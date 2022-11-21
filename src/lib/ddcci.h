/*
    ddc/ci interface functions header
    Copyright(c) 2004 Oleg I. Vdovikin (oleg@cs.msu.su)
    Copyright(c) 2004-2005 Nicolas Boichat (nicolas@boichat.ch)

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

#ifndef DDCCI_H
#define DDCCI_H

#include <time.h>
#include <sys/time.h>

struct Profile;

enum MonitorType {
	MONITOR_TYPE_UNK,
	MONITOR_TYPE_LCD,
	MONITOR_TYPE_CRT
};

/* Structure to store CAPS vcp entry (control and related values) */
struct VcpEntry {
	int values_len; /* -1 if values are not specified */
	unsigned short* values;
};

/* Structure to store CAPS */
struct Caps {
	struct VcpEntry* vcp[256]; /* vcp entries */
	enum MonitorType type;
	char* raw_caps; /* raw text caps */
};

enum DevType {
		DEV_TYPE_DEV,
		DEV_TYPE_PCI,
		DEV_TYPE_ADL
};

#include "monitor_db.h"

struct Monitor {
	const struct Monitor_vtable *__vtable;

	int fd;
	unsigned int addr;
	int adl_adapter, adl_display;
	char pnpid[8];
	unsigned char digital; /* 0 - digital, 1 - analog */
	struct timeval last;
	struct MonitorDB* db;
	struct Caps caps;
	
	struct Profile* profiles; /* profiles available for this monitor. Filled by get_all_profiles. */
	enum DevType dev_type;
	int probing; /* are we probing? */
	
	/**
	 * 0 - the db is designed for this monitor
	 * 1 - we are using a manufacturer standard profile (warn the user)
	 * 2 - we are using the VESA generic profile (warn the user)
	*/
	int fallback;
};

/* Struct used to return monitor data probed by ddcci_probe */
struct MonitorList {
	char* filename; /* I2C device filename */
	
	unsigned char supported; /* 0 - DDC/CI not supported, 1 - DDC/CI supported */
	char* name;
	unsigned char digital; /* 0 - digital, 1 - analog */
	
	struct MonitorList* next;
};

struct MonitorList* ddcci_probe();
void ddcci_free_list(struct MonitorList* list);

int ddcci_open(struct Monitor* mon, const char* filename, int probing);
int ddcci_save(struct Monitor* mon);
int ddcci_close(struct Monitor* mon);

int ddcci_writectrl(struct Monitor* mon, unsigned char ctrl, unsigned short value, int delay);

/* return values: < 0 - failure, 0 - contron not supported, > 0 - supported */
int ddcci_readctrl(struct Monitor* mon, unsigned char ctrl, 
	unsigned short *value, unsigned short *maximum);

int ddcci_parse_caps(const char* caps_str, struct Caps* caps, int add);

int ddcci_caps(struct Monitor* mon);

/* verbosity level (0 - normal, 1 - encoded data, 2 - ddc/ci frames) */
void ddcci_verbosity(int verbosity);

int get_verbosity();

int ddcci_init(char* usedatadir);

void ddcci_release();

void ddcpci_send_heartbeat();

/* Create $HOME/.ddccontrol and subdirectories if necessary */
int ddcci_create_config_dir();

/* Macros */
#define DDCCI_DB_RETURN_IF_RUN(cond, value, message, node, run) \
	if (cond) { \
		if (node) \
			fprintf(stderr, _("Error: %s @%s:%d (%s:%ld)\n"), message, __FILE__, __LINE__, \
				((xmlNodePtr)node)->doc->name, XML_GET_LINE(node)); \
		else \
			fprintf(stderr, _("Error: %s @%s:%d\n"), message, __FILE__, __LINE__); \
		run \
		return value; \
	}

#define DDCCI_DB_RETURN_IF(cond, value, message, node) \
	DDCCI_DB_RETURN_IF_RUN(cond, value, message, node, {})
	
#define DDCCI_RETURN_IF_RUN(cond, value, message, run) \
	if (cond) { \
		fprintf(stderr, _("Error: %s @%s:%d\n"), message, __FILE__, __LINE__); \
		run \
		return value; \
	}

#define DDCCI_RETURN_IF(cond, value, message) \
	DDCCI_RETURN_IF_RUN(cond, value, message, {})

#endif //DDCCI_H

/*
    ddc/ci command line tool
    Copyright(c) 2004 Oleg I. Vdovikin (oleg@cs.msu.su)
    Copyright(c) 2004 Nicolas Boichat (nicolas@boichat.ch)

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ddcci.h"
#include "monitor_db.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"

#define RETRYS 3 /* number of retrys */

void usage(char *name)
{
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"%s [-v] [-c] [-f] [-a] dev\n", name);
	fprintf(stderr,"%s [-v] [-c] [-f] [-s] [-r ctrl] [-w value] dev\n", name);
	fprintf(stderr,"\tdev: device, e.g. /dev/i2c-0\n");
	fprintf(stderr,"\t-c : query capability\n");
	fprintf(stderr,"\t-d : query ctrls 0 - 255\n");
	fprintf(stderr,"\t-r : query ctrl\n");
	fprintf(stderr,"\t-w : value to write to ctrl\n");
	fprintf(stderr,"\t-f : force (avoid validity checks)\n");
	fprintf(stderr,"\t-s : save settings\n");
	fprintf(stderr,"\t-v : verbosity (specify more to increase)\n");
}

int main(int argc, char **argv)
{
	int i, retry, ret;
	
	/* filedescriptor and name of device */
	struct monitor mon;
	struct control_ret ctrl_ret;
	char *fn;
	
	/* what to do */
	int dump = 0;
	int ctrl = -1;
	int value = -1;
	int caps = 0;
	int save = 0;
	int force = 0;
	verbosity = 1;
	
	fprintf(stdout, "ddccontrol version " VERSION "\n");
	fprintf(stdout, "Copyright 2004 Oleg I. Vdovikin (oleg@cs.msu.su)\n");
	fprintf(stdout, "Copyright 2004 Nicolas Boichat (nicolas@boichat.ch)\n");
	fprintf(stdout, "This program comes with ABSOLUTELY NO WARRANTY.\n");
	fprintf(stdout, "You may redistribute copies of this program under the terms of the GNU General Public License.\n\n");
	
	while ((i=getopt(argc,argv,"hdr:w:csfv")) >= 0)
	{
		switch(i) {
		case 'h':
			usage(argv[0]);
			exit(1);
			break;
/*		case 'a':
				if ((addr = strtol(optarg, NULL, 0)) < 0 || (addr > 127)){
				fprintf(stderr,"'%s' does not seem to be a valid i2c address\n", optarg);
				exit(1);
				}
			break;*/
		case 'r':
				if ((ctrl = strtol(optarg, NULL, 0)) < 0 || (ctrl > 255)){
				fprintf(stderr,"'%s' does not seem to be a valid register name\n", optarg);
				exit(1);
				}
			break;
		case 'w':
			if ((value = strtol(optarg, NULL, 0)) < 0 || (value > 65535)){
				fprintf(stderr,"'%s' does not seem to be a valid value.\n", optarg);
				exit(1);
				}
				break;
		case 'c':
				caps++;
			break;
		case 'd':
			dump++;
			break;
		case 's':
			save++;
			break;
		case 'f':
			force++;
			break;
		case 'v':
			verbosity++;
			break;
		}
	}
	
	if (optind == argc) 
	{
		usage(argv[0]);
		exit(1);
	}
	
	fn = argv[optind];
	
	if ((ret = ddcci_open(&mon, fn)) < 0) {
		fprintf(stderr, "\nDDC/CI at %s is unusable (%d).\n", fn, ret);
	} else {
		if (caps) {
			unsigned char buf[1024];
			
			for (retry = RETRYS; retry; retry--) {
				if (ddcci_caps(&mon, buf, 1024) >= 0) {
					//printf("CAPS: %s\n", buf);
					break;
				}
			}
		}
		if (ctrl >= 0) {
			if (value >= 0) {
				fprintf(stdout, "\nWriting 0x%02x, 0x%02x(%d)\n",
					ctrl, value, value);
				ddcci_writectrl(&mon, ctrl, value);
			} else {
				fprintf(stdout, "\nReading 0x%02x\n", ctrl);
			}
			
			for (retry = RETRYS; retry; retry--) {
				if (ddcci_readctrl(&mon, ctrl, 1, &ctrl_ret) >= 0) {
					break;
				}
			}
		}
		else if (dump) {
			fprintf(stdout, "\nControls (valid/current/max):\n");
			
			for (i = 0; i < 256; i++) {
				for (retry = RETRYS; retry; retry--) {
					if (ddcci_readctrl(&mon, i, force, &ctrl_ret) >= 0) {
						break;
					}
				}
			}
		}
		else if (ctrl == -1) {
			struct monitor_db* monitor = mon.db;
			struct group_db* group;
			struct control_db* control;
			struct value_db* valued;
			
			verbosity--;
			if (monitor) {		
				printf("\n= %s\n", monitor->name);
				
				group = monitor->group_list;
				while (group != NULL) {
					printf("> %s\n", group->name);
					
					control = group->control_list;
					while (control != NULL) {
						printf("\t> id=%s, name=%s, address=%#x, delay=%dms, type=%d\n", 
							control->id, control->name, control->address, control->delay, control->type);
						
						valued = control->value_list;
						if (valued) {
							printf("\t  Possible values:\n");
						}
						while (valued != NULL) {
							printf("\t\t> id=%s - name=%s, value=%#x\n", valued->id, valued->name, valued->value);
							valued = valued->next;
						}
						
						for (retry = RETRYS; retry; retry--) {
							if (ddcci_readctrl(&mon, control->address, force, &ctrl_ret) >= 0) {
								printf("\t  supported=%c, value=%#x, maximum=%#x\n", 
									ctrl_ret.supported ? '+' : '-', ctrl_ret.value, ctrl_ret.maximum);
								break;
							}
						}
						
						control = control->next;
					}
					group = group->next;
				}
			}
			verbosity++;
		}
		if (save) {
			ddcci_save(&mon);
		}
	}
	
	ddcci_close(&mon);
	
	exit(0);
}
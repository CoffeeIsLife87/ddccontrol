/*
    ddc/ci D-Bus client library implementation

    Copyright(c) 2018 Miroslav Kravec (kravec.miroslav@gmail.com)

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

#include "dbus_client.h"

#include "internal.h"
#include "interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ddcci_dbus_open(DDCControl *proxy, struct monitor* mon, const char* filename) {
    char *pnpid = NULL;
    GError *error = NULL;

    memset(mon, 0, sizeof(struct monitor));

    gboolean result = ddccontrol_call_open_monitor_sync(proxy, filename, &pnpid, &mon->caps.raw_caps, NULL, &error);
    if(result == FALSE) {
        fprintf(stderr, _("Open monitor failed: %s\n."), error->message);
        return -1;
    }

    strncpy(&mon->pnpid, pnpid, 7);
    mon->pnpid[7] = 0;

    ddcci_parse_caps(mon->caps.raw_caps, &mon->caps, 1);

    // TODO: duplicated from ddcci.c
    // DUPLICATED CODE START
    mon->db = ddcci_create_db(mon->pnpid, &mon->caps, 1);
    mon->fallback = 0; /* No fallback */

    if (!mon->db) {
        /* Fallback on manufacturer generic profile */
        char buffer[7];
        buffer[0] = 0;
        strncat(buffer, mon->pnpid, 3); /* copy manufacturer id */
        switch(mon->caps.type) {
        case lcd:
            strcat(buffer, "lcd");
            mon->db = ddcci_create_db(buffer, &mon->caps, 1);
            mon->fallback = 1;
            break;
        case crt:
            strcat(buffer, "crt");
            mon->db = ddcci_create_db(buffer, &mon->caps, 1);
            mon->fallback = 1;
            break;
        case unk:
            break;
        }

        if (!mon->db) {
            /* Fallback on VESA generic profile */
            mon->db = ddcci_create_db("VESA", &mon->caps, 1);
            mon->fallback = 2;
        }
    }
    // DUPLICATED CODE END
    return 0;
}

int ddcci_dbus_readctrl(DDCControl *proxy, char *fn,
        unsigned char ctrl, unsigned short *value, unsigned short *maximum)
{
    int result;
    GError *error = NULL;

    gboolean call_result = ddccontrol_call_get_control_sync(proxy, fn, ctrl, &result, value, maximum, NULL, &error);

    if(call_result == TRUE) {
        return result;
    } else {
        fprintf(stderr, _("Control 0x%02x D-Bus read failed: %s\n."), ctrl, error->message);
        return -10;
    }
}

int ddcci_dbus_writectrl(DDCControl *proxy, char *fn,
        unsigned char ctrl, unsigned short value)
{
    GError *error = NULL;

    gboolean call_result = ddccontrol_call_set_control_sync(proxy, fn, ctrl, value, NULL, &error);

    if(call_result == TRUE) {
        // original write result
        return 0;
    } else {
        fprintf(stderr, _("Control 0x%02x D-Bus write failed: %s\n."), ctrl, error->message);
        return -10;
    }
}

struct monitorlist * ddcci_dbus_rescan_monitors(DDCControl *proxy) {
    int i;
    struct monitorlist *monlist = NULL, *current = NULL;
    
    char **devices = NULL, **names = NULL;
    char *supported = NULL, *digital = NULL;

    GError *error = NULL;
    GVariant *v_supported, *v_digital;
    size_t supported_n, digital_n;

    gboolean result = ddccontrol_call_rescan_monitors_sync(proxy, &devices, &v_supported, &names, &v_digital, NULL, &error);

    if(result == FALSE) {
        // TODO: better error output
        fprintf(stderr, _("Probe failed: %s\n."), error->message);
        return NULL;
    }

    supported = g_variant_get_fixed_array(v_supported, &supported_n, sizeof(char));
    digital = g_variant_get_fixed_array(v_digital, &digital_n, sizeof(char));

    for(i = 0; devices[i] != NULL && i < supported_n; i++) {
        if(current == NULL) {
            monlist = current = malloc(sizeof(struct monitorlist));
        } else {
            current->next = malloc(sizeof(struct monitorlist));
            current = current->next;
        }
        current->filename = devices[i];
        current->supported = supported[i];
        current->name = names[i];
        current->digital = digital[i];
    }
    if(current != NULL)
        current->next = NULL;
    return monlist;
}

DDCControl* ddcci_dbus_open_proxy() {
    GError *error = NULL;
 
    DDCControl *proxy = ddccontrol_proxy_new_for_bus_sync(
            G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_NONE,
            "ddccontrol.DDCControl",
            "/ddccontrol/DDCControl",
            NULL,
            &error
    );

    if( error != NULL ) {
        fprintf(stderr, _("D-Bus connection failed with error: %s.\n"), error->message);
        return NULL;
    }

    return proxy;
}

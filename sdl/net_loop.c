/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// net_loop.c
#include <stdint.h>
#include <qcommon.h>
#include <net.h>
#include "net_loop.h"
#include <debug.h>
#include <misc_utils.h>
#include <bsp_sys.h>

#define	LOOPBACK	0x7f000001

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

netadr_t	net_local_adr = {0};

qboolean	localconnectpending = false;
qsocket_t	*loop_client = NULL;
qsocket_t	*loop_server = NULL;

loopback_t	loopbacks[2];

#ifdef STM32
static inline ntohs (short a)
{
    uint8_t *b = (uint8_t *)&a;
    return b[0] << 8 | b[1];
}
#endif

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}


qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void	NET_Config (qboolean multiplayer)
{
}

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	d_memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	*net_from = net_local_adr;
	return true;
}

qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return NET_CompareAdr (adr, net_local_adr);
}

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
    int     i;
    loopback_t  *loop;

    loop = &loopbacks[sock^1];

    i = loop->send & (MAX_LOOPBACK-1);
    loop->send++;

    d_memcpy (loop->msgs[i].data, data, length);
    loop->msgs[i].datalen = length;
}

static qboolean NET_ParseIP (char *str, unsigned char ipv4[4], unsigned short *port)
{
    int i = 0;
    char buf[32] = {0}, *bufp, *pbufp;
    if (!isdigit((int)str[0])) {
        return false;
    }
    d_memcpy(buf, str, sizeof(buf));

    bufp = buf;
    pbufp = buf;
    while (bufp != bufp + sizeof(buf) && *bufp && i < 4) {
      if (*bufp == '.' || *bufp == ':') {
            if (bufp == pbufp) {
                dprintf("%s() : Garbage\n", __func__);
                return false;
            }
            *bufp = 0;
            ipv4[i++] = atoi(pbufp);
            pbufp = bufp + 1;
        }
        bufp++;
    }
    if (i != 4) {
        return false;
    }
    *port = atoi(pbufp);
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
    if (!strcmp (s, "localhost")) {
        memset (a, 0, sizeof(*a));
        a->type = NA_LOOPBACK;
    } else if (NET_ParseIP(s, a->ip, &a->port)) {
        a->type = NA_IP;
    } else {
        d_assert(0);
    }
    dprintf("%d() : %s\n", __func__, s);

    return true;
}


/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

    d_assert(0);
}

void NET_Sleep(int msec)
{

}



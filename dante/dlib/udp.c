/*
 * Copyright (c) 1997, 1998
 *      Inferno Nettverk A/S, Norway.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. The above copyright notice, this list of conditions and the following
 *    disclaimer must appear in all copies of the software, derivative works
 *    or modified versions, and any portions thereof, aswell as in all
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Inferno Nettverk A/S, Norway.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Inferno Nettverk A/S requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  sdc@inet.no
 *  Inferno Nettverk A/S
 *  Oslo Research Park
 *  Gaustadaléen 21
 *  N-0371 Oslo
 *  Norway
 * 
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

static const char rcsid[] =
"$Id: udp.c,v 1.77 1998/11/13 21:18:27 michaels Exp $";

#include "common.h"

/* ARGSUSED */
ssize_t
Rsendto(s, msg, len, flags, to, tolen)
	int s;
	const void *msg;
	size_t len;
	int flags;
	const struct sockaddr *to;
	int tolen;
{
	struct socksfd_t *socksfd;
	const struct sockaddr *newto;
	char *newmsg;
	size_t newlen;
	ssize_t n;


	if (to != NULL && to->sa_family != AF_INET)
		return sendto(s, msg, len, flags, to, tolen);

	if (udpsetup(s, to, SOCKS_SEND) != 0)
		return errno == 0 ? sendto(s, msg, len, flags, to, tolen) : -1;

	socksfd = socks_getaddr((unsigned int)s);
	SASSERTX(socksfd != NULL);

	if (to == NULL) {
		if (socksfd->state.udpconnect)
			newto	= &socksfd->connected;
		else
			/* have to assume tcp socket. */
			return sendto(s, msg, len, flags, NULL, 0);
	}
	else
		newto = to;

	/* prefix a udp header to the msg */
	newlen = len;
	if ((newmsg = udpheader_add(newto, msg, &newlen)) == NULL) {
		errno = ENOBUFS;
		return -1;
	}

	n = sendto(s, newmsg, newlen, flags, to, tolen);
	n -= newlen - len;

	free(newmsg);

	return MAX(-1, n);
}


ssize_t
Rrecvfrom(s, buf, len, flags, from, fromlen)
	int s;
#ifdef HAVE_RECVFROM_CHAR
	char *buf;
	int len;
#else
	void *buf;
	size_t len;
#endif  /* HAVE_RECVFROM_CHAR */
	int flags;
	struct sockaddr *from;
	socklen_t *fromlen;
{
	const char *function = "Rrecvfrom()";
	struct socksfd_t *socksfd;
	struct udpheader_t *header;
	char *newbuf;
	struct sockaddr newfrom;
	socklen_t newfromlen;
	size_t newlen;
	int n;


	if (!socks_addrisok((unsigned int)s)) {
		socks_rmaddr((unsigned int)s);
		return recvfrom(s, buf, len, flags, from, fromlen);
	}

	if (udpsetup(s, from, SOCKS_RECV) != 0)
		return errno == 0 ? recvfrom(s, buf, len, flags, from, fromlen) : -1;

	socksfd = socks_getaddr((unsigned int)s);
	SASSERTX(socksfd != NULL);
	
	if (!socksfd->state.protocol.udp)
		/* assume tcp connection. */
		return recvfrom(s, buf, len, flags, from, fromlen);

	/* if packet is from socksserver it will be prefixed with a header. */
	newlen = len 
			 + sizeof(header->flag)
			 + sizeof(header->frag)
			 + sizeof(header->host.atype)
			 + sizeof(header->host.addr)
			 + sizeof(header->host.port);
	if ((newbuf = (char *)malloc(sizeof(char) * newlen)) == NULL) {
		errno = ENOBUFS;
		return -1;
	}

	newfromlen = sizeof(newfrom);
	if ((n = recvfrom(s, newbuf, newlen, flags, &newfrom, &newfromlen)) <= 0) {
		free(newbuf);
		return n;
	}

	if (sockaddrcmp(&newfrom, &socksfd->reply) == 0) {
		/* packet is from socksserver. */
		
		if ((header = string2udpheader(newbuf, (size_t)n)) == NULL) {
			swarnx("%s: unrecognized udp packet from %s",
			function, sockaddr2string(&newfrom));
			free(newbuf);
			errno = EAGAIN;
			return -1;	/* don't know if callee wants to retry. */
		}
		newfrom = *sockshost2sockaddr(&header->host);

		/* callee doesn't get socksheader. */
		n -= PACKETSIZE_UDP(header);
		SASSERTX(n >= 0);
		SASSERTX(n <= len);
		memcpy(buf, &newbuf[PACKETSIZE_UDP(header)], (size_t)n);


		/* if connected udpsocket, only forward from "connected" source. */
		if (socksfd->state.udpconnect) {
			if (sockaddrcmp(&newfrom, &socksfd->connected) != 0) {
				char *a, *b;

				slog(LOG_DEBUG, "%s: expected udpreply from %s, got it from %s",
				function,
				strcheck(a = strdup(sockaddr2string(&socksfd->connected))),
				strcheck(b = strdup(sockaddr2string(&newfrom))));

				free(a);
				free(b);

				free(newbuf);
				errno = EAGAIN;
				return -1;	/* don't know if callee wants to retry. */
			}
		}
	}
	else
		memcpy(buf, newbuf, (size_t)n); /* not from socksserver. */

	free(newbuf);

	if (from != NULL && fromlen != NULL) {
		*fromlen = MIN(*fromlen, newfromlen);
		memcpy(from, &newfrom, (size_t)*fromlen);
	}

	return n;
}


int
udpsetup(s, to, type)
	int s;
	const struct sockaddr *to;
	int type;
{
	struct socks_t packet;
	struct socksfd_t socksfd;
	struct sockaddr_in newto;
	struct sockshost_t src, dst;
	int len, p;

	if (!socks_addrisok((unsigned int)s))
		socks_rmaddr((unsigned int)s);

	if (socks_getaddr((unsigned int)s) != NULL)
		return 0;

	/*
	 * if this socket has not previously been used we need to
	 * make a new connection to the socksserver for it.
	*/

	switch (type) {
		case SOCKS_RECV:
			/*
			 * problematic, trying to receive on socket not sent on.
			*/

			bzero(&newto, sizeof(newto));
			newto.sin_family 		 	= AF_INET;
			newto.sin_addr.s_addr 	= htonl(INADDR_ANY);
			newto.sin_port 			= htons(0);

			/* LINTED pointer casts may be troublesome */
			to = (struct sockaddr *)&newto;

			break;
		
		case SOCKS_SEND:
			if (to == NULL)
				return -1; /* no address and unknown socket, no idea. */
			break;

		default:
			SERRX(type);
	}

	/*
	 * we need to send the sockssever our address.
	 * First check if the socket already has a name, if so
	 * use that, otherwise assign the name ourselves.
	*/

	/* LINTED pointer casts may be troublesome */
	if (socks_getfakeip(((const struct sockaddr_in *)to)->sin_addr.s_addr) 
	!= NULL) {
		const char *ipname
		/* LINTED pointer casts may be troublesome */
		= socks_getfakeip(((const struct sockaddr_in *)to)->sin_addr.s_addr);

		SASSERTX(ipname != NULL);
		SASSERTX(strlen(ipname) < sizeof(dst.addr.domain));

		dst.atype = SOCKS_ADDR_DOMAIN;
		strcpy(dst.addr.domain, ipname);
	}
	else {
		dst.atype		= SOCKS_ADDR_IPV4;
		/* LINTED pointer casts may be troublesome */
		dst.addr.ipv4	= ((const struct sockaddr_in *)to)->sin_addr;
	}
	/* LINTED pointer casts may be troublesome */
	dst.port			= ((const struct sockaddr_in *)to)->sin_port;

	bzero(&socksfd, sizeof(socksfd));

	len = sizeof(socksfd.local);
	if (getsockname(s, &socksfd.local, &len) != 0)
		return -1;
	src = *sockaddr2sockshost(&socksfd.local);

	bzero(&packet, sizeof(packet));
	packet.version 			= SOCKS_V5;
	packet.req.version		= packet.version;
	packet.req.command		= SOCKS_UDPASSOCIATE;
	packet.req.flag 		  |= SOCKS_USECLIENTPORT;
/*	packet.req.flag 		  |= SOCKS_INTERFACEREQUEST; */
	packet.req.host 			= src;
	packet.auth					= &socksfd.state.auth;

	if ((socksfd.s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	if ((socksfd.route = socks_connectroute(socksfd.s, &packet, &src, &dst))
	== NULL) {
		close(socksfd.s);
		return -1;
	}

	/* LINTED  pointer casts may be troublesome */
	if ((((struct sockaddr_in *)(&socksfd.local))->sin_addr.s_addr
	== htonl(INADDR_ANY))
	|| ((struct sockaddr_in *)(&socksfd.local))->sin_port == htons(0)) {
		/* local name not set, set it. */

		/*
		 * don't have much of an idea on what ip address to use so might as 
		 * well use same as tcp connection to socksserver uses.
		*/
		len = sizeof(socksfd.local);
		if (getsockname(socksfd.s, &socksfd.local, &len) != 0) {
			close(socksfd.s);
			return -1;
		}	
		/* LINTED  pointer casts may be troublesome */
		((struct sockaddr_in *)&socksfd.local)->sin_port = htons(0);
		
		if (bind(s, &socksfd.local, sizeof(socksfd.local)) != 0) {
			close(socksfd.s);
			return -1;
		}

		if (getsockname(s, &socksfd.local, &len) != 0) {
			close(socksfd.s);
			return -1;
		}

		packet.req.host = *sockaddr2sockshost(&socksfd.local);
	}
	
	if (socks_negotiate(socksfd.s, &packet) != 0)
		return -1;

	socksfd.state.version 			= packet.version;
	socksfd.state.command 			= SOCKS_UDPASSOCIATE;
	socksfd.state.protocol.udp		= 1;
	socksfd.reply 						= *sockshost2sockaddr(&packet.res.host);

	p = sizeof(socksfd.server);
	if (getpeername(socksfd.s, &socksfd.server, &p) != 0) {
		close(socksfd.s);
		return -1;
	}

#if 0
	/*
	 * if the remote server supports interface requests, try to get
	 * the address it's using on our behalf.
	*/
	if (packet.res.flag & SOCKS_INTERFACEREQUEST) {
		struct interfacerequest_t ifreq;

		ifreq.rsv 				= 0;
		ifreq.sub 				= SOCKS_INTERFACEDATA;
		ifreq.flag 				= 0;
		ifreq.host.atype		= SOCKS_ADDR_IPV4;
		ifreq.host.addr.ipv4	= ((const struct sockaddr_in *)to)->sin_addr;
		ifreq.host.port		= ((const struct sockaddr_in *)to)->sin_port;

		if (send_interfacerequest(socksfd.s, &ifreq,
		socksfd.state.version) == 0) {
		}
	}
#else
	/* making a wild guess. */
	socksfd.remote = socksfd.reply;
#endif

	if (socks_addaddr((unsigned int)s, &socksfd) == NULL) {
		close(socksfd.s);
		errno = ENOBUFS;
		return -1;
	}

	return 0;
}

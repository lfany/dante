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
"$Id: clientprotocol.c,v 1.22 1998/11/13 21:18:06 michaels Exp $";

#include "common.h"

int
socks_sendrequest(s, request)
	int s;
	const struct request_t *request;
{
	const char *function = "socks_sendrequest()";
	char  requestmem[sizeof(*request)];
	char *p = requestmem;

	switch (request->version) {
		case SOCKS_V4:
			/* 
		 	 * VN   CD  DSTPORT DSTIP USERID   0
		 	 *  1 + 1  +   2   +  4  +  ?    + 1  = 9 + USERID
			*/

			/* VN */
			memcpy(p, &request->version, sizeof(request->version)); 
			p += sizeof(request->version);

			/* CD */
			memcpy(p, &request->command, sizeof(request->command));
			p += sizeof(request->command);

			p = sockshost2mem(&request->host, p, request->version);

			*p++ = 0; /* not bothering to send any userid, should we? */

			break; /* SOCKS_V4 */
			 
		 case SOCKS_V5:
			/*
			 * rfc1928 request:
		    *
			 *	+----+-----+-------+------+----------+----------+
			 *	|VER | CMD |  FLAG | ATYP | DST.ADDR | DST.PORT |
			 *	+----+-----+-------+------+----------+----------+
			 *	| 1  |  1  |   1   |  1   | Variable |    2     |
			 *	+----+-----+-------+------+----------+----------+
			 *	  1	   1     1      1       > 0         2
			 *
		  	 *	Which gives a fixed size of minimum 7 octets.
			 *	The first octet of DST.ADDR when it is SOCKS_ADDR_DOMAINNAME
			 *	contains the length of DST.ADDR.
			*/
				
			/* VER */
			memcpy(p, &request->version, sizeof(request->version));
			p += sizeof(request->version);

			/* CMD */
			memcpy(p, &request->command, sizeof(request->command));
			p += sizeof(request->command);

			/* FLAG */
			memcpy(p, &request->flag, sizeof(request->flag));
			p += sizeof(request->flag);

			p = sockshost2mem(&request->host, p, request->version);

			break;

		 default:
			SERRX(request->version);
	}

	slog(LOG_DEBUG, "%s: sending request: %s",
	function, socks_packet2string(request, SOCKS_REQUEST));

	/*
	 * Send the request to the server.
	*/
	if (writen(s, requestmem, (size_t)(p - requestmem))
	!= (size_t)(p - requestmem)) {
		swarn("%s: writen()", function);
		return -1;
	}

	return 0;
}

int
socks_recvresponse(s, response, version)
	int s;
	struct response_t	*response;
	int version;
{
	const char *function = "socks_recvresponse()";

	/* get the versionspecific data. */
	switch (version) {
		case SOCKS_V4: {
			/*
			 * The socks V4 reply length is fixed:
			 * VN   CD  DSTPORT  DSTIP
		 	 *  1 + 1  +   2   +   4
		 	*/
			char responsemem[sizeof(response->version)
								+ sizeof(response->reply)
								];
			char *p = responsemem;

			if (readn(s, responsemem, sizeof(responsemem)) != sizeof(responsemem)){
				swarn("%s: readn()", function);
				return -1;
			}
				
			/* VN */
			memcpy(&response->version, p, sizeof(response->version));
			p += sizeof(response->version);
			if (version != response->version) {
				swarnx("%s: unexpected version (%d != %d) from server",
				function, version, response->version);
				return -1;
			}

			/* CD */
			memcpy(&response->reply, p, sizeof(response->reply));
			p += sizeof(response->reply);
			break;
		}

		case SOCKS_V5: {
			/*
			 * rfc1928 reply:
			 *
		  	 * +----+-----+-------+------+----------+----------+
			 * |VER | REP |  FLAG | ATYP | BND.ADDR | BND.PORT |
			 * +----+-----+-------+------+----------+----------+
			 * | 1  |  1  |   1   |  1   |  > 0     |    2     |
			 * +----+-----+-------+------+----------+----------+
			 *
			 *	Which gives a size of >= 7 octets.
 			 *
			*/
			char responsemem[sizeof(response->version)
								+ sizeof(response->reply)
								+ sizeof(response->flag)
								];
			char *p = responsemem;

			if (readn(s, responsemem, sizeof(responsemem)) != sizeof(responsemem)){
				swarn("%s: readn()", function);
				return -1;
			}
				
			/* VER */
			memcpy(&response->version, p, sizeof(response->version));
			p += sizeof(response->version);
			if (version != response->version) {
				swarnx("%s: unexpected version (%d != %d) from server",
				function, version, response->version);
				return -1;
			}

			/* REP */
			memcpy(&response->reply, p, sizeof(response->reply));
			p += sizeof(response->reply);

			/* FLAG */
			memcpy(&response->flag, p, sizeof(response->flag));
			p += sizeof(response->flag);

			break;
		}

		default:
		 	SERRX(version);
	}

	if (recv_sockshost(s, &response->host, version) != 0)
		return -1;
	
	slog(LOG_DEBUG, "%s: received response: %s",
	function, socks_packet2string(response, SOCKS_RESPONSE));

	return 0;
}


int
send_interfacerequest(s, ifreq, version)
	int s;
	const struct interfacerequest_t *ifreq;
	int version;
{
	char request[sizeof(*ifreq)];
	char *p = request;

	memcpy(p, &ifreq->rsv, sizeof(ifreq->rsv));
	p += sizeof(ifreq->rsv);

	memcpy(p, &ifreq->sub, sizeof(ifreq->sub));
	p += sizeof(ifreq->sub);

	memcpy(p, &ifreq->flag, sizeof(ifreq->flag));
	p += sizeof(ifreq->flag);
	
	p = sockshost2mem(&ifreq->host, p, version);

	if (writen(s, request, (size_t)(p - request)) != (size_t)(p - request))
		return -1;
	return 0;
}

int
socks_negotiate(s, packet)
	int s;
	struct socks_t	*packet;
{
	
	switch (packet->req.version) {
		case SOCKS_V4:
			break;	/* no pre-request negotiation in v4. */

		case SOCKS_V5:
			if (negotiate_method(s, packet) != 0)
				return -1;
			break;

		default:
			SERRX(packet->req.version);
	}

	if (socks_sendrequest(s, &packet->req) != 0)
		return -1;

	if (socks_recvresponse(s, &packet->res, packet->req.version) != 0)
		return -1;

	if (!serverreplyisok(packet->res.version, packet->res.reply))
		return -1;

	SASSERTX(packet->req.version == packet->res.version);

	return 0;
}


int
recv_sockshost(s, host, version)
	int s;
	struct sockshost_t *host;
	int version;
{
	const char *function = "recv_sockshost()";

	switch (version) {
		case SOCKS_V4: {
			/*
			 * DSTPORT  DSTIP
		 	 *   2    +   4
		 	*/
			char hostmem[sizeof(host->port)
						  + sizeof(host->addr.ipv4)
							];
			char *p = hostmem;

			if (readn(s, hostmem, sizeof(hostmem)) != sizeof(hostmem)){
				swarn("%s: readn()", function);
				return -1;
			}

			host->atype = SOCKS_ADDR_IPV4;
				
			/* BND.PORT */
			memcpy(&host->port, p, sizeof(host->port));
			p += sizeof(host->port);

			/* BND.ADDR */
			memcpy(&host->addr.ipv4, p, sizeof(host->addr.ipv4));
			p += sizeof(host->addr.ipv4);

			break;
		}

		case SOCKS_V5:
			/*
		  	 * +------+----------+----------+
			 * | ATYP | BND.ADDR | BND.PORT |
			 * +------+----------+----------+
			 * |  1   |  > 0     |    2     |
			 * +------+----------+----------+
			*/

		 	/* ATYP */
			if (readn(s, &host->atype, sizeof(host->atype)) != sizeof(host->atype))
				return -1;

			switch(host->atype) {
				case SOCKS_ADDR_IPV4:
					if (readn(s, &host->addr.ipv4, sizeof(host->addr.ipv4))
					!= sizeof(host->addr.ipv4)) {
						swarn("%s: readn()", function);
						return -1;
					}
					break;

				case SOCKS_ADDR_IPV6:
					if (readn(s, host->addr.ipv6, sizeof(host->addr.ipv6))
					!= sizeof(host->addr.ipv6)) {
						swarn("%s: readn()", function);
						return -1;
					}
					break;

				case SOCKS_ADDR_DOMAIN: {
					unsigned char alen;

					/* read length of domainname. */
					if (readn(s, &alen, sizeof(alen)) < sizeof(alen))
						return -1;
					
					OCTETIFY(alen);

					/* BND.ADDR, alen octets */
					if (readn(s, host->addr.domain, (size_t)alen) != (size_t)alen) {
						swarn("%s: readn()", function);
						return -1;
					}

					SASSERTX(alen < sizeof(host->addr.domain));
					host->addr.domain[alen] = NUL;

					break;
				}

				default:
					swarnx("%s: unsupported address format %d in reply", 
					function, host->atype);
					return -1;
			}

			/* BND.PORT */
			if (readn(s, &host->port, sizeof(host->port)) != sizeof(host->port)) 
				return -1;
			break;
	}

	return 0;
}

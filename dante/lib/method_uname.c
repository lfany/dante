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
"$Id: method_uname.c,v 1.18 1998/11/13 21:18:19 michaels Exp $";

#include "common.h"

int
clientmethod_uname(s, version)
	int s;
	int version;
{
	const char *function = "clientmethod_uname()";
	char *offset;
	char *name, *password;

	char request[
		  sizeof(char)		/* version.				*/
		+ sizeof(char)		/* username length.	*/
		+ MAXNAMELEN		/* username.			*/
		+ sizeof(char)		/* password length.	*/
		+ MAXPWLEN			/* password.			*/
	];
	
	unsigned char response[
		  sizeof(char)		/* version. */
		+ sizeof(char)		/* status.	*/
	];

	switch (version) {
		case SOCKS_V5:
			break;

		default:
			SERRX(version);
	}

	/* fill in request. */

	offset = request;

	*offset++ = (char)version;

	if ((name = socks_getusername(offset + 1, MAXNAMELEN)) == NULL) {
		swarnx("%s: socks_getusername() failed", function);
		return -1;
	}
	/* first byte gives length. */
	*offset = (char)strlen(name);
	OCTETIFY(*offset);
	offset += *offset + 1;
	
	if ((password = socks_getpassword(name, offset + 1, MAXPWLEN)) == NULL) {
		swarnx("could not determine password of client");
		return -1;
	}
	/* first byte gives length. */
	*offset = (char)strlen(password);
	OCTETIFY(*offset);
	offset += *offset + 1;
	
	if (writen(s, request, (size_t)(offset - request)) != offset - request) {
		bzero(password, MAXPWLEN);
		swarn("%s: writen()", function);	
		return -1;
	}
	bzero(password, MAXPWLEN);

	if (readn(s, response, sizeof(response)) != sizeof(response)) {
		swarn("%s: readn()", function);	
		return -1;
	}

	if (request[UNAME_VERSION] != response[UNAME_VERSION]) {
		swarnx("%s: sent v%d, got v%d",
		function, (int)request[UNAME_VERSION], response[UNAME_VERSION]);
		return -1;
	}
	
	return response[UNAME_STATUS];
}

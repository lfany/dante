/*
 * Copyright (c) 1997, 1998, 1999
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
 *  N-0349 Oslo
 *  Norway
 *
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

#define WE_DONT_WANT_NO_SOCKADDR_ARG_UNION

#include "common.h"

#if SOCKSLIBRARY_DYNAMIC

#include "interposition.h"

static const char rcsid[] =
"$Id: interposition.c,v 1.53 1999/09/25 13:10:31 karls Exp $";

#undef accept
#undef bind
#undef bindresvport
#undef connect
#undef gethostbyname
#undef gethostbyname2
#undef getpeername
#undef getsockname
#undef read
#undef readv
#undef recv
#undef recvfrom
#undef recvmsg
#undef rresvport
#undef send
#undef sendmsg
#undef sendto
#undef write
#undef writev

#if NEED_DYNA_RTLD
#define DL_LAZY RTLD_LAZY
#endif  /* NEED_DYNA_RTLD */

static struct libsymbol_t libsymbolv[] = {
	{	SYMBOL_ACCEPT,						LIBRARY_ACCEPT },
	{	SYMBOL_BIND,						LIBRARY_BIND },
	{	SYMBOL_BINDRESVPORT,				LIBRARY_BINDRESVPORT },
	{	SYMBOL_CONNECT,					LIBRARY_CONNECT },
	{	SYMBOL_GETHOSTBYNAME,			LIBRARY_GETHOSTBYNAME },
	{	SYMBOL_GETHOSTBYNAME2,			LIBRARY_GETHOSTBYNAME2 },
	{	SYMBOL_GETPEERNAME,				LIBRARY_GETPEERNAME },
	{	SYMBOL_GETSOCKNAME,				LIBRARY_GETSOCKNAME },
	{	SYMBOL_READ,						LIBRARY_READ },
	{	SYMBOL_READV,						LIBRARY_READV },
	{	SYMBOL_RECV,						LIBRARY_RECV },
	{	SYMBOL_RECVMSG,					LIBRARY_RECVMSG },
	{	SYMBOL_RECVFROM,					LIBRARY_RECVFROM },
	{	SYMBOL_RRESVPORT,					LIBRARY_RRESVPORT },
	{	SYMBOL_SEND,						LIBRARY_SEND },
	{	SYMBOL_SENDMSG,					LIBRARY_SENDMSG },
	{	SYMBOL_SENDTO,						LIBRARY_SENDTO },
	{	SYMBOL_WRITE,						LIBRARY_WRITE },
	{	SYMBOL_WRITEV,						LIBRARY_WRITEV },
#if HAVE_EXTRA_OSF_SYMBOLS
	{	SYMBOL_EACCEPT,					LIBRARY_EACCEPT },
	{	SYMBOL_EGETPEERNAME,				LIBRARY_EGETPEERNAME },
	{  SYMBOL_EGETSOCKNAME,				LIBRARY_EGETSOCKNAME },
	{	SYMBOL_EREADV,						LIBRARY_EREADV },
	{	SYMBOL_ERECVFROM,					LIBRARY_ERECVFROM },
	{	SYMBOL_ERECVMSG,					LIBRARY_ERECVMSG },
	{  SYMBOL_ESENDMSG,					LIBRARY_ESENDMSG },
	{	SYMBOL_EWRITEV,					LIBRARY_EWRITEV },

	{	SYMBOL_NACCEPT,					LIBRARY_EACCEPT },
	{	SYMBOL_NGETPEERNAME,				LIBRARY_NGETPEERNAME },
	{	SYMBOL_NGETSOCKNAME,				LIBRARY_NGETSOCKNAME },
	{  SYMBOL_NRECVFROM,					LIBRARY_NRECVFROM },
	{  SYMBOL_NRECVMSG,					LIBRARY_NRECVMSG },
	{	SYMBOL_NSENDMSG,					LIBRARY_NSENDMSG },
#endif  /* HAVE_EXTRA_OSF_SYMBOLS */


};

__BEGIN_DECLS

static struct libsymbol_t *
libsymbol __P((const char *symbol));
/*
 * Finds the libsymbol_t that "symbol" is defined in.
 */

__END_DECLS

static struct libsymbol_t *
libsymbol(symbol)
	const char *symbol;
{
	const char *function = "libsymbol()";
	size_t i;

	for (i = 0; i < ELEMENTS(libsymbolv); ++i)
		if (strcmp(libsymbolv[i].symbol, symbol) == 0)
			return &libsymbolv[i];

	serrx(1, "%s: configuration error, can't find symbol %s", function, symbol);
	return NULL; /* please compiler. */
}


void *
symbolfunction(symbol)
	char *symbol;
{
	const char *function = "symbolfunction()";
	struct libsymbol_t *lib;

	lib = libsymbol(symbol);

	SASSERTX(lib != NULL);
	SASSERTX(lib->library != NULL);
	SASSERTX(strcmp(lib->symbol, symbol) == 0);

	if (lib->handle == NULL)
		if ((lib->handle = dlopen(lib->library, DL_LAZY)) == NULL)
			serrx(EXIT_FAILURE, "%s: %s: %s", function, lib->library, dlerror());

	if (lib->function == NULL)
		if ((lib->function = dlsym(lib->handle, symbol)) == NULL)
			serrx(EXIT_FAILURE, "%s: %s: %s", function, symbol, dlerror());

#if 0
	if (strcmp(symbol, SYMBOL_WRITE) != 0)
		slog(LOG_DEBUG, "found symbol %s in library %s",
		lib->symbol, lib->library);
#endif

	return lib->function;
}


	/* the real system calls. */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_ACCEPT_0
sys_accept(s, addr, addrlen)
	HAVE_PROT_ACCEPT_1 s;
	HAVE_PROT_ACCEPT_2 addr;
	HAVE_PROT_ACCEPT_3 addrlen;
{
	int rc;
	HAVE_PROT_ACCEPT_0 (*function)(HAVE_PROT_ACCEPT_1 s, HAVE_PROT_ACCEPT_2 addr, HAVE_PROT_ACCEPT_3 addrlen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_ACCEPT);
	rc = function(s, addr, addrlen);
	SYSCALL_END(s);
	return rc;
}
#endif  /* !HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_BIND_0
sys_bind(s, name, namelen)
	HAVE_PROT_BIND_1 s;
	HAVE_PROT_BIND_2 name;
	HAVE_PROT_BIND_3 namelen;
{
	int rc;
	HAVE_PROT_BIND_0 (*function)(HAVE_PROT_BIND_1 s, HAVE_PROT_BIND_2 name, HAVE_PROT_BIND_3 namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_BIND);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

int
sys_bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	int rc;
	int (*function)(int sd, struct sockaddr_in *sin);

	SYSCALL_START(sd);
	function = symbolfunction(SYMBOL_BINDRESVPORT);
	rc = function(sd, sin);
	SYSCALL_END(sd);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_CONNECT_0
sys_connect(s, name, namelen)
	HAVE_PROT_CONNECT_1 s;
	HAVE_PROT_CONNECT_2 name;
	HAVE_PROT_CONNECT_3 namelen;
{
	int rc;
	HAVE_PROT_CONNECT_0 (*function)(HAVE_PROT_CONNECT_1 s, HAVE_PROT_CONNECT_2 name, HAVE_PROT_CONNECT_3 namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_CONNECT);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

struct hostent *
sys_gethostbyname(name)
	const char *name;
{
	struct hostent *(*function)(const char *name);

	function = symbolfunction(SYMBOL_GETHOSTBYNAME);
	return function(name);
}

struct hostent *
sys_gethostbyname2(name, af)
	const char *name;
	int af;
{
	struct hostent *(*function)(const char *name, int af);

	function = symbolfunction(SYMBOL_GETHOSTBYNAME2);
	return function(name, af);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_GETPEERNAME_0
sys_getpeername(s, name, namelen)
	HAVE_PROT_GETPEERNAME_1 s;
	HAVE_PROT_GETPEERNAME_2 name;
	HAVE_PROT_GETPEERNAME_3 namelen;
{
	int rc;
	HAVE_PROT_GETPEERNAME_0 (*function)(HAVE_PROT_GETPEERNAME_1 s, HAVE_PROT_GETPEERNAME_2 name, HAVE_PROT_GETPEERNAME_3 namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_GETPEERNAME);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* ! HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_GETSOCKNAME_0
sys_getsockname(s, name, namelen)
	HAVE_PROT_GETSOCKNAME_1 s;
	HAVE_PROT_GETSOCKNAME_2 name;
	HAVE_PROT_GETSOCKNAME_3 namelen;
{
	int rc;
	HAVE_PROT_GETSOCKNAME_0 (*function)(HAVE_PROT_GETSOCKNAME_1 s, HAVE_PROT_GETSOCKNAME_2 name, HAVE_PROT_GETSOCKNAME_3 namelen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_GETSOCKNAME);
	rc = function(s, name, namelen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_READ_0
sys_read(d, buf, nbytes)
	HAVE_PROT_READ_1 d;
	HAVE_PROT_READ_2 buf;
	HAVE_PROT_READ_3 nbytes;
{
	ssize_t rc;
	HAVE_PROT_READ_0 (*function)(HAVE_PROT_READ_1 d, HAVE_PROT_READ_2 buf, HAVE_PROT_READ_3 nbytes);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_READ);
	rc = function(d, buf, nbytes);
	SYSCALL_END(d);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_READV_0
sys_readv(d, iov, iovcnt)
	HAVE_PROT_READV_1 d;
	HAVE_PROT_READV_2 iov;
	HAVE_PROT_READV_3 iovcnt;
{
	ssize_t rc;
	HAVE_PROT_READV_0 (*function)(HAVE_PROT_READV_1 d, HAVE_PROT_READV_2 iov, HAVE_PROT_READV_3 iovcnt);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_READV);
	rc = function(d, iov, iovcnt);
	SYSCALL_END(d);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_RECV_0
sys_recv(s, buf, len, flags)
	HAVE_PROT_RECV_1 s;
	HAVE_PROT_RECV_2 buf;
	HAVE_PROT_RECV_3 len;
	HAVE_PROT_RECV_4 flags;
{
	ssize_t rc;
	HAVE_PROT_RECV_0 (*function)(HAVE_PROT_RECV_1 s, HAVE_PROT_RECV_2 buf, HAVE_PROT_RECV_3 len, HAVE_PROT_RECV_4 flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECV);
	rc = function(s, buf, len, flags);
	SYSCALL_END(s);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_RECVFROM_0
sys_recvfrom(s, buf, len, flags, from, fromlen)
	HAVE_PROT_RECVFROM_1 s;
	HAVE_PROT_RECVFROM_2 buf;
	HAVE_PROT_RECVFROM_3 len;
	HAVE_PROT_RECVFROM_4 flags;
	HAVE_PROT_RECVFROM_5 from;
	HAVE_PROT_RECVFROM_6 fromlen;
{
	int rc;
	HAVE_PROT_RECVFROM_0 (*function)(HAVE_PROT_RECVFROM_1 s, HAVE_PROT_RECVFROM_2 buf, HAVE_PROT_RECVFROM_3 len, HAVE_PROT_RECVFROM_4 flags, HAVE_PROT_RECVFROM_5 from, HAVE_PROT_RECVFROM_6 fromlen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECVFROM);
	rc = function(s, buf, len, flags, from, fromlen);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_RECVMSG_0
sys_recvmsg(s, msg, flags)
	HAVE_PROT_RECVMSG_1 s;
	HAVE_PROT_RECVMSG_2 msg;
	HAVE_PROT_RECVMSG_3 flags;
{
	ssize_t rc;
	HAVE_PROT_RECVMSG_0 (*function)(HAVE_PROT_RECVMSG_1 s, HAVE_PROT_RECVMSG_2 msg, HAVE_PROT_RECVMSG_3 flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_RECVMSG);
	rc = function(s, msg, flags);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

int
sys_rresvport(port)
	int *port;
{
	int (*function)(int *port);

	function = symbolfunction(SYMBOL_RRESVPORT);
	return function(port);
}

HAVE_PROT_SEND_0
sys_send(s, msg, len, flags)
	HAVE_PROT_SEND_1 s;
	HAVE_PROT_SEND_2 msg;
	HAVE_PROT_SEND_3 len;
	HAVE_PROT_SEND_4 flags;
{
	ssize_t rc;
	HAVE_PROT_SEND_0 (*function)(HAVE_PROT_SEND_1 s, HAVE_PROT_SEND_2 msg, HAVE_PROT_SEND_3 len, HAVE_PROT_SEND_4 flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SEND);
	rc = function(s, msg, len, flags);
	SYSCALL_END(s);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_SENDMSG_0
sys_sendmsg(s, msg, flags)
	HAVE_PROT_SENDMSG_1 s;
	HAVE_PROT_SENDMSG_2 msg;
	HAVE_PROT_SENDMSG_3 flags;
{
	ssize_t rc;
	HAVE_PROT_SENDMSG_0 (*function)(HAVE_PROT_SENDMSG_1 s, HAVE_PROT_SENDMSG_2 msg, HAVE_PROT_SENDMSG_3  flags);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SENDMSG);
	rc = function(s, msg, flags);
	SYSCALL_END(s);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_SENDTO_0
sys_sendto(s, msg, len, flags, to, tolen)
	HAVE_PROT_SENDTO_1 s;
	HAVE_PROT_SENDTO_2 msg;
	HAVE_PROT_SENDTO_3 len;
	HAVE_PROT_SENDTO_4 flags;
	HAVE_PROT_SENDTO_5 to;
	HAVE_PROT_SENDTO_6 tolen;
{
	ssize_t rc;
	HAVE_PROT_SENDTO_0 (*function)(HAVE_PROT_SENDTO_1 s, HAVE_PROT_SENDTO_2 msg, HAVE_PROT_SENDTO_3 len, HAVE_PROT_SENDTO_4 flags, HAVE_PROT_SENDTO_5 to, HAVE_PROT_SENDTO_6 tolen);

	SYSCALL_START(s);
	function = symbolfunction(SYMBOL_SENDTO);
	rc = function(s, msg, len, flags, to, tolen);
	SYSCALL_END(s);
	return rc;
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_WRITE_0
sys_write(d, buf, nbytes)
	HAVE_PROT_WRITE_1 d;
	HAVE_PROT_WRITE_2 buf;
	HAVE_PROT_WRITE_3 nbytes;
{
	ssize_t rc;
	HAVE_PROT_WRITE_0 (*function)(HAVE_PROT_WRITE_1 d, HAVE_PROT_WRITE_2 buf, HAVE_PROT_WRITE_3 nbutes);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_WRITE);
	rc = function(d, buf, nbytes);
	SYSCALL_END(d);
	return rc;
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_WRITEV_0
sys_writev(d, iov, iovcnt)
	HAVE_PROT_WRITEV_1 d;
	HAVE_PROT_WRITEV_2 iov;
	HAVE_PROT_WRITEV_3 iovcnt;
{
	ssize_t rc;
	HAVE_PROT_WRITEV_0 (*function)(HAVE_PROT_WRITEV_1 d, HAVE_PROT_WRITEV_2 buf, HAVE_PROT_WRITEV_3 iovcnt);

	SYSCALL_START(d);
	function = symbolfunction(SYMBOL_WRITEV);
	rc = function(d, iov, iovcnt);
	SYSCALL_END(d);
	return rc;
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */


	/*
	 * the interpositioned functions.
	 */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_ACCEPT_0
accept(s, addr, addrlen)
	HAVE_PROT_ACCEPT_1 s;
	HAVE_PROT_ACCEPT_2 addr;
	HAVE_PROT_ACCEPT_3 addrlen;
{
	if (ISSYSCALL(s))
		return sys_accept(s, addr, addrlen);
	return Raccept(s, addr, addrlen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_BIND_0
bind(s, name, namelen)
	HAVE_PROT_BIND_1 s;
	HAVE_PROT_BIND_2 name;
	HAVE_PROT_BIND_3 namelen;
{
	if (ISSYSCALL(s))
		return sys_bind(s, name, namelen);
	return Rbind(s, name, namelen);
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

int
bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	if (ISSYSCALL(sd))
		return sys_bindresvport(sd, sin);
	return Rbindresvport(sd, sin);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_CONNECT_0
connect(s, name, namelen)
	HAVE_PROT_CONNECT_1 s;
	HAVE_PROT_CONNECT_2 name;
	HAVE_PROT_CONNECT_3 namelen;
{
	if (ISSYSCALL(s))
		return sys_connect(s, name, namelen);
	return Rconnect(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

struct hostent *
gethostbyname(name)
	const char *name;
{
	return Rgethostbyname(name);
}

struct hostent *
gethostbyname2(name, af)
	const char *name;
	int af;
{
	return Rgethostbyname2(name, af);
}


#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_GETPEERNAME_0
getpeername(s, name, namelen)
	HAVE_PROT_GETPEERNAME_1 s;
	HAVE_PROT_GETPEERNAME_2 name;
	HAVE_PROT_GETPEERNAME_3 namelen;
{
	if (ISSYSCALL(s))
		return sys_getpeername(s, name, namelen);
	return Rgetpeername(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_GETSOCKNAME_0
getsockname(s, name, namelen)
	HAVE_PROT_GETSOCKNAME_1 s;
	HAVE_PROT_GETSOCKNAME_2 name;
	HAVE_PROT_GETSOCKNAME_3 namelen;
{
	if (ISSYSCALL(s))
		return sys_getpeername(s, name, namelen);
	return Rgetsockname(s, name, namelen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_READ_0
read(d, buf, nbytes)
	HAVE_PROT_READ_1 d;
	HAVE_PROT_READ_2 buf;
	HAVE_PROT_READ_3 nbytes;
{
	if (ISSYSCALL(d))
		return sys_read(d, buf, nbytes);
	return Rread(d, buf, nbytes);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_READV_0
readv(d, iov, iovcnt)
	HAVE_PROT_READV_1 d;
	HAVE_PROT_READV_2 iov;
	HAVE_PROT_READV_3 iovcnt;
{
	if (ISSYSCALL(d))
		return sys_readv(d, iov, iovcnt);
	return Rreadv(d, iov, iovcnt);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_RECV_0
recv(s, msg, len, flags)
	HAVE_PROT_RECV_1 s;
	HAVE_PROT_RECV_2 msg;
	HAVE_PROT_RECV_3 len;
	HAVE_PROT_RECV_4 flags;
{
	if (ISSYSCALL(s))
		return sys_recv(s, msg, len, flags);
	return Rrecv(s, msg, len, flags);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_RECVFROM_0
recvfrom(s, buf, len, flags, from, fromlen)
	HAVE_PROT_RECVFROM_1 s;
	HAVE_PROT_RECVFROM_2 buf;
	HAVE_PROT_RECVFROM_3 len;
	HAVE_PROT_RECVFROM_4 flags;
	HAVE_PROT_RECVFROM_5 from;
	HAVE_PROT_RECVFROM_6 fromlen;
{
	if (ISSYSCALL(s))
		return sys_recvfrom(s, buf, len, flags, from, fromlen);
	return Rrecvfrom(s, buf, len, flags, from, fromlen);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_RECVMSG_0
recvmsg(s, msg, flags)
	HAVE_PROT_RECVMSG_1 s;
	HAVE_PROT_RECVMSG_2 msg;
	HAVE_PROT_RECVMSG_3 flags;
{
	if (ISSYSCALL(s))
		return sys_recvmsg(s, msg, flags);
	return Rrecvmsg(s, msg, flags);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

int
rresvport(port)
	int *port;
{
	return Rrresvport(port);
}

HAVE_PROT_WRITE_0
write(d, buf, nbytes)
	HAVE_PROT_WRITE_1 d;
	HAVE_PROT_WRITE_2 buf;
	HAVE_PROT_WRITE_3 nbytes;
{
	if (ISSYSCALL(d))
		return sys_write(d, buf, nbytes);
	return Rwrite(d, buf, nbytes);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_WRITEV_0
writev(d, iov, iovcnt)
	HAVE_PROT_WRITEV_1 d;
	HAVE_PROT_WRITEV_2 iov;
	HAVE_PROT_WRITEV_3 iovcnt;
{
	if (ISSYSCALL(d))
		return sys_writev(d, iov, iovcnt);
	return Rwritev(d, iov, iovcnt);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

HAVE_PROT_SEND_0
send(s, msg, len, flags)
	HAVE_PROT_SEND_1 s;
	HAVE_PROT_SEND_2 msg;
	HAVE_PROT_SEND_3 len;
	HAVE_PROT_SEND_4 flags;
{
	if (ISSYSCALL(s))
		return sys_send(s, msg, len, flags);
	return Rsend(s, msg, len, flags);
}

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_SENDMSG_0
sendmsg(s, msg, flags)
	HAVE_PROT_SENDMSG_1 s;
	HAVE_PROT_SENDMSG_2 msg;
	HAVE_PROT_SENDMSG_3 flags;
{
	if (ISSYSCALL(s))
		return sys_sendmsg(s, msg, flags);
	return Rsendmsg(s, msg, flags);
}
#endif /* HAVE_EXTRA_OSF_SYMBOLS */

#if !HAVE_EXTRA_OSF_SYMBOLS
HAVE_PROT_SENDTO_0
sendto(s, msg, len, flags, to, tolen)
	HAVE_PROT_SENDTO_1 s;
	HAVE_PROT_SENDTO_2 msg;
	HAVE_PROT_SENDTO_3 len;
	HAVE_PROT_SENDTO_4 flags;
	HAVE_PROT_SENDTO_5 to;
	HAVE_PROT_SENDTO_6 tolen;
{
	if (ISSYSCALL(s))
		return sys_sendto(s, msg, len, flags, to, tolen);
	return Rsendto(s, msg, len, flags, to, tolen);
}
#endif /* !HAVE_EXTRA_OSF_SYMBOLS */

#endif /* SOCKSLIBRARY_DYNAMIC */

/*****************************************************************************

 @(#) $RCSfile$ $Name$($Revision$) $Date$

 -----------------------------------------------------------------------------

 Copyright (c) 2001-2007  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2000  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, version 3 of the license.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 -----------------------------------------------------------------------------

 Last Modified $Date$ by $Author$

 -----------------------------------------------------------------------------

 $Log$
 *****************************************************************************/

#ident "@(#) $RCSfile$ $Name$($Revision$) $Date$"

static char const ident[] = "$RCSfile$ $Name$($Revision$) $Date$";

/* ode2oid.c - object descriptor to object identifier */

#ifndef	lint
static char *rcsid =
    "Header: /xtel/isode/isode/psap/RCS/ode2oid.c,v 9.0 1992/06/16 12:25:44 isode Rel";
#endif

/* 
 * Header: /xtel/isode/isode/psap/RCS/ode2oid.c,v 9.0 1992/06/16 12:25:44 isode Rel
 *
 *
 * Log: ode2oid.c,v
 * Revision 9.0  1992/06/16  12:25:44  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

/* LINTLIBRARY */

#include <stdio.h>
#include "psap.h"
#include "ppkt.h"

/* work around define collisions */
#undef missingP
#undef pylose
#include "rtpkt.h"

/* work around type clashes */
#undef missingP
#undef pylose
#undef toomuchP
#define ACSE
#define assocblk assocblkxxx
#define newacblk newacblkxxx
#define findacblk findacblkxxx
#include "acpkt.h"

/*  */
#define ODECACHESIZE 10
static struct la_cache {
	char *descriptor;
	int ref;
	OID oid;
} Cache[ODECACHESIZE];

static void
preloadcache(str)
	char *str;
{
	struct la_cache *cp = &Cache[0];
	register struct isobject *io;

	(void) setisobject(0);
	while (io = getisobject()) {
		if (strcmp(str, io->io_descriptor) == 0 ||
		    strcmp(DFLT_ASN, io->io_descriptor) == 0 ||
		    strcmp(AC_ASN, io->io_descriptor) == 0 ||
		    strcmp(BER, io->io_descriptor) == 0 || strcmp(RT_ASN, io->io_descriptor) == 0) {
			if ((cp->oid = oid_cpy(&io->io_identity)) == NULLOID ||
			    (cp->descriptor = malloc((unsigned) (strlen(io->io_descriptor) + 1)))
			    == NULLCP) {
				if (cp->oid) {
					oid_free(cp->oid);
					cp->oid = NULLOID;
				}
			} else {
				(void) strcpy(cp->descriptor, io->io_descriptor);
				cp->ref = 1;
				cp++;
			}
		}
	}
	(void) endisobject();
}

OID
ode2oid(descriptor)
	char *descriptor;
{
	register struct isobject *io;
	int i, least;
	struct la_cache *cp, *cpn;
	static char firsttime = 0;

	if (firsttime == 0) {
		preloadcache(descriptor);
		firsttime = 1;
	}

	least = Cache[0].ref;
	for (cpn = cp = &Cache[0], i = 0; i < ODECACHESIZE; i++, cp++) {
		if (cp->ref < least) {
			least = cp->ref;
			cpn = cp;
		}
		if (cp->ref <= 0)
			continue;
		if (strcmp(descriptor, cp->descriptor) == 0) {
			cp->ref++;
			return cp->oid;
		}
	}

	if ((io = getisobjectbyname(descriptor)) == NULL)
		return NULLOID;

	if (cpn->oid)
		oid_free(cpn->oid);
	if (cpn->descriptor)
		free(cpn->descriptor);

	cpn->ref = 1;
	if ((cpn->oid = oid_cpy(&io->io_identity)) == NULLOID ||
	    (cpn->descriptor = malloc((unsigned) (strlen(descriptor) + 1))) == NULLCP) {
		if (cpn->oid) {
			oid_free(cpn->oid);
			cpn->oid = NULLOID;
		}
		cpn->ref = 0;
	} else
		(void) strcpy(cpn->descriptor, descriptor);

	return (&io->io_identity);
}

#ifdef DEBUG
free_oid_cache()
{
	struct la_cache *cp;
	int i;

	for (cp = &Cache[0], i = 0; i < ODECACHESIZE; i++, cp++) {
		if (cp->descriptor)
			free(cp->descriptor);

		if (cp->oid)
			oid_free(cp->oid);
	}
}
#endif
/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* LGPL Copyright 1998 Raph Levien <raph@acm.org> */

#include "art_misc.h"
#include "art_vpath.h"
#include "art_uta.h"
#include "art_uta_vpath.h"
#include "art_svp.h"
#include "art_uta_svp.h"
#include "art_vpath_svp.h"


/* I will of course want to replace this with a more direct implementation.
   But this gets the api in place. */
ArtUta *
art_uta_from_svp (const ArtSVP *svp)
{
  ArtVpath *vpath;
  ArtUta *uta;

  vpath = art_vpath_from_svp (svp);
  uta = art_uta_from_vpath (vpath);
  art_free (vpath);
  return uta;
}

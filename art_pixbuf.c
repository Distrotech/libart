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

#include "art_misc.h"
#include "art_pixbuf.h"

/* A generic data structure for holding a buffer of pixels. One way
   to think about this module is as a virtualization over specific
   pixel buffer formats. */

ArtPixBuf *
art_pixbuf_new_rgb (art_u8 *pixels, int width, int height, int rowstride)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 3;
  pixbuf->has_alpha = 0;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;

  return pixbuf;
}

ArtPixBuf *
art_pixbuf_new_rgba (art_u8 *pixels, int width, int height, int rowstride)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 4;
  pixbuf->has_alpha = 1;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;

  return pixbuf;
}

/* Warning: if you call this function, make sure the pixels were
   allocated with art_alloc. */
void
art_pixbuf_free (ArtPixBuf *pixbuf)
{
  art_free (pixbuf->pixels);
  art_free (pixbuf);
}

void
art_pixbuf_free_shallow (ArtPixBuf *pixbuf)
{
  art_free (pixbuf);
}

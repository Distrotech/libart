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
art_pixbuf_new_rgb_dnotify (art_u8 *pixels, int width, int height, int rowstride,
			    void *dfunc_data, ArtDestroyNotify dfunc)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 3;
  pixbuf->has_alpha = 0;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = (art_u8 *) pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;
  pixbuf->destroy_data = dfunc_data;
  pixbuf->destroy = dfunc;

  return pixbuf;
}

ArtPixBuf *
art_pixbuf_new_rgba_dnotify (art_u8 *pixels, int width, int height, int rowstride,
			     void *dfunc_data, ArtDestroyNotify dfunc)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 4;
  pixbuf->has_alpha = 1;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = (art_u8 *) pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;
  pixbuf->destroy_data = dfunc_data;
  pixbuf->destroy = dfunc;

  return pixbuf;
}

ArtPixBuf *
art_pixbuf_new_const_rgb (const art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgb_dnotify ((art_u8 *) pixels, width, height, rowstride, NULL, NULL);
}

ArtPixBuf *
art_pixbuf_new_const_rgba (const art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgba_dnotify ((art_u8 *) pixels, width, height, rowstride, NULL, NULL);
}

static void
art_pixel_destroy (void *func_data, void *data)
{
  art_free (data);
}

ArtPixBuf *
art_pixbuf_new_rgb (art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgb_dnotify (pixels, width, height, rowstride, NULL, art_pixel_destroy);
}

ArtPixBuf *
art_pixbuf_new_rgba (art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgba_dnotify (pixels, width, height, rowstride, NULL, art_pixel_destroy);
}

/* free an ArtPixBuf with destroy notification */
void
art_pixbuf_free (ArtPixBuf *pixbuf)
{
  ArtDestroyNotify destroy = pixbuf->destroy;
  void *destroy_data = pixbuf->destroy_data;
  art_u8 *pixels = pixbuf->pixels;

  pixbuf->pixels = NULL;
  pixbuf->destroy = NULL;
  pixbuf->destroy_data = NULL;

  if (destroy)
    destroy (destroy_data, pixels);

  art_free (pixbuf);
}

/* deprecated function, use the _dnotify variants for allocation instead */
void
art_pixbuf_free_shallow (ArtPixBuf *pixbuf)
{
  art_free (pixbuf);
}

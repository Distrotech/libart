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

#include <math.h>
#include "art_misc.h"
#include "art_rect.h"

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

/* rectangle primitives stolen from gzilla */

/* Make a copy of the rectangle. */
void
art_irect_copy (ArtIRect *dest, const ArtIRect *src) {
  dest->x0 = src->x0;
  dest->y0 = src->y0;
  dest->x1 = src->x1;
  dest->y1 = src->y1;
}

/* Find the smallest rectangle that includes both source rectangles. */
void
art_irect_union (ArtIRect *dest, const ArtIRect *src1, const ArtIRect *src2) {
  if (art_irect_empty (src1)) {
    art_irect_copy (dest, src2);
  } else if (art_irect_empty (src2)) {
    art_irect_copy (dest, src1);
  } else {
    dest->x0 = MIN (src1->x0, src2->x0);
    dest->y0 = MIN (src1->y0, src2->y0);
    dest->x1 = MAX (src1->x1, src2->x1);
    dest->y1 = MAX (src1->y1, src2->y1);
  }
}

/* Return the intersection of the two rectangles */
void
art_irect_intersect (ArtIRect *dest, const ArtIRect *src1, const ArtIRect *src2) {
  dest->x0 = MAX (src1->x0, src2->x0);
  dest->y0 = MAX (src1->y0, src2->y0);
  dest->x1 = MIN (src1->x1, src2->x1);
  dest->y1 = MIN (src1->y1, src2->y1);
}

/* Return true if the rectangle is empty. */
int
art_irect_empty (const ArtIRect *src) {
  return (src->x1 <= src->x0 || src->y1 <= src->y0);
}

#if 0
gboolean irect_point_inside (ArtIRect *rect, GzwPoint *point) {
  return (point->x >= rect->x0 && point->y >= rect->y0 &&
	  point->x < rect->x1 && point->y < rect->y1);
}
#endif

void
art_bbox_affine_transform (ArtDRect *dst, const ArtDRect *src, const double matrix[6])
{
  double x0, y0, x1, y1;

  /* note: this only works for 90 degree rotations */
  x0 = src->x0 * matrix[0] + src->y0 * matrix[2] + matrix[4];
  y0 = src->x0 * matrix[1] + src->y0 * matrix[3] + matrix[5];
  x1 = src->x1 * matrix[0] + src->y1 * matrix[2] + matrix[4];
  y1 = src->x1 * matrix[1] + src->y1 * matrix[3] + matrix[5];
  dst->x0 = MIN (x0, x1);
  dst->x1 = MAX (x0, x1);
  dst->y0 = MIN (y0, y1);
  dst->y1 = MAX (y0, y1);
}

void
art_drect_to_irect (ArtIRect *dst, ArtDRect *src)
{
  dst->x0 = floor (src->x0);
  dst->y0 = floor (src->y0);
  dst->x1 = ceil (src->x1);
  dst->y1 = ceil (src->y1);
}

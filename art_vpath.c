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

/* Basic constructors and operations for vector paths */

#include <math.h>

#include "art_misc.h"

#include "art_rect.h"
#include "art_vpath.h"

void
art_vpath_add_point (ArtVpath **p_vpath, int *pn_points, int *pn_points_max,
		     ArtPathcode code, double x, double y)
{
  int i;

  i = (*pn_points)++;
  if (i == *pn_points_max)
    art_expand (*p_vpath, ArtVpath, *pn_points_max);
  (*p_vpath)[i].code = code;
  (*p_vpath)[i].x = x;
  (*p_vpath)[i].y = y;
}

/* number of steps should really depend on radius. */
#define CIRCLE_STEPS 128

ArtVpath *
art_vpath_new_circle (double x, double y, double r)
{
  int i;
  ArtVpath *vec;
  double theta;

  vec = art_new (ArtVpath, CIRCLE_STEPS + 2);

  for (i = 0; i < CIRCLE_STEPS + 1; i++)
    {
      vec[i].code = i ? ART_LINETO : ART_MOVETO;
      theta = (i & (CIRCLE_STEPS - 1)) * (M_PI * 2.0 / CIRCLE_STEPS);
      vec[i].x = x + r * cos (theta);
      vec[i].y = y - r * sin (theta);
    }
  vec[i].code = ART_END;

  return vec;
}

/* Result (x', y') = (matrix[0] * x + matrix[2] * y + matrix[4],
                      matrix[1] * x + matrix[3] * y + matrix[5])

   Returns newly allocated transformed path. */
ArtVpath *
art_vpath_affine_transform (const ArtVpath *src, const double matrix[6])
{
  int i;
  int size;
  ArtVpath *new;
  double x, y;

  for (i = 0; src[i].code != ART_END; i++);
  size = i;

  new = art_new (ArtVpath, size + 1);

  for (i = 0; i < size; i++)
    {
      new[i].code = src[i].code;
      x = src[i].x;
      y = src[i].y;
      new[i].x = matrix[0] * x + matrix[2] * y + matrix[4];
      new[i].y = matrix[1] * x + matrix[3] * y + matrix[5];
    }
  new[i].code = ART_END;

  return new;
}

void
art_vpath_bbox_drect (const ArtVpath *vec, ArtDRect *drect)
{
  int i;
  double x0, y0, x1, y1;

  if (vec[0].code == ART_END)
    {
      x0 = y0 = x1 = y1 = 0;
    }
  else
    {
      x0 = x1 = vec[0].x;
      y0 = y1 = vec[0].y;
      for (i = 1; vec[i].code != ART_END; i++)
	{
	  if (vec[i].x < x0) x0 = vec[i].x;
	  if (vec[i].x > x1) x1 = vec[i].x;
	  if (vec[i].y < y0) y0 = vec[i].y;
	  if (vec[i].y > y1) y1 = vec[i].y;
	}
    }
  drect->x0 = x0;
  drect->y0 = y0;
  drect->x1 = x1;
  drect->y1 = y1;
}

void
art_vpath_bbox_irect (const ArtVpath *vec, ArtIRect *irect)
{
  ArtDRect drect;

  art_vpath_bbox_drect (vec, &drect);
  art_drect_to_irect (irect, &drect);
}


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

/* Basic constructors and operations for sorted vector paths */

#include "art_misc.h"

#include "art_rect.h"
#include "art_svp.h"

/* Add a new segment. The arguments can be zero and NULL if the caller
   would rather fill them in later.

   We also realloc one auxiliary array of ints of size n_segs if
   desired.
*/
int
art_svp_add_segment (ArtSVP **p_vp, int *pn_segs_max,
		     int **pn_points_max,
		     int n_points, int dir, ArtPoint *points,
		     ArtDRect *bbox)
{
  int seg_num;
  ArtSVP *svp;
  ArtSVPSeg *seg;

  svp = *p_vp;
  seg_num = svp->n_segs++;
  if (*pn_segs_max == seg_num)
    {
      *pn_segs_max <<= 1;
      svp = (ArtSVP *)art_realloc (svp, sizeof(ArtSVP) +
				   (*pn_segs_max - 1) * sizeof(ArtSVPSeg));
      *p_vp = svp;
      if (pn_points_max != NULL)
	*pn_points_max = art_renew (*pn_points_max, int, *pn_segs_max);
    }
  seg = &svp->segs[seg_num];
  seg->n_points = n_points;
  seg->dir = dir;
  seg->points = points;
  if (bbox)
    seg->bbox = *bbox;
  else if (points)
    {
      double x_min, x_max;
      int i;

      x_min = x_max = points[0].x;
      for (i = 1; i < n_points; i++)
	{
	  if (x_min > points[i].x)
	    x_min = points[i].x;
	  if (x_max < points[i].x)
	    x_max = points[i].x;
	}
      seg->bbox.x0 = x_min;
      seg->bbox.y0 = points[0].y;
      
      seg->bbox.x1 = x_max;
      seg->bbox.y1 = points[n_points - 1].y;
    }
  return seg_num;
}

void
art_svp_free (ArtSVP *svp)
{
  int n_segs = svp->n_segs;
  int i;

  for (i = 0; i < n_segs; i++)
    art_free (svp->segs[i].points);
  art_free (svp);
}

#define EPSILON 1e-6

int
art_svp_seg_compare (const void *s1, const void *s2)
{
  const ArtSVPSeg *seg1 = s1;
  const ArtSVPSeg *seg2 = s2;

  if (seg1->points[0].y - EPSILON > seg2->points[0].y) return 1;
  else if (seg1->points[0].y + EPSILON < seg2->points[0].y) return -1;
  else if (seg1->points[0].x - EPSILON > seg2->points[0].x) return 1;
  else if (seg1->points[0].x + EPSILON < seg2->points[0].x) return -1;
  else if ((seg1->points[1].x - seg1->points[0].x) *
	   (seg2->points[1].y - seg2->points[0].y) -
	   (seg1->points[1].y - seg1->points[0].y) *
	   (seg2->points[1].x - seg2->points[0].x) > 0) return 1;
  else return -1;
}


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

/* Basic constructors and operations for bezier paths */

#include <math.h>

#include "art_misc.h"

#include "art_bpath.h"
#include "art_vpath.h"

/* p must be allocated 2^level points. */

/* level must be >= 1 */
ArtPoint *
art_bezier_to_vec (double x0, double y0,
		   double x1, double y1,
		   double x2, double y2,
		   double x3, double y3,
		   ArtPoint *p,
		   int level)
{
  double x_m, y_m;

#ifdef VERBOSE
  printf ("bezier_to_vec: %g,%g %g,%g %g,%g %g,%g %d\n",
	  x0, y0, x1, y1, x2, y2, x3, y3, level);
#endif
  if (level == 1) {
    x_m = (x0 + 3 * (x1 + x2) + x3) * 0.125;
    y_m = (y0 + 3 * (y1 + y2) + y3) * 0.125;
    p->x = x_m;
    p->y = y_m;
    p++;
    p->x = x3;
    p->y = y3;
    p++;
#ifdef VERBOSE
    printf ("-> (%g, %g) -> (%g, %g)\n", x_m, y_m, x3, y3);
#endif
  } else {
    double xa1, ya1;
    double xa2, ya2;
    double xb1, yb1;
    double xb2, yb2;

    xa1 = (x0 + x1) * 0.5;
    ya1 = (y0 + y1) * 0.5;
    xa2 = (x0 + 2 * x1 + x2) * 0.25;
    ya2 = (y0 + 2 * y1 + y2) * 0.25;
    xb1 = (x1 + 2 * x2 + x3) * 0.25;
    yb1 = (y1 + 2 * y2 + y3) * 0.25;
    xb2 = (x2 + x3) * 0.5;
    yb2 = (y2 + y3) * 0.5;
    x_m = (xa2 + xb1) * 0.5;
    y_m = (ya2 + yb1) * 0.5;
#ifdef VERBOSE
    printf ("%g,%g %g,%g %g,%g %g,%g\n", xa1, ya1, xa2, ya2,
	    xb1, yb1, xb2, yb2);
#endif
    p = art_bezier_to_vec (x0, y0, xa1, ya1, xa2, ya2, x_m, y_m, p, level - 1);
    p = art_bezier_to_vec (x_m, y_m, xb1, yb1, xb2, yb2, x3, y3, p, level - 1);
  }
  return p;
}

#define RENDER_LEVEL 4
#define RENDER_SIZE (1 << (RENDER_LEVEL))

/* Creates a new vector path, given a bezier path. The flatness
   argument is present in the api but is not used. A value of 0.5
   should usually be appropriate for antialiased display (1 for "lego"
   displays) - at least if the resulting vpath is not going to be
   scaled. */

/* We could scan first and allocate to fit, but we don't. */
ArtVpath *
art_bez_path_to_vec (ArtBpath *bez, double flatness)
{
  ArtVpath *vec;
  int vec_n, vec_n_max;
  int bez_index;
  int space_needed;
  double x, y;
  ArtPoint seg[RENDER_SIZE];
  int i;

  vec_n = 0;
  vec_n_max = RENDER_SIZE;
  vec = art_new (ArtVpath, vec_n_max);

  /* Initialization is unnecessary because of the precondition that the
     bezier path does not begin with LINETO or CURVETO, but is here
     to make the code warning-free. */
  x = 0;
  y = 0;

  bez_index = 0;
  do
    {
#ifdef VERBOSE
      printf ("%s %g %g\n",
	      bez[bez_index].code == ART_CURVETO ? "curveto" :
	      bez[bez_index].code == ART_LINETO ? "lineto" :
	      bez[bez_index].code == ART_MOVETO ? "moveto" :
	      bez[bez_index].code == ART_MOVETO_OPEN ? "moveto-open" :
	      "end", bez[bez_index].x3, bez[bez_index].y3);
#endif
      if (bez[bez_index].code == ART_CURVETO)
	space_needed = RENDER_SIZE;
      else
	space_needed = 1;
      if (vec_n + space_needed > vec_n_max)
	/* We know a simple doubling will do it because space_needed
	   is always <= vec_n_max */
	art_expand (vec, ArtVpath, vec_n_max);
      switch (bez[bez_index].code)
	{
	case ART_MOVETO_OPEN:
	case ART_MOVETO:
	case ART_LINETO:
	  x = bez[bez_index].x3;
	  y = bez[bez_index].y3;
	  vec[vec_n].code = bez[bez_index].code;
	  vec[vec_n].x = x;
	  vec[vec_n].y = y;
	  vec_n++;
	  break;
	case ART_END:
	  vec[vec_n].code = bez[bez_index].code;
	  vec[vec_n].x = 0;
	  vec[vec_n].y = 0;
	  vec_n++;
	  break;
	case ART_CURVETO:
#ifdef VERBOSE
	  printf ("%g,%g %g,%g %g,%g %g,%g\n", x, y,
			 bez[bez_index].x1, bez[bez_index].y1,
			 bez[bez_index].x2, bez[bez_index].y2,
			 bez[bez_index].x3, bez[bez_index].y3);
#endif
	  art_bezier_to_vec (x, y,
			     bez[bez_index].x1, bez[bez_index].y1,
			     bez[bez_index].x2, bez[bez_index].y2,
			     bez[bez_index].x3, bez[bez_index].y3,
			     seg,
			     RENDER_LEVEL);
	  /* This loop is slightly inauspicious. If bezier_to_vec had been
	     coded to use an array of vec_path_els rather than points,
	     it would not be necessary. */
	  for (i = 0; i < RENDER_SIZE; i++)
	    {
	      vec[vec_n].code = ART_LINETO;
	      vec[vec_n].x = seg[i].x;
	      vec[vec_n].y = seg[i].y;
	      vec_n++;
	    }
	  x = bez[bez_index].x3;
	  y = bez[bez_index].y3;
	  break;
	}
    }
  while (bez[bez_index++].code != ART_END);
  return vec;
}


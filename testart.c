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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "art_misc.h"
#include "art_vpath.h"
#include "art_svp.h"
#include "art_svp_vpath.h"
#include "art_gray_svp.h"
#include "art_rgb_svp.h"
#include "art_svp_vpath_stroke.h"
#include "art_svp_ops.h"

ArtVpath *
randstar (int n)
{
  ArtVpath *vec;
  int i;
  double r, th;

  vec = art_new (ArtVpath, n + 2);
  for (i = 0; i < n; i++)
    {
      vec[i].code = i ? ART_LINETO : ART_MOVETO;
      r = rand () * (250.0 / RAND_MAX);
#if 0
      r = r + 0.9 * (250 - r);
#endif
      th = i * 2 * M_PI / n;
      vec[i].x = 250 + r * cos (th);
      vec[i].y = 250 - r * sin (th);
    }
  vec[i].code = ART_LINETO;
  vec[i].x = vec[0].x;
  vec[i].y = vec[0].y;
  i++;
  vec[i].code = ART_END;
  vec[i].x = 0;
  vec[i].y = 0;
  return vec;
}

#define TILE_SIZE 32
#define NUM_ITERS 1
#define COLOR

#ifdef COLOR
#define BYTES_PP 3
#else
#define BYTES_PP 1
#endif

void
print_svp (ArtSVP *vp)
{
  int i, j;

  for (i = 0; i < vp->n_segs; i++)
    {
      printf ("segment %d, dir = %s (%f, %f) - (%f, %f)\n",
	      i, vp->segs[i].dir ? "down" : "up",
	      vp->segs[i].bbox.x0,
	      vp->segs[i].bbox.y0,
	      vp->segs[i].bbox.x1,
	      vp->segs[i].bbox.y1);
      for (j = 0; j < vp->segs[i].n_points; j++)
        printf ("  (%g, %g)\n",
                vp->segs[i].points[j].x,
                vp->segs[i].points[j].y);
    }
}

int
main (int argc, char **argv)
{
  ArtVpath *vpath, *vpath2;
  ArtSVP *svp, *svp2;
  ArtSVP *svp3;
  art_u8 buf[512 * 512 * BYTES_PP];
  int i, j;
  int iter;

  vpath = randstar (50);
  svp = art_svp_from_vpath (vpath);

  vpath2 = randstar (50);
  svp2 = art_svp_vpath_stroke (vpath2,
			       ART_PATH_STROKE_JOIN_MITER,
			       ART_PATH_STROKE_CAP_BUTT,
			       15,
			       4,
			       0.5);

  svp3 = art_svp_intersect (svp, svp2);
  /*
  print_svp (svp2);
  */

#ifdef COLOR
  printf ("P6\n512 512\n255\n");
#else
  printf ("P5\n512 512\n255\n");
#endif
  for (iter = 0; iter < NUM_ITERS; iter++)
    for (j = 0; j < 512; j += TILE_SIZE)
      for (i = 0; i < 512; i += TILE_SIZE)
#ifdef COLOR
	{
	  art_rgb_svp_aa (svp, i, j, i + TILE_SIZE, j + TILE_SIZE,
			  0xffe0a0, 0x100040,
			  buf + (j * 512 + i) * BYTES_PP, 512 * BYTES_PP);
	  art_rgb_svp_alpha (svp2, i, j, i + TILE_SIZE, j + TILE_SIZE,
			     0xff000080,
			     buf + (j * 512 + i) * BYTES_PP, 512 * BYTES_PP);
	  art_rgb_svp_alpha (svp3, i, j, i + TILE_SIZE, j + TILE_SIZE,
			     0x00ff0080,
			     buf + (j * 512 + i) * BYTES_PP, 512 * BYTES_PP);
	}
#else
	art_gray_svp_aa (svp, i, j, i + TILE_SIZE, j + TILE_SIZE,
		   buf + (j * 512 + i) * BYTES_PP, 512 * BYTES_PP);
#endif

  fwrite (buf, 1, 512 * 512 * BYTES_PP, stdout);

  return 0;
}

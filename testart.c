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
#include "art_svp_render_aa.h"

/* This function needs to go into libart proper. */
typedef struct _byte_data byte_data;

struct _byte_data {
  art_u8 *buf;
  int rowstride;
  int x0, x1;
};

static void
byte_callback (void *callback_data, int y,
	       int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  byte_data *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  int running_sum = start;
  int i_sum;
  int x0, x1;
  int k;

#if 0
  printf ("start = %d", start);
  running_sum = start;
  for (k = 0; k < n_steps; k++)
    {
      running_sum += steps[k].delta;
      printf (" %d:%d", steps[k].x, running_sum >> 16);
    }
  printf ("\n");
#endif

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  if (run_x1 >= x1) run_x1 = x1;
	  i_sum = running_sum >> 16;
	  memset (linebuf, i_sum, run_x1 - x0);
	}
      else
	run_x1 = x0;

      /* render the steps into tmpbuf */
      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = steps[k].x;
	  run_x1 = steps[k + 1].x;
	  if (run_x0 < x0) run_x0 = x0;
	  if (run_x1 >= x1) run_x1 = x1;
	  i_sum = running_sum >> 16;
	  if (run_x1 > run_x0)
	    memset (linebuf + run_x0 - x0, i_sum, run_x1 - run_x0);
	}
      running_sum += steps[k].delta;
      run_x0 = steps[k].x;
      if (run_x0 < x1)
	{
	  if (run_x0 < x0) run_x0 = x0;
	  i_sum = running_sum >> 16;
	  memset (linebuf + run_x0 - x0, i_sum, x1 - run_x0);
	}
    }
  else
    {
      i_sum = running_sum >> 16;
      memset (linebuf, i_sum, x1 - x0);
    }

  data->buf += data->rowstride;
}

/* Render the vector path into the bytemap. Each pixel gets a value
   proportional to the area within the pixel overlapping the (filled)
   vector path */

void
render_aa (const ArtSVP *svp,
	   int x0, int y0, int x1, int y1,
	   art_u8 *buf, int rowstride)
{
  byte_data data;

  data.buf = buf;
  data.rowstride = rowstride;
  data.x0 = x0;
  data.x1 = x1;
  art_svp_render_aa (svp, x0, y0, x1, y1, byte_callback, &data);
}


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

int
main (int argc, char **argv)
{
  ArtVpath *vpath;
  ArtSVP *svp;
  art_u8 buf[512 * 512];

  vpath = randstar (50);
  svp = art_svp_from_vpath (vpath);
  printf ("P5\n512 512\n255\n");
  render_aa (svp, 0, 0, 512, 512, buf, 512);
  fwrite (buf, 1, 512 * 512, stdout);

  return 0;
}

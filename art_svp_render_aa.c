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

/* The spiffy antialiased renderer for sorted vector paths. */

#include <math.h>
#include "art_misc.h"

#include "art_rect.h"
#include "art_svp.h"

#include "art_svp_render_aa.h"

static void
art_svp_render_insert_active (int i, int *active_segs, int n_active_segs,
			      double *seg_x, double *seg_dx)
{
  int j;
  double x;
  int tmp1, tmp2;

  /* this is a cheap hack to get ^'s sorted correctly */
  x = seg_x[i] + 0.001 * seg_dx[i];
  for (j = 0; j < n_active_segs && seg_x[active_segs[j]] < x; j++);

  tmp1 = i;
  while (j <= n_active_segs)
    {
      tmp2 = active_segs[j];
      active_segs[j] = tmp1;
      tmp1 = tmp2;
      j++;
    }
}

static void
art_svp_render_delete_active (int *active_segs, int j, int n_active_segs)
{
  int k;

  for (k = j; k < n_active_segs; k++)
    active_segs[k] = active_segs[k + 1];
}

static int
art_svp_render_step_compare (const void *s1, const void *s2)
{
  const ArtSVPRenderAAStep *step1 = s1;
  const ArtSVPRenderAAStep *step2 = s2;

  return step1->x - step2->x;
}
void
art_svp_render_aa (const ArtSVP *svp,
		   int x0, int y0, int x1, int y1,
		   void (*callback) (void *callback_data,
				     int y,
				     int start,
				     ArtSVPRenderAAStep *steps, int n_steps),
		   void *callback_data)
{
  int *active_segs;
  int n_active_segs;
  int *cursor;
  double *seg_x;
  double *seg_dx;
  int i, j;
  int y;
  int seg_index;
  int x;
  ArtSVPRenderAAStep *steps;
  int n_steps;
  int n_steps_max;
  double y_top, y_bot;
  double x_top, x_bot;
  double x_min, x_max;
  int ix_min, ix_max;
  double delta, last, this; /* delta should be int too? */
  int xdelta;
  double rslope, drslope;
  int start;
  const ArtSVPSeg *seg;
  int curs;

  active_segs = art_new (int, svp->n_segs);
  cursor = art_new (int, svp->n_segs);
  seg_x = art_new (double, svp->n_segs);
  seg_dx = art_new (double, svp->n_segs);

  n_steps_max = 256;
  steps = art_new (ArtSVPRenderAAStep, n_steps_max);

  n_active_segs = 0;

  i = 0;
  /* iterate through the scanlines */
  for (y = y0; y < y1; y++)
    {
      /* insert new active segments */
      for (; i < svp->n_segs && svp->segs[i].bbox.y0 < y + 1; i++)
	{
	  if (svp->segs[i].bbox.y1 > y)
	    {
	      seg = &svp->segs[i];
	      /* move cursor to topmost vector which overlaps [y,y+1) */
	      for (curs = 0; seg->points[curs + 1].y < y; curs++);
	      cursor[i] = curs;
	      seg_dx[i] = (seg->points[curs + 1].x - seg->points[curs].x) /
		(seg->points[curs + 1].y - seg->points[curs].y);
	      seg_x[i] = seg->points[curs].x +
		(y - seg->points[curs].y) * seg_dx[i];
	      art_svp_render_insert_active (i, active_segs, n_active_segs++,
					    seg_x, seg_dx);
	    }
	}

      n_steps = 0;

      /* render the runlengths, advancing and deleting as we go */
      start = 0x8000;

      for (j = 0; j < n_active_segs; j++)
	{
	  seg_index = active_segs[j];
	  seg = &svp->segs[seg_index];
	  curs = cursor[seg_index];
	  while (curs != seg->n_points - 1 &&
		 seg->points[curs].y < y + 1)
	    {
	      y_top = y;
	      if (y_top < seg->points[curs].y)
		y_top = seg->points[curs].y;
	      y_bot = y + 1;
	      if (y_bot > seg->points[curs + 1].y)
		y_bot = seg->points[curs + 1].y;
	      if (y_top != y_bot) {
		delta = (seg->dir ? 16711680.0 : -16711680.0) *
		  (y_bot - y_top);
		x_top = seg_x[seg_index] + (y_top - y) * seg_dx[seg_index];
		x_bot = seg_x[seg_index] + (y_bot - y) * seg_dx[seg_index];
		if (x_top < x_bot)
		  {
		    x_min = x_top;
		    x_max = x_bot;
		  }
		else
		  {
		    x_min = x_bot;
		    x_max = x_top;
		  }
		ix_min = floor (x_min);
		ix_max = floor (x_max);
		if (ix_min >= x1)
		  {
		    /* skip; it starts to the right of the render region */
		  }
		else if (ix_max < x0)
		  /* it ends to the left of the render region */
		  start += delta;
		else if (ix_min == ix_max)
		  {
		    /* case 1, antialias a single pixel */
		    if (n_steps + 2 > n_steps_max)
		      art_expand (steps, ArtSVPRenderAAStep, n_steps_max);
		    xdelta = (ix_min + 1 - (x_min + x_max) * 0.5) * delta;
		    steps[n_steps].x = ix_min;
		    steps[n_steps].delta = xdelta;
		    n_steps++;
		    xdelta = delta - xdelta;
		    steps[n_steps].x = ix_min + 1;
		    steps[n_steps].delta = xdelta;
		    n_steps++;
		  }
		else
		  {
		    /* case 2, antialias a run */
		    if (n_steps + x_max + 1 - x_min > n_steps_max)
		      {
			do
			  n_steps_max <<= 1;
			while (n_steps + x_max + 1 - x_min > n_steps_max);
			steps = art_renew (steps, ArtSVPRenderAAStep,
					   n_steps_max);
		      }
		    rslope = 1.0 / fabs (seg_dx[seg_index]);
		    drslope = delta * rslope;
		    last =
		      drslope * 0.5 *
		      (ix_min + 1 - x_min) * (ix_min + 1 - x_min);
		    xdelta = last;
		    steps[n_steps].x = ix_min;
		    steps[n_steps].delta = last;
		    n_steps++;
		    for (x = ix_min + 1; x < ix_max; x++)
		      {
			this = (seg->dir ? 16711680.0 : -16711680.0) * rslope * (x + 0.5 - x_min);
			xdelta = this - last;
			last = this;
			steps[n_steps].x = x;
			steps[n_steps].delta = xdelta;
			n_steps++;
		      }
		    this =
		      delta * (1 - 0.5 *
			       (x_max - ix_max) * (x_max - ix_max) *
			       rslope);
		    xdelta = this - last;
		    last = this;
		    steps[n_steps].x = ix_max;
		    steps[n_steps].delta = xdelta;
		    n_steps++;
		    xdelta = delta - last;
		    steps[n_steps].x = ix_max + 1;
		    steps[n_steps].delta = xdelta;
		    n_steps++;
		  }
	      }
	      curs++;
	      if (curs != seg->n_points - 1 &&
		  seg->points[curs].y < y + 1)
		{
		  seg_dx[seg_index] = (seg->points[curs + 1].x -
				       seg->points[curs].x) /
		    (seg->points[curs + 1].y - seg->points[curs].y);
		  seg_x[seg_index] = seg->points[curs].x +
		    (y - seg->points[curs].y) * seg_dx[seg_index];
		}
	      /* break here, instead of duplicating predicate in while? */
	    }
	  if (seg->points[curs].y >= y + 1)
	    {
	      curs--;
	      cursor[seg_index] = curs;
	      seg_x[seg_index] += seg_dx[seg_index];
	    }
	  else
	    {
	      art_svp_render_delete_active (active_segs, j--,
					    --n_active_segs);
	    }
	}

      /* sort the steps */
      if (n_steps)
	qsort (steps, n_steps, sizeof(ArtSVPRenderAAStep),
	       art_svp_render_step_compare);

      (*callback) (callback_data, y, start, steps, n_steps);
    }

  art_free (steps);

  art_free (seg_dx);
  art_free (seg_x);
  art_free (cursor);
  art_free (active_segs);
}

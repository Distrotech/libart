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

/* Sort vector paths into sorted vector paths */

#include <stdlib.h>
#include <math.h>

#include "art_misc.h"

#include "art_vpath.h"
#include "art_svp.h"
#include "art_svp_wind.h"
#include "art_svp_vpath.h"
#include "art_svp_vpath_stroke.h"

#define EPSILON 1e-6
#define EPSILON_2 1e-12

#define yes_OPTIMIZE_INNER

/* Assume that forw and rev are at point i0. Bring them to i1,
   joining with the vector i1 - i2.

   It so happens that all invocations of this function maintain
   the precondition i1 = i0 + 1, so we could decrease the number
   of arguments by one. We haven't done that here, though.

   forw is to the line's right and rev is to its left.

   Also need to implement a "flatness" value for the round joins.

   Precondition: no zero-length vectors, otherwise a divide by
   zero will happen.
*/
static void
render_seg (ArtVpath **p_forw, int *pn_forw, int *pn_forw_max,
	    ArtVpath **p_rev, int *pn_rev, int *pn_rev_max,
	    ArtVpath *vpath, int i0, int i1, int i2,
	    ArtPathStrokeJoinType join,
	    double line_width, double miter_limit)
{
  double dx0, dy0;
  double dx1, dy1;
  double dlx0, dly0;
  double dlx1, dly1;
  double dmx, dmy;
  double dmr2;
  double scale;
  double cross;

  /* The vectors of the lines from i0 to i1 and i1 to i2. */
  dx0 = vpath[i1].x - vpath[i0].x;
  dy0 = vpath[i1].y - vpath[i0].y;

  dx1 = vpath[i2].x - vpath[i1].x;
  dy1 = vpath[i2].y - vpath[i1].y;

  /* Set dl[xy]0 to the vector from i0 to i1, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx0 * dx0 + dy0 * dy0);
  dlx0 = dy0 * scale;
  dly0 = -dx0 * scale;

  /* Set dl[xy]1 to the vector from i1 to i2, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx1 * dx1 + dy1 * dy1);
  dlx1 = dy1 * scale;
  dly1 = -dx1 * scale;

#ifdef VERBOSE
  printf ("%% render_seg: (%g, %g) - (%g, %g) - (%g, %g)\n",
	  vpath[i0].x, vpath[i0].y,
	  vpath[i1].x, vpath[i1].y,
	  vpath[i2].x, vpath[i2].y);

  printf ("%% render_seg: d[xy]0 = (%g, %g), dl[xy]0 = (%g, %g)\n",
	  dx0, dy0, dlx0, dly0);

  printf ("%% render_seg: d[xy]1 = (%g, %g), dl[xy]1 = (%g, %g)\n",
	  dx1, dy1, dlx1, dly1);
#endif

  /* now, forw's last point is expected to be colinear along d[xy]0
     to point i0 - dl[xy]0, and rev with i0 + dl[xy]0. */

  /* positive for positive area (i.e. left turn) */
  cross = dx1 * dy0 - dx0 * dy1;

  dmx = (dlx0 + dlx1) * 0.5;
  dmy = (dly0 + dly1) * 0.5;
  dmr2 = dmx * dmx + dmy * dmy;

  if (join == ART_PATH_STROKE_JOIN_MITER &&
      dmr2 * miter_limit * miter_limit < line_width * line_width)
    join = ART_PATH_STROKE_JOIN_BEVEL;

  /* the case when dmr2 is zero or very small bothers me
     (i.e. near a 180 degree angle) */
  scale = line_width * line_width / dmr2;
  dmx *= scale;
  dmy *= scale;

  if (cross * cross < EPSILON_2 && dx0 * dx1 + dy0 * dy1 >= 0)
    {
      /* going straight */
#ifdef VERBOSE
      printf ("%% render_seg: straight\n");
#endif
      art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
		       ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
      art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
		       ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
    }
  else if (cross > 0)
    {
      /* left turn, forw is outside and rev is inside */

#ifdef VERBOSE
      printf ("%% render_seg: left\n");
#endif
      if (
#ifdef NO_OPTIMIZE_INNER
	  0 &&
#endif
	  /* check that i1 + dm[xy] is inside i0-i1 rectangle */
	  (dx0 + dmx) * dx0 + (dy0 + dmy) * dy0 > 0 &&
	  /* and that i1 + dm[xy] is inside i1-i2 rectangle */
	  ((dx1 - dmx) * dx1 + (dy1 - dmy) * dy1 > 0)
#ifdef PEDANTIC_INNER
	  &&
	  /* check that i1 + dl[xy]1 is inside i0-i1 rectangle */
	  (dx0 + dlx1) * dx0 + (dy0 + dly1) * dy0 > 0 &&
	  /* and that i1 + dl[xy]0 is inside i1-i2 rectangle */
	  ((dx1 - dlx0) * dx1 + (dy1 - dly0) * dy1 > 0)
#endif
	  )
	{
	  /* can safely add single intersection point */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dmx, vpath[i1].y + dmy);
	}
      else
	{
	  /* need to loop-de-loop the inside */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x, vpath[i1].y);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx1, vpath[i1].y + dly1);
	}

      if (join == ART_PATH_STROKE_JOIN_BEVEL)
	{
	  /* bevel */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx1, vpath[i1].y - dly1);
	}
      else if (join == ART_PATH_STROKE_JOIN_MITER)
	{
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dmx, vpath[i1].y - dmy);
	}
    }
  else
    {
      /* right turn, rev is outside and forw is inside */
#ifdef VERBOSE
      printf ("%% render_seg: right\n");
#endif

      if (
#ifdef NO_OPTIMIZE_INNER
	  0 &&
#endif
	  /* check that i1 - dm[xy] is inside i0-i1 rectangle */
	  (dx0 - dmx) * dx0 + (dy0 - dmy) * dy0 > 0 &&
	  /* and that i1 - dm[xy] is inside i1-i2 rectangle */
	  ((dx1 + dmx) * dx1 + (dy1 + dmy) * dy1 > 0)
#ifdef PEDANTIC_INNER
	  &&
	  /* check that i1 - dl[xy]1 is inside i0-i1 rectangle */
	  (dx0 - dlx1) * dx0 + (dy0 - dly1) * dy0 > 0 &&
	  /* and that i1 - dl[xy]0 is inside i1-i2 rectangle */
	  ((dx1 + dlx0) * dx1 + (dy1 + dly0) * dy1 > 0)
#endif
	  )
	{
	  /* can safely add single intersection point */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dmx, vpath[i1].y - dmy);
	}
      else
	{
	  /* need to loop-de-loop the inside */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x, vpath[i1].y);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx1, vpath[i1].y - dly1);
	}

      if (join == ART_PATH_STROKE_JOIN_BEVEL)
	{
	  /* bevel */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx1, vpath[i1].y + dly1);
	}
      else if (join == ART_PATH_STROKE_JOIN_MITER)
	{
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dmx, vpath[i1].y + dmy);
	}

    }
}

/* caps i1, under the assumption of a vector from i0 */
static void
render_cap (ArtVpath **p_result, int *pn_result, int *pn_result_max,
	    ArtVpath *vpath, int i0, int i1,
	    ArtPathStrokeCapType cap, double line_width)
{
  double dx0, dy0;
  double dlx0, dly0;
  double scale;

  dx0 = vpath[i1].x - vpath[i0].x;
  dy0 = vpath[i1].y - vpath[i0].y;

  /* Set dl[xy]0 to the vector from i0 to i1, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx0 * dx0 + dy0 * dy0);
  dlx0 = dy0 * scale;
  dly0 = -dx0 * scale;

  /* butt */
  art_vpath_add_point (p_result, pn_result, pn_result_max,
		   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
  art_vpath_add_point (p_result, pn_result, pn_result_max,
		   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
  /* todo: other cap types */
}

ArtVpath *
art_svp_vpath_stroke_raw (ArtVpath *vpath,
			  ArtPathStrokeJoinType join,
			  ArtPathStrokeCapType cap,
			  double line_width,
			  double miter_limit,
			  double flatness)
{
  int begin_idx, end_idx;
  int i;
  ArtVpath *forw, *rev;
  int n_forw, n_rev;
  int n_forw_max, n_rev_max;
  ArtVpath *result;
  int n_result, n_result_max;
  double half_lw = 0.5 * line_width;

  n_forw_max = 16;
  forw = art_new (ArtVpath, n_forw_max);

  n_rev_max = 16;
  rev = art_new (ArtVpath, n_rev_max);

  n_result = 0;
  n_result_max = 16;
  result = art_new (ArtVpath, n_result_max);

  for (begin_idx = 0; vpath[begin_idx].code != ART_END; begin_idx = end_idx)
    {
      n_forw = 0;
      n_rev = 0;

      /* we don't know what the first point joins with until we get to the
	 last point and see if it's closed. So we start with the second
	 line in the path. */
      for (i = begin_idx; vpath[i + 1].code == ART_LINETO;
	   i++)
	{
	  /* render the segment from [i] to [i + 1], joining with [i - 1]
	     and [i + 2] */
	  if (vpath[i + 2].code != ART_LINETO)
	    {
	      /* reached end of path */
	      /* todo: make "closed" detection conform to PostScript
		 semantics (i.e. explicit closepath code rather than
		 the fact that end of the path is the beginning) */
	      if (vpath[i + 1].x == vpath[begin_idx].x &&
		  vpath[i + 1].y == vpath[begin_idx].y)
		{
		  int j;

		  /* path is closed, render join to beginning */
		  render_seg (&forw, &n_forw, &n_forw_max,
			      &rev, &n_rev, &n_rev_max,
			      vpath, i, i + 1, begin_idx + 1,
			      join, half_lw, miter_limit);

#ifdef VERBOSE
		  printf ("%% forw %d, rev %d\n", n_forw, n_rev);
#endif
		  /* do forward path */
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, forw[n_forw - 1].x,
				   forw[n_forw - 1].y);
		  for (j = 0; j < n_forw; j++)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, forw[j].x,
				     forw[j].y);

		  /* do reverse path, reversed */
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, rev[0].x,
				   rev[0].y);
		  for (j = n_rev - 1; j >= 0; j--)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, rev[j].x,
				     rev[j].y);
		}
	      else
		{
		  /* path is open */
		  int j;

		  /* add to forw rather than result to ensure that
		     forw has at least one point. */
		  render_cap (&forw, &n_forw, &n_forw_max,
			      vpath, i, i + 1,
			      cap, half_lw);
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, forw[0].x,
				   forw[0].y);
		  for (j = 1; j < n_forw; j++)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, forw[j].x,
				     forw[j].y);
		  for (j = n_rev - 1; j >= 0; j--)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, rev[j].x,
				     rev[j].y);
		  render_cap (&result, &n_result, &n_result_max,
			      vpath, begin_idx + 1, begin_idx,
			      cap, half_lw);
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_LINETO, forw[0].x,
				   forw[0].y);
		}
	    }
	  else
	    render_seg (&forw, &n_forw, &n_forw_max,
			&rev, &n_rev, &n_rev_max,
			vpath, i, i + 1, i + 2,
			join, half_lw, miter_limit);
	}
      end_idx = i + 1;
    }

  art_free (forw);
  art_free (rev);
#ifdef VERBOSE
  printf ("%% n_result = %d\n", n_result);
#endif
  art_vpath_add_point (&result, &n_result, &n_result_max, ART_END, 0, 0);
  return result;
}

#ifdef VERBOSE

#define XOFF 50
#define YOFF 700

static void
print_ps_vpath (ArtVpath *vpath)
{
  int i;

  for (i = 0; vpath[i].code != ART_END; i++)
    {
      switch (vpath[i].code)
	{
	case ART_MOVETO:
	  printf ("%g %g moveto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	case ART_LINETO:
	  printf ("%g %g lineto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	default:
	  break;
	}
    }
  printf ("stroke showpage\n");
}

static void
print_ps_svp (ArtSVP *vpath)
{
  int i, j;

  printf ("%% begin\n");
  for (i = 0; i < vpath->n_segs; i++)
    {
      printf ("%g setgray\n", vpath->segs[i].dir ? 0.7 : 0);
      for (j = 0; j < vpath->segs[i].n_points; j++)
	{
	  printf ("%g %g %s\n",
		  XOFF + vpath->segs[i].points[j].x,
		  YOFF - vpath->segs[i].points[j].y,
		  j ? "lineto" : "moveto");
	}
      printf ("stroke\n");
    }

  printf ("showpage\n");
}
#endif

/* Render a vector path into a stroked outline.

   Status of this routine:

   Basic correctness: Only miter and bevel line joins are implemented,
   and only butt line caps. Otherwise, seems to be fine.

   Numerical stability: We cheat (adding random perturbation). Thus,
   it seems very likely that no numerical stability problems will be
   seen in practice.

   Speed: Should be pretty good.

   Precision: The perturbation fuzzes the coordinates slightly,
   but not enough to be visible.  */
ArtSVP *
art_svp_vpath_stroke (ArtVpath *vpath,
		      ArtPathStrokeJoinType join,
		      ArtPathStrokeCapType cap,
		      double line_width,
		      double miter_limit,
		      double flatness)
{
  ArtVpath *vpath_stroke, *vpath2;
  ArtSVP *svp, *svp2, *svp3;

  vpath_stroke = art_svp_vpath_stroke_raw (vpath, join, cap,
					   line_width, miter_limit, flatness);
#ifdef VERBOSE
  print_ps_vpath (vpath_stroke);
#endif
  vpath2 = art_vpath_perturb (vpath_stroke);
#ifdef VERBOSE
  print_ps_vpath (vpath2);
#endif
  art_free (vpath_stroke);
  svp = art_svp_from_vpath (vpath2);
#ifdef VERBOSE
  print_ps_svp (svp);
#endif
  art_free (vpath2);
  svp2 = art_svp_uncross (svp);
#ifdef VERBOSE
  print_ps_svp (svp2);
#endif
  art_svp_free (svp);
  svp3 = art_svp_rewind_uncrossed (svp2, ART_WIND_RULE_NONZERO);
#ifdef VERBOSE
  print_ps_svp (svp3);
#endif
  art_svp_free (svp2);

  return svp3;
}

/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 2001 Raph Levien
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

/* This file contains a testbed implementation of the new intersection
   code.
*/

#include <math.h> /* for sqrt */

/* Sanitychecking verifies the main invariant on every priority queue
   point. Do not use in production, as it slows things down way too
   much. */
#define SANITYCHECK

#define VERBOSE
#ifdef VERBOSE
#include <stdio.h>
#endif

#include "art_misc.h"
#include "art_svp.h"
#include "art_svp_intersect.h"

/* A priority queue - perhaps move to a separate file if it becomes
   needed somewhere else */

#define ART_PRIQ_USE_HEAP

typedef struct _ArtPriQ ArtPriQ;
typedef struct _ArtPriPoint ArtPriPoint;

struct _ArtPriQ {
  int n_items;
  int n_items_max;
  ArtPriPoint **items;
};

struct _ArtPriPoint {
  double x;
  double y;
  void *user_data;
};

static ArtPriQ *
art_pri_new (void)
{
  ArtPriQ *result = art_new (ArtPriQ, 1);

  result->n_items = 0;
  result->n_items_max = 16;
  result->items = art_new (ArtPriPoint *, result->n_items_max);
  return result;
}

static void
art_pri_free (ArtPriQ *pq)
{
  art_free (pq->items);
  art_free (pq);
}

static art_boolean
art_pri_empty (ArtPriQ *pq)
{
  return pq->n_items == 0;
}

#ifdef ART_PRIQ_USE_HEAP

/* This heap implementation is based on Vasek Chvatal's course notes:
   http://www.cs.rutgers.edu/~chvatal/notes/pq.html#heap */

static void
art_pri_bubble_up (ArtPriQ *pq, int vacant, ArtPriPoint *missing)
{
  ArtPriPoint **items = pq->items;
  int parent;

  parent = (vacant - 1) >> 1;
  while (vacant > 0 && (missing->y < items[parent]->y ||
			(missing->y == items[parent]->y &&
			 missing->x < items[parent]->x)))
    {
      items[vacant] = items[parent];
      vacant = parent;
      parent = (vacant - 1) >> 1;
    }

  items[vacant] = missing;
}

static void
art_pri_insert (ArtPriQ *pq, ArtPriPoint *point)
{
  if (pq->n_items == pq->n_items_max)
    art_expand (pq->items, ArtPriPoint *, pq->n_items_max);

  art_pri_bubble_up (pq, pq->n_items++, point);
}

static void
art_pri_sift_down_from_root (ArtPriQ *pq, ArtPriPoint *missing)
{
  ArtPriPoint **items = pq->items;
  int vacant = 0, child = 2;
  int n = pq->n_items;

  while (child < n)
    {
      if (items[child - 1]->y < items[child]->y ||
	  (items[child - 1]->y == items[child]->y &&
	   items[child - 1]->x < items[child]->x))
	child--;
      items[vacant] = items[child];
      vacant = child;
      child = (vacant + 1) << 1;
    }
  if (child == n)
    {
      items[vacant] = items[n - 1];
      vacant = n - 1;
    }

  art_pri_bubble_up (pq, vacant, missing);
}

static ArtPriPoint *
art_pri_choose (ArtPriQ *pq)
{
  ArtPriPoint *result = pq->items[0];

  art_pri_sift_down_from_root (pq, pq->items[--pq->n_items]);
  return result;
}

#else

/* Choose least point in queue */
static ArtPriPoint *
art_pri_choose (ArtPriQ *pq)
{
  int i;
  int best = 0;
  double best_x, best_y;
  double y;
  ArtPriPoint *result;

  if (pq->n_items == 0)
    return NULL;

  best_x = pq->items[best]->x;
  best_y = pq->items[best]->y;

  for (i = 1; i < pq->n_items; i++)
    {
      y = pq->items[i]->y;
      if (y < best_y || (y == best_y && pq->items[i]->x < best_x))
	{
	  best = i;
	  best_x = pq->items[best]->x;
	  best_y = y;
	}
    }
  result = pq->items[best];
  pq->items[best] = pq->items[--pq->n_items];
  return result;
}

static void
art_pri_insert (ArtPriQ *pq, ArtPriPoint *point)
{
  if (pq->n_items == pq->n_items_max)
    art_expand (pq->items, ArtPriPoint *, pq->n_items_max);

  pq->items[pq->n_items++] = point;
}

#endif

#ifdef TEST_PRIQ

#include <stdlib.h> /* for rand() */
#include <stdio.h>

static double
double_rand (double lo, double hi, int quant)
{
  int tmp = rand () / (RAND_MAX * (1.0 / quant)) + 0.5;
  return lo + tmp * ((hi - lo) / quant);
}

/*
 * This custom allocator for priority queue points is here so I can
 * test speed. It doesn't look like it will be that significant, but
 * if I want a small improvement later, it's something.
 */

typedef ArtPriPoint *ArtPriPtPool;

static ArtPriPtPool *
art_pri_pt_pool_new (void)
{
  ArtPriPtPool *result = art_new (ArtPriPtPool, 1);
  *result = NULL;
  return result;
}

static ArtPriPoint *
art_pri_pt_alloc (ArtPriPtPool *pool)
{
  ArtPriPoint *result = *pool;
  if (result == NULL)
    return art_new (ArtPriPoint, 1);
  else
    {
      *pool = result->user_data;
      return result;
    }
}

static void
art_pri_pt_free (ArtPriPtPool *pool, ArtPriPoint *pt)
{
  pt->user_data = *pool;
  *pool = pt;
}

static void
art_pri_pt_pool_free (ArtPriPtPool *pool)
{
  ArtPriPoint *pt = *pool;
  while (pt != NULL)
    {
      ArtPriPoint *next = pt->user_data;
      art_free (pt);
      pt = next;
    }
  art_free (pool);
}

int
main (int argc, char **argv)
{
  ArtPriPtPool *pool = art_pri_pt_pool_new ();
  ArtPriQ *pq;
  int i, j;
  const int n_iter = 1;
  const int pq_size = 100;

  for (j = 0; j < n_iter; j++)
    {
      pq = art_pri_new ();

      for (i = 0; i < pq_size; i++)
	{
	  ArtPriPoint *pt = art_pri_pt_alloc (pool);
	  pt->x = double_rand (0, 1, 100);
	  pt->y = double_rand (0, 1, 100);
	  pt->user_data = (void *)i;
	  art_pri_insert (pq, pt);
	}

      while (!art_pri_empty (pq))
	{
	  ArtPriPoint *pt = art_pri_choose (pq);
	  if (n_iter == 1)
	    printf ("(%g, %g), %d\n", pt->x, pt->y, (int)pt->user_data);
	  art_pri_pt_free (pool, pt);
	}

      art_pri_free (pq);
    }
  art_pri_pt_pool_free (pool);
  return 0;
}

#else /* TEST_PRIQ */

/* A virtual class for an "svp writer". A client of this object creates an
   SVP by repeatedly calling "add segment" and "add point" methods on it.
*/

typedef struct _ArtSvpWriterRewind ArtSvpWriterRewind;

/* An implementation of the svp writer virtual class that applies the
   winding rule. */

struct _ArtSvpWriterRewind {
  ArtSvpWriter super;
  ArtWindRule rule;
  ArtSVP *svp;
  int n_segs_max;
  int *n_points_max;
};

static int
art_svp_writer_rewind_add_segment (ArtSvpWriter *self, int wind_left,
				   int delta_wind, double x, double y)
{
  ArtSvpWriterRewind *swr = (ArtSvpWriterRewind *)self;
  ArtSVP *svp;
  ArtSVPSeg *seg;
  art_boolean left_filled, right_filled;
  int wind_right = wind_left + delta_wind;
  int seg_num;
  const int init_n_points_max = 4;

  switch (swr->rule)
    {
    case ART_WIND_RULE_NONZERO:
      left_filled = (wind_left != 0);
      right_filled = (wind_right != 0);
      break;
    case ART_WIND_RULE_INTERSECT:
      left_filled = (wind_left > 1);
      right_filled = (wind_right > 1);
      break;
    case ART_WIND_RULE_ODDEVEN:
      left_filled = (wind_left & 1);
      right_filled = (wind_right & 1);
      break;
    case ART_WIND_RULE_POSITIVE:
      left_filled = (wind_left > 0);
      right_filled = (wind_right > 0);
      break;
    default:
      art_die ("Unknown wind rule %d\n", swr->rule);
    }
  if (left_filled == right_filled)
    {
      /* discard segment now */
#ifdef VERBOSE
      printf ("swr add_segment: %d += %d (%g, %g) --> -1\n",
	      wind_left, delta_wind, x, y);
#endif
      return -1;
   }

  svp = swr->svp;
  seg_num = svp->n_segs++;
  if (swr->n_segs_max == seg_num)
    {
      swr->n_segs_max <<= 1;
      svp = (ArtSVP *)art_realloc (svp, sizeof(ArtSVP) +
				   (swr->n_segs_max - 1) *
				   sizeof(ArtSVPSeg));
      swr->svp = svp;
      swr->n_points_max = art_renew (swr->n_points_max, int,
				     swr->n_segs_max);
    }
  seg = &svp->segs[seg_num];
  seg->n_points = 1;
  seg->dir = right_filled;
  swr->n_points_max[seg_num] = init_n_points_max;
  seg->bbox.x0 = x;
  seg->bbox.y0 = y;
  seg->bbox.x1 = x;
  seg->bbox.y1 = y;
  seg->points = art_new (ArtPoint, init_n_points_max);
  seg->points[0].x = x;
  seg->points[0].y = y;
#ifdef VERBOSE
    printf ("swr add_segment: %d += %d (%g, %g) --> %d(%s)\n",
	    wind_left, delta_wind, x, y, seg_num,
	    seg->dir ? "v" : "^");
#endif
  return seg_num;
}

static void
art_svp_writer_rewind_add_point (ArtSvpWriter *self, int seg_id,
				 double x, double y)
{
  ArtSvpWriterRewind *swr = (ArtSvpWriterRewind *)self;
  ArtSVPSeg *seg;
  int n_points;

#ifdef VERBOSE
  printf ("swr add_point: %d (%g, %g)\n", seg_id, x, y);
#endif
  if (seg_id < 0)
    /* omitted segment */
    return;

  seg = &swr->svp->segs[seg_id];
  n_points = seg->n_points++;
  if (swr->n_points_max[seg_id] == n_points)
    art_expand (seg->points, ArtPoint, swr->n_points_max[seg_id]);
  seg->points[n_points].x = x;
  seg->points[n_points].y = y;
  if (x < seg->bbox.x0)
    seg->bbox.x0 = x;
  if (y < seg->bbox.x1)
    seg->bbox.x1 = x;
  seg->bbox.y1 = y;
}

static void
art_svp_writer_rewind_close_segment (ArtSvpWriter *self, int seg_id)
{
  /* Not needed for this simple implementation. A potential future
     optimization is to merge segments that can be merged safely. */
#ifdef VERBOSE
  printf ("swr close_segment: %d\n", seg_id);
#endif
}

ArtSVP *
art_svp_writer_rewind_reap (ArtSvpWriter *self)
{
  ArtSvpWriterRewind *swr = (ArtSvpWriterRewind *)self;
  ArtSVP *result = swr->svp;

  art_free (swr->n_points_max);
  art_free (swr);
  return result;
}

ArtSvpWriter *
art_svp_writer_rewind_new (ArtWindRule rule)
{
  ArtSvpWriterRewind *result = art_new (ArtSvpWriterRewind, 1);

  result->super.add_segment = art_svp_writer_rewind_add_segment;
  result->super.add_point = art_svp_writer_rewind_add_point;
  result->super.close_segment = art_svp_writer_rewind_close_segment;

  result->rule = rule;
  result->n_segs_max = 16;
  result->svp = art_alloc (sizeof(ArtSVP) + 
			   (result->n_segs_max - 1) * sizeof(ArtSVPSeg));
  result->svp->n_segs = 0;
  result->n_points_max = art_new (int, result->n_segs_max);

  return &result->super;
}

/* Now, data structures for the active list */

typedef struct _ArtActiveSeg ArtActiveSeg;

/* Note: BNEG is 1 for \ lines, and 0 for /. Thus,
   x[(flags & BNEG) ^ 1] <= x[flags & BNEG] */
#define ART_ACTIVE_FLAGS_BNEG 1

/* This flag is set if the segment has been inserted into the active
   list. */
#define ART_ACTIVE_FLAGS_IN_ACTIVE 2

#define ART_ACTIVE_FLAGS_DEL 4
#define ART_ACTIVE_FLAGS_INS 8

/* This flag is set if the seg_id is a valid output segment. */
#define ART_ACTIVE_FLAGS_OUT 16

/* todo: tangle flag */

struct _ArtActiveSeg {
  int flags;
  int wind_left, delta_wind;
  ArtActiveSeg *left, *right; /* doubly linked list structure */

  const ArtSVPSeg *in_seg;
  int in_curs;

  double x[2];
  double y0, y1;
  double a, b, c; /* line equation; ax+by+c = 0 for the line, a^2 + b^2 = 1,
		     and a>0 */

  /* bottom point and intersection point stack */
  int n_stack;
  int n_stack_max;
  ArtPoint *stack;

  /* horiz commit list */
  ArtActiveSeg *horiz_left, *horiz_right;
  double horiz_x;
  int seg_id;
};

typedef struct _ArtIntersectCtx ArtIntersectCtx;

struct _ArtIntersectCtx {
  const ArtSVP *in;
  ArtSvpWriter *out;

  ArtPriQ *pq;

  ArtActiveSeg *active_head;

  double y;
  ArtActiveSeg *horiz_first;
  ArtActiveSeg *horiz_last;

  /* segment index of next input segment to be added to pri q */
  int in_curs;
};

#define EPSILON_A 1e-6 /* Fuzz for insertion of new active segments */
#define EPSILON_B 1e-6 /* Fuzz for crossing tests */
#define EPSILON_C 1e-5 /* Threshold for breaking lines at point insertions */

/**
 * art_svp_intersect_setup_seg: Set up an active segment from input segment.
 * @seg: Active segment.
 * @pri_pt: Priority queue point to initialize.
 *
 * Sets the x[], a, b, c, flags, and stack fields according to the
 * line from the current cursor value. Sets the priority queue point
 * to the bottom point of this line. Also advances the input segment
 * cursor.
 **/
static void
art_svp_intersect_setup_seg (ArtActiveSeg *seg, ArtPriPoint *pri_pt)
{
  const ArtSVPSeg *in_seg = seg->in_seg;
  int in_curs = seg->in_curs++;
  double x0, y0, x1, y1;
  double dx, dy, s;
  double a, b;

  x0 = in_seg->points[in_curs].x;
  y0 = in_seg->points[in_curs].y;
  x1 = in_seg->points[in_curs + 1].x;
  y1 = in_seg->points[in_curs + 1].y;
  pri_pt->x = x1;
  pri_pt->y = y1;
  dx = x1 - x0;
  dy = y1 - y0;
  s = 1 / sqrt (dx * dx + dy * dy);
  seg->a = a = dy * s;
  seg->b = b = -dx * s;
  seg->c = -(a * x0 + b * y0);
  seg->flags = (seg->flags & ~ART_ACTIVE_FLAGS_BNEG) | (dx > 0);
  seg->x[0] = x0;
  seg->x[1] = x1;
  seg->y0 = y0;
  seg->y1 = y1;
  seg->n_stack = 1;
  seg->stack[0].x = x1;
  seg->stack[0].y = y1;
}

static void
art_svp_intersect_push_pt (ArtIntersectCtx *ctx, ArtActiveSeg *seg,
			   double x, double y)
{
  ArtPriPoint *pri_pt;
  int n_stack = seg->n_stack;

  if (n_stack == seg->n_stack_max)
    art_expand (seg->stack, ArtPoint, seg->n_stack_max);
  seg->stack[n_stack].x = x;
  seg->stack[n_stack].y = y;
  seg->n_stack++;

  seg->x[1] = x;
  seg->y1 = y;

  pri_pt = art_new (ArtPriPoint, 1);
  pri_pt->x = x;
  pri_pt->y = y;
  pri_pt->user_data = seg;
  art_pri_insert (ctx->pq, pri_pt);
}

/**
 * art_svp_intersect_break: Break an active segment at the scan line.
 *
 * Note: y must be greater than the top point's y, and less than
 * the bottom's.
 *
 * Return value: x coordinate of break point.
 */
static double
art_svp_intersect_break (ArtIntersectCtx *ctx, ArtActiveSeg *seg,
			 double y)
{
  double x0, y0, x1, y1;
  const ArtSVPSeg *in_seg = seg->in_seg;
  int in_curs = seg->in_curs;
  double x;

  x0 = in_seg->points[in_curs - 1].x;
  y0 = in_seg->points[in_curs - 1].y;
  x1 = in_seg->points[in_curs].x;
  y1 = in_seg->points[in_curs].y;
  x = x0 + (x1 - x0) * ((y - y0) / (y1 - y0));

  /* I think we can count on min(x0, x1) <= x <= max(x0, x1) with sane
     arithmetic, but it might be worthwhile to check just in case. */

  art_svp_intersect_push_pt (ctx, seg, x, y);

  return x;
}

/**
 * art_svp_intersect_test_cross: Test crossing of a pair of active segments.
 * @ctx: Intersector context.
 * @left_seg: Left segment of the pair.
 * @right_seg: Right segment of the pair.
 *
 * Tests crossing of @left_seg and @right_seg. If there is a crossing,
 * inserts the intersection point into both segments.
 *
 * Return value: True if the intersection took place at the current
 * scan line, indicating further iteration is needed.
 **/
static art_boolean
art_svp_intersect_test_cross (ArtIntersectCtx *ctx,
			      ArtActiveSeg *left_seg, ArtActiveSeg *right_seg)
{
  double left_x0, left_y0, left_x1;
  double left_y1 = left_seg->y1;
  double right_y1 = right_seg->y1;
  double d;

  const ArtSVPSeg *in_seg;
  int in_curs;
  double d0, d1, t;
  double x, y; /* intersection point */

#ifdef VERBOSE 
  static int count = 0;

  printf ("art_svp_intersect_test_cross %lx <-> %lx: count=%d\n",
	  (unsigned long)left_seg, (unsigned long)right_seg, count++);
#endif

  if (left_y1 < right_y1)
    {
      /* Test left (x1, y1) against right segment */
      double left_x1 = left_seg->x[1];

      if (left_x1 <
	  right_seg->x[(right_seg->flags & ART_ACTIVE_FLAGS_BNEG) ^ 1] ||
	  left_y1 == right_seg->y0)
	return ART_FALSE;
      d = left_x1 * right_seg->a + left_y1 * right_seg->b + right_seg->c;
      if (d < -EPSILON_C)
	return ART_FALSE;
      else if (d < EPSILON_C)
	{
	  art_warn ("art_svp_intersect_test_cross: need to break\n");
	}
    }
  else if (left_y1 > right_y1)
    {
      /* Test right (x1, y1) against left segment */
      double right_x1 = right_seg->x[1];

      if (right_x1 > left_seg->x[left_seg->flags & ART_ACTIVE_FLAGS_BNEG] ||
	  right_y1 == left_seg->y0)
	return ART_FALSE;
      d = right_x1 * left_seg->a + right_y1 * left_seg->b + left_seg->c;
      if (d > EPSILON_C)
	return ART_FALSE;
      else if (d > -EPSILON_C)
	{
	  art_warn ("art_svp_intersect_test_cross: need to break\n");
	}
    }
  else /* left_y1 == right_y1 */
    { 
      double left_x1 = left_seg->x[1];
      double right_x1 = right_seg->x[1];

      if (left_x1 <= right_x1)
	return ART_FALSE;
    }

  /* The segments cross. Find the intersection point. */

  in_seg = left_seg->in_seg;
  in_curs = left_seg->in_curs;
  left_x0 = in_seg->points[in_curs - 1].x;
  left_y0 = in_seg->points[in_curs - 1].y;
  left_x1 = in_seg->points[in_curs].x;
  left_y1 = in_seg->points[in_curs].y;
  d0 = left_x0 * right_seg->a + left_y0 * right_seg->b + right_seg->c;
  d1 = left_x1 * right_seg->a + left_y1 * right_seg->b + right_seg->c;
  if (d0 == d1)
    {
      x = left_x0;
      y = left_y0;
    }
  else
    {
      /* Is this division always safe? It could possibly overflow. */
      t = d0 / (d0 - d1);
      if (t <= 0)
	{
	  x = left_x0;
	  y = left_y0;
	}
      else if (t >= 1)
	{
	  x = left_x1;
	  y = left_y1;
	}
      else
	{
	  x = left_x0 + t * (left_x1 - left_x0);
	  y = left_y0 + t * (left_y1 - left_y0);
	}
    }

  /* Make sure intersection point is within bounds of right seg. */
  if (y < right_seg->y0)
    {
      x = right_seg->x[0];
      y = right_seg->y0;
    }
  else if (y > right_seg->y1)
    {
      x = right_seg->x[1];
      y = right_seg->y1;
    }
  else if (x < right_seg->x[(right_seg->flags & ART_ACTIVE_FLAGS_BNEG) ^ 1])
    x = right_seg->x[(right_seg->flags & ART_ACTIVE_FLAGS_BNEG) ^ 1];
  else if (x > right_seg->x[right_seg->flags & ART_ACTIVE_FLAGS_BNEG])
    x = right_seg->x[right_seg->flags & ART_ACTIVE_FLAGS_BNEG];

  if (y == left_seg->y0)
    {
      if (y != right_seg->y0)
	art_warn ("*** art_svp_intersect_test_cross: intersection (%g, %g) matches former y0 of %lx, %lx\n",
		  x, y, (unsigned long)left_seg, (unsigned long)right_seg);

      /* Intersection takes place at current scan line; process immediately
	 rather than queueing intersection point into priq. */

      /* Choose "most vertical" segement */
      if (left_seg->a > right_seg->a)
	x = left_seg->x[0];
      else
	x = right_seg->x[0];
      /* todo: fudge x */

      /* Swap left_seg and right_seg in active list. */
      right_seg->left = left_seg->left;
      if (right_seg->left != NULL)
	right_seg->left->right = right_seg;
      else
	ctx->active_head = right_seg;
      left_seg->right = right_seg->right;
      if (left_seg->right != NULL)
	left_seg->right->left = left_seg;
      left_seg->left = right_seg;
      right_seg->right = left_seg;

      return ART_TRUE;
    }
  else if (y == right_seg->y0)
    art_warn ("*** art_svp_intersect_test_cross: intersection (%g, %g) matches latter y0 of %lx, %lx\n",
	      x, y, (unsigned long)left_seg, (unsigned long)right_seg);
  else
    {
#ifdef VERBOSE
      printf ("Inserting (%g, %g) into %lx, %lx\n",
	      x, y, (unsigned long)left_seg, (unsigned long)right_seg);
#endif
      /* Insert the intersection point into both segments. */
      art_svp_intersect_push_pt (ctx, left_seg, x, y);
      art_svp_intersect_push_pt (ctx, right_seg, x, y);
    }
  return ART_FALSE;
}

/**
 * art_svp_intersect_active_delete: Delete segment from active list.
 * @ctx: Intersection context.
 * @seg: Segment to delete.
 *
 * Deletes @seg from the active list, and frees it.
 **/
static /* todo inline */ void
art_svp_intersect_active_delete (ArtIntersectCtx *ctx, ArtActiveSeg *seg)
{
  ArtActiveSeg *left = seg->left, *right = seg->right;

  if (left != NULL)
    left->right = right;
  else
    ctx->active_head = right;
  if (right != NULL)
    right->left = left;
  art_free (seg->stack);
#ifdef VERBOSE
  printf ("Freeing %lx\n", seg);
#endif
  art_free (seg);
}

/**
 * art_svp_intersect_add_horiz: Add point to horizontal list.
 * @ctx: Intersector context.
 * @seg: Segment with point to insert into horizontal list.
 *
 * Inserts @seg into horizontal list, keeping it in ascending horiz_x
 * order.
 *
 * Note: the horiz_commit routine processes "clusters" of segs in the
 * horiz list, all sharing the same horiz_x value. The cluster is
 * processed in active list order, rather than horiz list order. Thus,
 * the order of segs in the horiz list sharing the same horiz_x
 * _should_ be irrelevant. Even so, we use b as a secondary sorting key,
 * as a "belt and suspenders" defensive coding tactic.
 **/
static void
art_svp_intersect_add_horiz (ArtIntersectCtx *ctx, ArtActiveSeg *seg)
{
  ArtActiveSeg **pp = &ctx->horiz_last;
  ArtActiveSeg *place;
  ArtActiveSeg *place_right = NULL;

#ifdef VERBOSE
  printf ("add_horiz %lx, x = %g\n", seg, seg->horiz_x);
#endif
  for (place = *pp; place != NULL && (place->horiz_x > seg->horiz_x ||
				      (place->horiz_x == seg->horiz_x &&
				       place->b < seg->b));
       place = *pp)
    {
      place_right = place;
      pp = &place->horiz_left;
    }
  *pp = seg;
  seg->horiz_left = place;
  seg->horiz_right = place_right;
  if (place == NULL)
    ctx->horiz_first = seg;
  else
    place->horiz_right = seg;
}

static void
art_svp_intersect_insert_cross (ArtIntersectCtx *ctx,
				ArtActiveSeg *seg)
{
  ArtActiveSeg *left = seg, *right = seg;

  for (;;)
    {
      if (left != NULL && left->left != NULL)
	{
	  if (art_svp_intersect_test_cross (ctx, left->left, left))
	    {
	      if (left == right || right == NULL)
		right = left->right;
	    }
	  else
	    {
	      left = NULL;
	    }
	}
      else if (right != NULL && right->right != NULL)
	{
	  if (art_svp_intersect_test_cross (ctx, right, right->right))
	    {
	      if (left == right || left == NULL)
		left = right->left;
	    }
	  else
	    {
	      right = NULL;
	    }
	}
      else
	break;
    }
}

static void
art_svp_intersect_process_intersection (ArtIntersectCtx *ctx,
					ArtActiveSeg *seg)
{
  int n_stack = --seg->n_stack;
  seg->x[1] = seg->stack[n_stack - 1].x;
  seg->y1 = seg->stack[n_stack - 1].y;
  seg->x[0] = seg->stack[n_stack].x;
  seg->y0 = seg->stack[n_stack].y;
  seg->horiz_x = seg->x[0];
  art_svp_intersect_add_horiz (ctx, seg);
  art_svp_intersect_insert_cross (ctx, seg);
}

static void
art_svp_intersect_advance_cursor (ArtIntersectCtx *ctx, ArtActiveSeg *seg,
				  ArtPriPoint *pri_pt)
{
  const ArtSVPSeg *in_seg = seg->in_seg;
  int in_curs = seg->in_curs;
  ArtSvpWriter *swr = seg->flags & ART_ACTIVE_FLAGS_OUT ? ctx->out : NULL;

  if (swr != NULL)
    swr->add_point (swr, seg->seg_id, seg->x[1], seg->y1);
  if (in_curs + 1 == in_seg->n_points)
    {
      ArtActiveSeg *left = seg->left, *right = seg->right;

      if (swr != NULL)
	swr->close_segment (swr, seg->seg_id);
      art_svp_intersect_active_delete (ctx, seg);
      if (left != NULL && right != NULL)
	art_svp_intersect_test_cross (ctx, left, right);
    }
  else
    {
      seg->horiz_x = seg->x[1];
      art_svp_intersect_add_horiz (ctx, seg);

      art_svp_intersect_setup_seg (seg, pri_pt);
      art_pri_insert (ctx->pq, pri_pt);
      art_svp_intersect_insert_cross (ctx, seg);
    }
}

/**
 * art_svp_intersect_horiz_commit: Commit points in horiz list to output.
 * @ctx: Intersection context.
 *
 * Commits the points in the horizontal list to the output, and
 * reorders the active list so that, below the current sweep line, it
 * is consistent with the actual geometry.
 *
 * Much of the "heavy lifting" of the intersection algorithm occurs in
 * this routine. The active list can get out of order from ordinary
 * intersection events, and also from numerical fuzz. In the latter case,
 * points are shifted slightly.
 *
 * This "commit" pass is also where winding numbers are assigned,
 * because doing it here provides much greater tolerance for inputs
 * which are not in strict SVP order.
 **/
static void
art_svp_intersect_horiz_commit (ArtIntersectCtx *ctx)
{
  ArtActiveSeg *seg;
  int winding_number;

#ifdef VERBOSE
      printf ("art_svp_intersect_horiz_commit: y=%g\n", ctx->y);
#endif

  /* Output points to svp writer. */
  for (seg = ctx->horiz_first; seg != NULL;)
    {
      /* Find a cluster with common horiz_x, */
      ArtActiveSeg *curs;
      double x = seg->horiz_x;

      /* Beginning of cluster. */
      for (curs = seg; curs->left != NULL; curs = curs->left)
	if (curs->left->horiz_x != x)
	  break;

      if (curs->left != NULL)
	winding_number = curs->left->wind_left + curs->left->delta_wind;
      else
	winding_number = 0;

      do
	{
#ifdef VERBOSE
	  printf (" winding_number = %d += %d\n",
		  winding_number, curs->delta_wind);
#endif
	  if (!(curs->flags & ART_ACTIVE_FLAGS_OUT) ||
	      curs->wind_left != winding_number)
	    {
	      ArtSvpWriter *swr = ctx->out;

	      if (curs->flags & ART_ACTIVE_FLAGS_OUT)
		{
		  swr->add_point (swr, curs->seg_id, curs->horiz_x, ctx->y);
		  swr->close_segment (swr, curs->seg_id);
		}

	      curs->seg_id = swr->add_segment (swr, winding_number,
					       curs->delta_wind,
					       curs->horiz_x, ctx->y);
	      curs->flags |= ART_ACTIVE_FLAGS_OUT;
	    }
	  curs->wind_left = winding_number;
	  winding_number += curs->delta_wind;
	  curs = curs->right;
	}
      while (curs != NULL && curs->horiz_x == x);

      /* Skip past cluster. */
      do
	seg = seg->horiz_right;
      while (seg != NULL && seg->horiz_x == x);
    }
  ctx->horiz_first = NULL;
  ctx->horiz_last = NULL;
}

/**
 * art_svp_intersect_add_point: Add a point, breaking nearby neighbors.
 * @ctx: Intersector context.
 * @x: X coordinate of point to add.
 * @y: Y coordinate of point to add.
 * @seg: "nearby" segment, or NULL if leftmost.
 *
 * Return value: Segment immediately to the left of the new point, or
 * NULL if the new point is leftmost.
 **/
static ArtActiveSeg *
art_svp_intersect_add_point (ArtIntersectCtx *ctx, double x, double y,
			     ArtActiveSeg *seg)
{
  ArtActiveSeg *left, *right;
  double x_min = x, x_max = x;
  art_boolean left_live, right_live;
  double d;
  double new_x;
  ArtActiveSeg *test, *result = NULL;
  double x_test;

  if (seg == NULL)
    {
      left = ctx->active_head;
      if (left == NULL)
	return NULL;
    }
  else
    left = seg;
  right = left->right; 
  left_live = ART_TRUE;
  right_live = (right != NULL);
  while (left_live || right_live)
    {
      if (left_live)
	{
	  if (x <= left->x[left->flags & ART_ACTIVE_FLAGS_BNEG] &&
	      /* It may be that one of these conjuncts turns out to be always
		 true. We test both anyway, to be defensive. */
	      y != left->y0 && y != left->y1)
	    {
	      d = x_min * left->a + y * left->b + left->c;
	      if (d < EPSILON_C)
		{
		  new_x = art_svp_intersect_break (ctx, left, y);
		  if (new_x > x_max)
		    {
		      x_max = new_x;
		      right_live = (right != NULL);
		    }
		  else if (new_x < x_min)
		    x_min = new_x;
		  left = left->left;
		  left_live = (left != NULL);
		}
	      else
		left_live = ART_FALSE;
	    }
	  else
	    left_live = ART_FALSE;
	}
      else if (right_live)
	{
	  if (x <= right->x[(right->flags & ART_ACTIVE_FLAGS_BNEG) ^ 1] &&
	      /* It may be that one of these conjuncts turns out to be always
		 true. We test both anyway, to be defensive. */
	      y != right->y0 && y != right->y1)
	    {
	      d = x_max * right->a + y * right->b + right->c;
	      if (d > -EPSILON_C)
		{
		  new_x = art_svp_intersect_break (ctx, right, y);
		  if (new_x < x_min)
		    {
		      x_min = new_x;
		      left_live = (left != NULL);
		    }
		  else if (new_x >= x_max)
		    x_max = new_x;
		  right = right->right;
		  right_live = (right != NULL);
		}
	      else
		right_live = ART_FALSE;
	    }
	  else
	    right_live = ART_FALSE;
	}
    }

  /* Now, (left, right) defines an interval of segments broken. Sort
     into ascending x order. */
  test = left == NULL ? ctx->active_head : left->right;
  result = left;
  if (test != NULL && test != right)
    {
      x_test = test->x[1];
      for (;;)
	{
	  if (x_test <= x)
	    result = test;
	  test = test->right;
	  if (test == right)
	    break;
	  new_x = test->x[1];
	  if (new_x < x_test)
	    {
	      art_warn ("art_svp_intersect_add_point: non-ascending x\n");
	    }
	  x_test = new_x;
	}
    }
  return result;
}

static void
art_svp_intersect_add_seg (ArtIntersectCtx *ctx, const ArtSVPSeg *in_seg)
{
  ArtActiveSeg *seg = art_new (ArtActiveSeg, 1);
  ArtActiveSeg *test;
  double x0, y0;
  ArtActiveSeg *beg_range;
  ArtActiveSeg *last = NULL;
  ArtActiveSeg *left, *right;
  ArtPriPoint *pri_pt = art_new (ArtPriPoint, 1);

  seg->flags = 0;
  seg->in_seg = in_seg;
  seg->in_curs = 0;

  seg->n_stack_max = 4;
  seg->stack = art_new (ArtPoint, seg->n_stack_max);

  pri_pt->user_data = seg;
  art_svp_intersect_setup_seg (seg, pri_pt);
  art_pri_insert (ctx->pq, pri_pt);

  /* Find insertion place for new segment */
  /* This is currently a left-to-right scan, but should be replaced
     with a binary search as soon as it's validated. */

  x0 = in_seg->points[0].x;
  y0 = in_seg->points[0].y;
  beg_range = NULL;
  for (test = ctx->active_head; test != NULL; test = test->right)
    {
      double d;
      int test_bneg = test->flags & ART_ACTIVE_FLAGS_BNEG;

      if (x0 < test->x[test_bneg])
	{
	  if (x0 < test->x[test_bneg ^ 1])
	    break;
	  d = x0 * test->a + y0 * test->b + test->c;
	  if (d < 0)
	    break;
	}
      last = test;
    }

  left = art_svp_intersect_add_point (ctx, x0, y0, last);
  seg->left = left;
  if (left == NULL)
    {
      right = ctx->active_head;
      ctx->active_head = seg;
    }
  else
    {
      right = left->right;
      left->right = seg;
    }
  seg->right = right;
  if (right != NULL)
    right->left = seg;

  seg->delta_wind = in_seg->dir ? 1 : -1;
  seg->horiz_x = x0;

  art_svp_intersect_add_horiz (ctx, seg);

  while (seg->left != NULL)
    if (!art_svp_intersect_test_cross (ctx, seg->left, seg))
      break;
  while (seg->right != NULL)
    if (!art_svp_intersect_test_cross (ctx, seg, seg->right))
      break;
}

#ifdef VERBOSE
static void
art_svp_intersect_print_active (ArtIntersectCtx *ctx)
{
  ArtActiveSeg *seg;

  printf ("Active list (y = %g):\n", ctx->y);
  for (seg = ctx->active_head; seg != NULL; seg = seg->right)
    {
      printf (" %lx: (%g, %g)-(%g, %g), (a, b, c) = (%g, %g, %g)\n",
	      (unsigned long)seg,
	      seg->x[0], seg->y0, seg->x[1], seg->y1,
	      seg->a, seg->b, seg->c);
    }
}
#endif

#ifdef SANITYCHECK
static void
art_svp_intersect_sanitycheck (ArtIntersectCtx *ctx)
{
  ArtActiveSeg *seg;
  ArtActiveSeg *last = NULL;
  double d;

  for (seg = ctx->active_head; seg != NULL; seg = seg->right)
    {
      if (seg->left != last)
	{
	  art_warn ("*** art_svp_intersect_sanitycheck: last=%lx, seg->left=%lx\n",
		    (unsigned long)last, (unsigned long)seg->left);
	}
      if (last != NULL)
	{
	  /* pairwise compare with previous seg */

	  /* First the top. */
	  if (last->y0 < seg->y0)
	    {
	    }
	  else
	    {
	    }

	  /* Then the bottom. */
	  if (last->y1 < seg->y1)
	    {
	      if (!((last->x[1] <
		     seg->x[(seg->flags & ART_ACTIVE_FLAGS_BNEG) ^ 1]) ||
		    last->y1 == seg->y0))
		{
		  d = last->x[1] * seg->a + last->y1 * seg->b + seg->c;
		  if (d >= -EPSILON_C)
		    art_warn ("*** bottom (%g, %g) of %lx is not clear of %lx to right (d = %g)\n",
			      last->x[1], last->y1, (unsigned long) last,
			      (unsigned long) seg, d);
		}
	    }
	  else if (last->y1 > seg->y1)

	    {
	      if (!((seg->x[1] >
		     last->x[last->flags & ART_ACTIVE_FLAGS_BNEG]) ||
		    seg->y1 == last->y0))
	      {
		d = seg->x[1] * last->a + seg->y1 * last->b + last->c;
		if (d <= EPSILON_C)
		  art_warn ("*** bottom (%g, %g) of %lx is not clear of %lx to left (d = %g)\n",
			      seg->x[1], seg->y1, (unsigned long) seg,
			      (unsigned long) last, d);
	      }
	    }
	  else
	    {
	      if (last->x[1] > seg->x[1])
		art_warn ("*** bottoms (%g, %g) of %lx and (%g, %g) of %lx out of order\n",
			  last->x[1], last->y1, (unsigned long)last,
			  seg->x[1], seg->y1, (unsigned long)seg);
	    }
	}
      last = seg;
    }
}
#endif

void
art_svp_intersector (const ArtSVP *in, ArtSvpWriter *out)
{
  ArtIntersectCtx *ctx = art_new (ArtIntersectCtx, 1);
  ArtPriQ *pq;
  ArtPriPoint *first_point;
#ifdef VERBOSE
  int count = 0;
#endif

  ctx->in = in;
  ctx->out = out;
  pq = art_pri_new ();
  ctx->pq = pq;

  ctx->active_head = NULL;

  ctx->horiz_first = NULL;
  ctx->horiz_last = NULL;

  ctx->in_curs = 0;
  first_point = art_new (ArtPriPoint, 1);
  first_point->x = in->segs[0].points[0].x;
  first_point->y = in->segs[0].points[0].y;
  first_point->user_data = NULL;
  ctx->y = first_point->y;
  art_pri_insert (pq, first_point);

  while (!art_pri_empty (pq))
    {
      ArtPriPoint *pri_point = art_pri_choose (pq);
      ArtActiveSeg *seg = (ArtActiveSeg *)pri_point->user_data;

#ifdef VERBOSE
      printf ("\nIntersector step %d\n", count++);
      art_svp_intersect_print_active (ctx);
      printf ("priq choose (%g, %g) %lx\n", pri_point->x, pri_point->y,
	      (unsigned long)pri_point->user_data);
#endif
#ifdef SANITYCHECK
      art_svp_intersect_sanitycheck(ctx);
#endif

      if (ctx->y != pri_point->y)
	{
	  art_svp_intersect_horiz_commit (ctx);
	  ctx->y = pri_point->y;
	}

      if (seg == NULL)
	{
	  /* Insert new segment from input */
	  const ArtSVPSeg *in_seg = &in->segs[ctx->in_curs++];
	  art_svp_intersect_add_seg (ctx, in_seg);
	  if (ctx->in_curs < in->n_segs)
	    {
	      const ArtSVPSeg *next_seg = &in->segs[ctx->in_curs];
	      pri_point->x = next_seg->points[0].x;
	      pri_point->y = next_seg->points[0].y;
	      /* user_data is already NULL */
	      art_pri_insert (pq, pri_point);
	    }
	  else
	    art_free (pri_point);
	}
      else
	{
	  int n_stack = seg->n_stack;

	  if (n_stack > 1)
	    {
	      art_svp_intersect_process_intersection (ctx, seg);
	      art_free (pri_point);
	    }
	  else
	    {
	      art_svp_intersect_advance_cursor (ctx, seg, pri_point);
	    }
	}
    }

  art_svp_intersect_horiz_commit (ctx);

  art_pri_free (pq);
  art_free (ctx);
}

#endif /* not TEST_PRIQ */

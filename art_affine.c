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

/* Simple manipulations with affine transformations */

#include <math.h>
#include <stdio.h> /* for sprintf */
#include <string.h> /* for strcpy */
#include "art_misc.h"
#include "art_point.h"
#include "art_affine.h"


/* According to a strict interpretation of the libart structure, this
   routine should go into its own module, art_point_affine.  However,
   it's only two lines of code, and it can be argued that it is one of
   the natural basic functions of an affine transformation.
*/

void
art_affine_point (ArtPoint *dst, const ArtPoint *src,
		  const double affine[6])
{
  double x, y;

  x = src->x;
  y = src->y;
  dst->x = x * affine[0] + y * affine[2] + affine[4];
  dst->y = x * affine[1] + y * affine[3] + affine[5];
}

void
art_affine_invert (double dst[6], const double src[6])
{
  double r_det;

  r_det = 1.0 / (src[0] * src[3] - src[1] * src[2]);
  dst[0] = src[3] * r_det;
  dst[1] = -src[1] * r_det;
  dst[2] = -src[2] * r_det;
  dst[3] = src[0] * r_det;
  dst[4] = -src[4] * dst[0] - src[5] * dst[2];
  dst[5] = -src[4] * dst[1] - src[5] * dst[3];
}


#define EPSILON 1e-6

/* It's ridiculous I have to write this myself. This is hardcoded to
   six digits of precision, which is good enough for PostScript.

   The return value is the number of characters (i.e. strlen (str)).
   It is no more than 12. */
static int
art_ftoa (char str[80], double x)
{
  char *p = str;
  int i, j;

  p = str;
  if (fabs (x) < EPSILON / 2)
    {
      strcpy (str, "0");
      return 1;
    }
  if (x < 0)
    {
      *p++ = '-';
      x = -x;
    }
  if ((int)floor ((x + EPSILON / 2) < 1))
    {
      *p++ = '0';
      *p++ = '.';
      i = sprintf (p, "%06d", (int)floor ((x + EPSILON / 2) * 1e6));
      while (i && p[i - 1] == '0')
	i--;
      if (i == 0)
	i--;
      p += i;
    }
  else if (x < 1e6)
    {
      i = sprintf (p, "%d", (int)floor (x + EPSILON / 2));
      p += i;
      if (i < 6)
	{
	  int ix;

	  *p++ = '.';
	  x -= floor (x + EPSILON / 2);
	  for (j = i; j < 6; j++)
	    x *= 10;
	  ix = floor (x + 0.5);

	  for (j = 0; j < i; j++)
	    ix *= 10;

	  /* A cheap hack, this routine can round wrong for fractions
	     near one. */
	  if (ix == 1000000)
	    ix = 999999;

	  sprintf (p, "%06d", ix);
	  i = 6 - i;
	  while (i && p[i - 1] == '0')
	    i--;
	  if (i == 0)
	    i--;
	  p += i;
	}
    }
  else
    p += sprintf (p, "%g", x);

  *p = '\0';
  return p - str;
}

/* Convert affine trasformation to concise PostScript string representation.

   The identity transform is mapped to the null string.
 */

#include <stdlib.h>
void
art_affine_to_string (char str[128], const double src[6])
{
  char tmp[80];
  int i, ix;

#if 0
  for (i = 0; i < 1000; i++)
    {
      double d = rand () * .1 / RAND_MAX;
      art_ftoa (tmp, d);
      printf ("%g %f %s\n", d, d, tmp);
    }
#endif
  if (fabs (src[4]) < EPSILON && fabs (src[5]) < EPSILON)
    {
      /* could be scale or rotate */
      if (fabs (src[1]) < EPSILON && fabs (src[2]) < EPSILON)
	{
	  /* scale */
	  if (fabs (src[0] - 1) < EPSILON && fabs (src[3] - 1) < EPSILON)
	    {
	      /* identity transform */
	      str[0] = '\0';
	      return;
	    }
	  else
	    {
	      sprintf (str, "%g %g scale", src[0], src[3]);
	      return;
	    }
	}
      else
	{
	  /* could be rotate */
	  if (fabs (src[0] - src[3]) < EPSILON &&
	      fabs (src[1] + src[2]) < EPSILON &&
	      (src[0] * src[0] + src[1] * src[1] - 1) < 2 * EPSILON)
	    {
	      double theta;

	      theta = (180 / M_PI) * atan2 (src[1], src[0]);
	      art_ftoa (tmp, theta);
	      sprintf (str, "%s rotate", tmp);
	      return;
	    }
	}
    }
  else
    {
      /* could be translate */
      if (fabs (src[0] - 1) < EPSILON && fabs (src[1]) < EPSILON &&
	  fabs (src[2]) < EPSILON && fabs (src[3] - 1) < EPSILON)
	{
	  sprintf (str, "%g %g translate", src[4], src[5]);
	  return;
	}
    }

  ix = 0;
  str[ix++] = '[';
  str[ix++] = ' ';
  for (i = 0; i < 6; i++)
    {
      ix += art_ftoa (str + ix, src[i]);
      str[ix++] = ' ';
    }
  strcpy (str + ix, "] concat");
}

/* dst is the composition of doing affine src1 then src2. Note that
   the PostScript concat operator multiplies on the left, i.e. if
   "M concat" is equivalent to "CTM = multiply (M, CTM)";

   It is safe to call this function with dst equal to one of the args.
 */
void
art_affine_multiply (double dst[6], const double src1[6], const double src2[6])
{
  double d0, d1, d2, d3, d4, d5;

  d0 = src1[0] * src2[0] + src1[1] * src2[2];
  d1 = src1[0] * src2[1] + src1[1] * src2[3];
  d2 = src1[2] * src2[0] + src1[3] * src2[2];
  d3 = src1[2] * src2[1] + src1[3] * src2[3];
  d4 = src1[4] * src2[0] + src1[5] * src2[2] + src2[4];
  d5 = src1[4] * src2[1] + src1[5] * src2[3] + src2[5];
  dst[0] = d0;
  dst[1] = d1;
  dst[2] = d2;
  dst[3] = d3;
  dst[4] = d4;
  dst[5] = d5;
}

/* set up the identity matrix */
void
art_affine_identity (double dst[6])
{
  dst[0] = 1;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = 1;
  dst[4] = 0;
  dst[5] = 0;
}

/* set up a scaling matrix */
void
art_affine_scale (double dst[6], double sx, double sy)
{
  dst[0] = sx;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = sy;
  dst[4] = 0;
  dst[5] = 0;
}

/* set up a rotation matrix; theta is given in degrees */
void
art_affine_rotate (double dst[6], double theta)
{
  double s, c;

  s = sin (theta * M_PI / 180.0);
  c = cos (theta * M_PI / 180.0);
  dst[0] = c;
  dst[1] = s;
  dst[2] = -s;
  dst[3] = c;
  dst[4] = 0;
  dst[5] = 0;
}

/* set up a translation matrix */
void
art_affine_translate (double dst[6], double tx, double ty)
{
  dst[0] = 1;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = 1;
  dst[4] = tx;
  dst[5] = ty;
}

/* find the affine's "expansion factor", i.e. the scale amount */
double
art_affine_expansion (const double src[6])
{
  double r_det;

  return sqrt (src[0] * src[3] - src[1] * src[2]);
}

/* Determine whether the affine transformation is rectilinear,
   i.e. whether a rectangle aligned to the grid is transformed into
   another rectangle aligned to the grid. */
int
art_affine_rectilinear (const double src[6])
{
  return ((fabs (src[1]) < EPSILON && fabs (src[2]) < EPSILON) ||
	  (fabs (src[0]) < EPSILON && fabs (src[3]) < EPSILON));
}

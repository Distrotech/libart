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

/* Render a sorted vector path into an RGB buffer. */

#include "art_misc.h"

#include "art_svp.h"
#include "art_svp_render_aa.h"
#include "art_rgb.h"
#include "art_rgb_svp.h"

typedef struct _ArtRgbSVPData ArtRgbSVPData;
typedef struct _ArtRgbSVPAlphaData ArtRgbSVPAlphaData;

struct _ArtRgbSVPData {
  art_u32 rgbtab[256];
  art_u8 *buf;
  int rowstride;
  int x0, x1;
};

struct _ArtRgbSVPAlphaData {
  int alphatab[256];
  art_u8 r, g, b, alpha;
  art_u8 *buf;
  int rowstride;
  int x0, x1;
};

static void
art_rgb_svp_callback (void *callback_data, int y,
		       int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtRgbSVPData *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  int running_sum = start;
  art_u32 rgb;
  int x0, x1;
  int k;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  rgb = data->rgbtab[running_sum >> 16];
	  art_rgb_fill_run (linebuf,
			    rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			    run_x1 - x0);
	}

      /* render the steps into tmpbuf */
      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      rgb = data->rgbtab[running_sum >> 16];
	      art_rgb_fill_run (linebuf + (run_x0 - x0) * 3,
				rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
				run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  rgb = data->rgbtab[running_sum >> 16];
	  art_rgb_fill_run (linebuf + (run_x1 - x0) * 3,
			    rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			    x1 - run_x1);
	}
    }
  else
    {
      rgb = data->rgbtab[running_sum >> 16];
      art_rgb_fill_run (linebuf,
			rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			x1 - x0);
    }

  data->buf += data->rowstride;
}

/* Render the vector path into the RGB buffer. */

void
art_rgb_svp_aa (const ArtSVP *svp,
		int x0, int y0, int x1, int y1,
		art_u32 fg_color, art_u32 bg_color,
		art_u8 *buf, int rowstride)
{
  ArtRgbSVPData data;

  int r_fg, g_fg, b_fg;
  int r_bg, g_bg, b_bg;
  int r, g, b;
  int dr, dg, db;
  int i;

  r_fg = fg_color >> 16;
  g_fg = (fg_color >> 8) & 0xff;
  b_fg = fg_color & 0xff;

  r_bg = bg_color >> 16;
  g_bg = (bg_color >> 8) & 0xff;
  b_bg = bg_color & 0xff;

  r = (r_bg << 16) + 0x8000;
  g = (g_bg << 16) + 0x8000;
  b = (b_bg << 16) + 0x8000;
  dr = ((r_fg - r_bg) << 16) / 255;
  dg = ((g_fg - g_bg) << 16) / 255;
  db = ((b_fg - b_bg) << 16) / 255;

  for (i = 0; i < 256; i++)
    {
      data.rgbtab[i] = (r & 0xff0000) | ((g & 0xff0000) >> 8) | (b >> 16);
      r += dr;
      g += dg;
      b += db;
    }
  data.buf = buf;
  data.rowstride = rowstride;
  data.x0 = x0;
  data.x1 = x1;
  art_svp_render_aa (svp, x0, y0, x1, y1, art_rgb_svp_callback, &data);
}

static void
art_rgb_svp_alpha_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtRgbSVPAlphaData *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  int running_sum = start;
  int x0, x1;
  int k;
  art_u8 r, g, b;
  int *alphatab;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  r = data->r;
  g = data->g;
  b = data->b;
  alphatab = data->alphatab;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = running_sum >> 16;
	  if (alpha)
	    art_rgb_run_alpha (linebuf,
			       r, g, b, alphatab[alpha],
			       run_x1 - x0);
	}

      /* render the steps into tmpbuf */
      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = running_sum >> 16;
	      if (alpha)
		art_rgb_run_alpha (linebuf + (run_x0 - x0) * 3,
				   r, g, b, alphatab[alpha],
				   run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = running_sum >> 16;
	  if (alpha)
	    art_rgb_run_alpha (linebuf + (run_x1 - x0) * 3,
			       r, g, b, alphatab[alpha],
			       x1 - run_x1);
	}
    }
  else
    {
      alpha = running_sum >> 16;
      if (alpha)
	art_rgb_run_alpha (linebuf,
			   r, g, b, alphatab[alpha],
			   x1 - x0);
    }

  data->buf += data->rowstride;
}

static void
art_rgb_svp_alpha_opaque_callback (void *callback_data, int y,
				   int start,
				   ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtRgbSVPAlphaData *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  int running_sum = start;
  int x0, x1;
  int k;
  art_u8 r, g, b;
  int *alphatab;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  r = data->r;
  g = data->g;
  b = data->b;
  alphatab = data->alphatab;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = running_sum >> 16;
	  if (alpha)
	    {
	      if (alpha == 255)
		art_rgb_fill_run (linebuf,
				  r, g, b,
				  run_x1 - x0);
	      else
		art_rgb_run_alpha (linebuf,
				   r, g, b, alphatab[alpha],
				   run_x1 - x0);
	    }
	}

      /* render the steps into tmpbuf */
      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = running_sum >> 16;
	      if (alpha)
		{
		  if (alpha == 255)
		    art_rgb_fill_run (linebuf + (run_x0 - x0) * 3,
				      r, g, b,
				      run_x1 - run_x0);
		  else
		    art_rgb_run_alpha (linebuf + (run_x0 - x0) * 3,
				       r, g, b, alphatab[alpha],
				       run_x1 - run_x0);
		}
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = running_sum >> 16;
	  if (alpha)
	    {
	      if (alpha == 255)
		art_rgb_fill_run (linebuf + (run_x1 - x0) * 3,
				  r, g, b,
				  x1 - run_x1);
	      else
		art_rgb_run_alpha (linebuf + (run_x1 - x0) * 3,
				   r, g, b, alphatab[alpha],
				   x1 - run_x1);
	    }
	}
    }
  else
    {
      alpha = running_sum >> 16;
      if (alpha)
	{
	  if (alpha == 255)
	    art_rgb_fill_run (linebuf,
			      r, g, b,
			      x1 - x0);
	  else
	    art_rgb_run_alpha (linebuf,
			       r, g, b, alphatab[alpha],
			       x1 - x0);
	}
    }

  data->buf += data->rowstride;
}

void
art_rgb_svp_alpha (const ArtSVP *svp,
		   int x0, int y0, int x1, int y1,
		   art_u32 rgba,
		   art_u8 *buf, int rowstride)
{
  ArtRgbSVPAlphaData data;
  int r, g, b, alpha;
  int i;
  int a, da;

  r = rgba >> 24;
  g = (rgba >> 16) & 0xff;
  b = (rgba >> 8) & 0xff;
  alpha = rgba & 0xff;

  data.r = r;
  data.g = g;
  data.b = b;
  data.alpha = alpha;

  a = 0x8000;
  da = (alpha * 66051 + 0x80) >> 8; /* 66051 equals 2 ^ 32 / (255 * 255) */

  for (i = 0; i < 256; i++)
    {
      data.alphatab[i] = a >> 16;
      a += da;
    }

  data.buf = buf;
  data.rowstride = rowstride;
  data.x0 = x0;
  data.x1 = x1;
  if (alpha == 255)
    art_svp_render_aa (svp, x0, y0, x1, y1, art_rgb_svp_alpha_opaque_callback,
		       &data);
  else
    art_svp_render_aa (svp, x0, y0, x1, y1, art_rgb_svp_alpha_callback, &data);
}


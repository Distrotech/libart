/* This is a stub and will get replaced with the real code. */

#ifndef __ART_ALPHAGAMMA_H__
#define __ART_ALPHAGAMMA_H__

typedef struct _ArtAlphaGamma ArtAlphaGamma;

struct _ArtAlphaGamma {
  int table[256];
  art_u8 invtable[1];
};

ArtAlphaGamma *
art_alphagamma_new (double gamma);

void
art_alphagamma_free (ArtAlphaGamma *alphagamma);

#endif

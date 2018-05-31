#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include "openslide-private.h"
#include "openslide-zstack.h"
#include "openslide-error.h"

static TIFFErrorHandler oerror;
static TIFFErrorHandler owarning;

static bool zlevel_in_range(openslide_t *osr, int32_t zlevel) {
  if (zlevel < 0) {
    return false;
  } else if (zlevel > osr->zlevel_count - 1) {
    return false;
  }
  return true;
}

static bool valid_level(openslide_t *osr, int32_t zlevel, int32_t level) {
  if (!zlevel_in_range(osr, zlevel)) {
	return false;
  }
  if (level < 0) {
    return false;
  } else if (level > osr->zlevels[zlevel]->level_count - 1) {
    return false;
  }
  return true;
}

static bool ensure_nonnegative_dimensions(openslide_t *osr, int64_t w, int64_t h) {
  if (w < 0 || h < 0) {
    GError *tmp_err = g_error_new(OPENSLIDE_ERROR, OPENSLIDE_ERROR_FAILED,
                                  "negative width (%"PRId64") "
                                  "or negative height (%"PRId64") "
                                  "not allowed", w, h);
    _openslide_propagate_error(osr, tmp_err);
    return false;
  }
  return true;
}

static bool read_region(openslide_t *osr,
			cairo_t *cr,
			int64_t x, int64_t y,
			int32_t zlevel, int32_t level,
			int64_t w, int64_t h,
			GError **err) {
  bool success = true;

  // save the old pattern, it's the only thing push/pop won't restore
  cairo_pattern_t *old_source = cairo_get_source(cr);
  cairo_pattern_reference(old_source);

  // push, so that saturate works with all sorts of backends
  cairo_push_group(cr);

  // clear to set the bounds of the group (seems to be a recent cairo bug)
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_fill(cr);

  // saturate those seams away!
  cairo_set_operator(cr, CAIRO_OPERATOR_SATURATE);

  if (valid_level(osr, zlevel, level)) {
    struct _openslide_level *l = osr->zlevels[zlevel]->levels[level];

    // offset if given negative coordinates
    double ds = l->downsample;
    int64_t tx = 0;
    int64_t ty = 0;
    if (x < 0) {
      tx = (-x) / ds;
      x = 0;
      w -= tx;
    }
    if (y < 0) {
      ty = (-y) / ds;
      y = 0;
      h -= ty;
    }
    cairo_translate(cr, tx, ty);

    // paint
    if (w > 0 && h > 0) {
      success = osr->ops->paint_region(osr, cr, x, y, l, w, h, err);
    }
  }

  cairo_pop_group_to_source(cr);

  if (success) {
    // commit, nothing went wrong
    cairo_paint(cr);
  }

  // restore old source
  cairo_set_source(cr, old_source);
  cairo_pattern_destroy(old_source);

  return success;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

int32_t osz_get_zlevel_count(openslide_t *osr) {
  if (openslide_get_error(osr)) {
    return -1;
  }
  return osr->zlevel_count;
}

void osz_get_zlevel_offset(openslide_t *osr, int32_t zlevel, double *offset) {
  if (openslide_get_error(osr)) {
    return;
  }
  if (!zlevel_in_range(osr, zlevel)) {
	return;
  }
  *offset = osr->zlevels[zlevel]->zoffset;
}

int32_t osz_get_level_count(openslide_t *osr, int32_t zlevel) {
  if (openslide_get_error(osr)) {
    return -1;
  }
  if (!zlevel_in_range(osr, zlevel)) {
	return -1;
  }
  return osr->zlevels[zlevel]->level_count;
}

void osz_get_level0_dimensions(openslide_t *osr, int32_t zlevel, int64_t *w, int64_t *h) {
	osz_get_level_dimensions(osr, zlevel, 0, w, h);
}

void osz_get_level_dimensions(openslide_t *osr, int32_t zlevel, int32_t level, int64_t *w, int64_t *h) {
  *w = -1;
  *h = -1;
  if (openslide_get_error(osr)) {
    return;
  }
  if (!valid_level(osr, zlevel, level)) {
    return;
  }
  *w = osr->zlevels[zlevel]->levels[level]->w;
  *h = osr->zlevels[zlevel]->levels[level]->h;
}

double osz_get_level_downsample(openslide_t *osr, int32_t zlevel, int32_t level) {
  if (openslide_get_error(osr) || !valid_level(osr, zlevel, level)) {
    return -1.0;
  }
  return osr->zlevels[zlevel]->levels[level]->downsample;
}

int32_t osz_get_best_level_for_downsample(openslide_t *osr, int32_t zlevel, double downsample) {
  if (openslide_get_error(osr) || !zlevel_in_range(osr, zlevel)) {
    return -1;
  }

  struct _openslide_zlevel *zlvl = osr->zlevels[zlevel];

  // too small, return first
  if (downsample < zlvl->levels[0]->downsample) {
    return 0;
  }

  // find where we are in the middle
  for (int32_t i = 1; i < zlvl->level_count; i++) {
    if (downsample < zlvl->levels[i]->downsample) {
      return i - 1;
    }
  }

  // too big, return last
  return zlvl->level_count - 1;
}

void osz_read_region(openslide_t *osr, uint32_t *dest, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h) {
  TIFFSetWarningHandler(NULL);
  GError *tmp_err = NULL;

  if (!ensure_nonnegative_dimensions(osr, w, h)) {
    return;
  }

  // clear the dest
  if (dest) {
    memset(dest, 0, w * h * 4);
  }

  // now that it's cleared, return if an error occurred
  if (openslide_get_error(osr)) {
    return;
  }

  // Break the work into smaller pieces if the region is large, because:
  // 1. Cairo will not allow surfaces larger than 32767 pixels on a side.
  // 2. cairo_push_group() creates an intermediate surface backed by a
  //    pixman_image_t, and Pixman requires that every byte of that image
  //    be addressable in 31 bits.
  // 3. We would like to constrain the intermediate surface to a reasonable
  //    amount of RAM.
  const int64_t d = 4096;
  double ds = osz_get_level_downsample(osr, zlevel, level);
  for (int64_t row = 0; row < (h + d - 1) / d; row++) {
    for (int64_t col = 0; col < (w + d - 1) / d; col++) {
      // calculate surface coordinates and size
      int64_t sx = x + col * d * ds;     // level 0 plane
      int64_t sy = y + row * d * ds;     // level 0 plane
      int64_t sw = MIN(w - col * d, d);  // level plane
      int64_t sh = MIN(h - row * d, d);  // level plane

      // create the cairo surface for the dest
      cairo_surface_t *surface;
      if (dest) {
        surface = cairo_image_surface_create_for_data(
                (unsigned char *) (dest + w * row * d + col * d),
                CAIRO_FORMAT_ARGB32, sw, sh, w * 4);
      } else {
        // nil surface
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
      }

      // create the cairo context
      cairo_t *cr = cairo_create(surface);
      cairo_surface_destroy(surface);

      // paint
      if (!read_region(osr, cr, sx, sy, zlevel, level, sw, sh, &tmp_err)) {
        cairo_destroy(cr);
        goto OUT;
      }

      // done
      if (!_openslide_check_cairo_status(cr, &tmp_err)) {
        cairo_destroy(cr);
        goto OUT;
      }

      cairo_destroy(cr);
    }
  }

OUT:
  if (tmp_err) {
    _openslide_propagate_error(osr, tmp_err);
    if (dest) {
      // ensure we don't return a partial result
      memset(dest, 0, w * h * 4);
    }
  }
}

void* osz_get_region(openslide_t *osr, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h) {
    void *dest = malloc(w*h*4);
    osz_read_region(osr, (uint32_t*)dest, zlevel, x, y, level, w, h);
    return dest;
}

void osz_free_region(void* region) {
    free(region);
}


void store_old_error_handlers() {
    if (oerror != NULL) {
        oerror = TIFFSetErrorHandler(NULL);
    }
    if (owarning != NULL) {
        owarning = TIFFSetWarningHandler(NULL);
    }
}

/* 
 * Set the verbosity of libtiff.  
 * 0 = no messages, 1 = only errors, 2 = warnings and errors
 */
void set_tiff_message_verbosity(const int32_t verbosity) {
    store_old_error_handlers();
    TIFFSetWarningHandler(NULL);
    TIFFSetErrorHandler(NULL);

    if (verbosity > 0) {
        TIFFSetErrorHandler(oerror);
    }
    if (verbosity > 1) {
        TIFFSetWarningHandler(owarning);
    }
}

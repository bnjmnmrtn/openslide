/**
 * @file openslide-zstack.h
 * The API with z-stack support for the openslide library
*/

#ifndef OPENSLIDE_OPENSLIDE_ZSTACK_H_
#define OPENSLIDE_OPENSLIDE_ZSTACK_H_

#include <stdint.h>
#include "openslide.h"

#ifdef __cplusplus
extern "C" {
#endif

OPENSLIDE_PUBLIC()
int32_t osz_get_zlevel_count(openslide_t *osr);

OPENSLIDE_PUBLIC()
int32_t osz_get_level_count(openslide_t *osr, int32_t zlevel);

OPENSLIDE_PUBLIC()
void osz_get_zlevel_offset(openslide_t *osr, int32_t zlevel, double *zoffset);

OPENSLIDE_PUBLIC()
void osz_get_level0_dimensions(openslide_t *osr, int32_t zlevel, int64_t *w, int64_t *h);

OPENSLIDE_PUBLIC()
void osz_get_level_dimensions(openslide_t *osr, int32_t zlevel, int32_t level, int64_t *w, int64_t *h);

OPENSLIDE_PUBLIC()
double osz_get_level_downsample(openslide_t *osr, int32_t zlevel, int32_t level);

OPENSLIDE_PUBLIC()
int32_t osz_get_best_level_for_downsample(openslide_t *osr, int32_t zlevel, double downsample);

OPENSLIDE_PUBLIC()
void osz_read_region(openslide_t *osr, uint32_t *dest, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h);

OPENSLIDE_PUBLIC()
void* osz_get_region(openslide_t *osr, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h);

OPENSLIDE_PUBLIC()
void osz_free_region(void* region);

OPENSLIDE_PUBLIC()
const char* osz_process_file(const char* filepath, const char *dest);

OPENSLIDE_PUBLIC()
void set_tiff_message_verbosity(const int32_t verbosity);

/**
 * The name of the property containing the Z-Offset of the image
 *
 */
#define OPENSLIDE_PROPERTY_NAME_OFFSET_Z "openslide.offset-z"
//@}

#ifdef __cplusplus
}
#endif

#endif

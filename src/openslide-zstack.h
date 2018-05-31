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

/**
 * Get the number of zlevels available for a given slide
 *
 * @param osr The OpenSlide object.
 * @return the number of zlevels or -1 if an error occurred.
 */
OPENSLIDE_PUBLIC()
int32_t osz_get_zlevel_count(openslide_t *osr);

/**
 * Get the number of levels in the whole slide image.
 *
 * @param osr The OpenSlide object.
 * @param zlevel The zlevel to get the level count for
 * @return The number of levels, or -1 if an error occurred.
 */
OPENSLIDE_PUBLIC()
int32_t osz_get_level_count(openslide_t *osr, int32_t zlevel);


/**
 * Get the offset given a zlevel
 *
 * @param osr The OpenSlide object
 * @param zlevel The level for which to get the offset for 
 * @param[out] zoffset a pointer to the offset.  If there is an error then this will not be set
 */
OPENSLIDE_PUBLIC()
void osz_get_zlevel_offset(openslide_t *osr, int32_t zlevel, double *zoffset);

/**
 * Get the dimensions of level 0 (the largest level). Exactly
 * equivalent to calling openslide_get_level_dimensions(osr, zlevel, 0, w, h).
 *
 * @param osr The OpenSlide object.
 * @param zlevel The zlevel at which to get the dimensions.
 * @param[out] w The width of the image, or -1 if an error occurred.
 * @param[out] h The height of the image, or -1 if an error occurred.
 */
OPENSLIDE_PUBLIC()
void osz_get_level0_dimensions(openslide_t *osr, int32_t zlevel, int64_t *w, int64_t *h);

/**
 * Get the dimensions of a level.
 *
 * @param osr The OpenSlide object.
 * @param zlevel The desired zlevel
 * @param level The desired level.
 * @param[out] w The width of the image, or -1 if an error occurred
 *               or the level was out of range.
 * @param[out] h The height of the image, or -1 if an error occurred
 *               or the level was out of range.
 */
OPENSLIDE_PUBLIC()
void osz_get_level_dimensions(openslide_t *osr, int32_t zlevel, int32_t level, int64_t *w, int64_t *h);

/**
 * Get the downsampling factor of a given level.
 *
 * @param osr The OpenSlide object.
 * @param zlevel The desired zlevel
 * @param level The desired level.
 * @return The downsampling factor for this level, or -1.0 if an error occurred
 *         or the level was out of range.
 */
OPENSLIDE_PUBLIC()
double osz_get_level_downsample(openslide_t *osr, int32_t zlevel, int32_t level);

/**
 * Get the best level to use for displaying the given downsample.
 *
 * @param osr The OpenSlide object.
 * @param zlevel The desired zlevel
 * @param downsample The downsample factor.
 * @return The level identifier, or -1 if an error occurred.
 */
OPENSLIDE_PUBLIC()
int32_t osz_get_best_level_for_downsample(openslide_t *osr, int32_t zlevel, double downsample);

/**
 * Copy pre-multiplied ARGB data from a whole slide image.
 *
 * This function reads and decompresses a region of a whole slide
 * image into the specified memory location. @p dest must be a valid
 * pointer to enough memory to hold the region, at least (@p w * @p h * 4)
 * bytes in length. If an error occurs or has occurred, then the memory
 * pointed to by @p dest will be cleared.
 *
 * @param osr The OpenSlide object.
 * @param dest The destination buffer for the ARGB data.
 * @param zlevel The desired zlevel
 * @param x The top left x-coordinate, in the level 0 reference frame.
 * @param y The top left y-coordinate, in the level 0 reference frame.
 * @param level The desired level.
 * @param w The width of the region. Must be non-negative.
 * @param h The height of the region. Must be non-negative.
 */
OPENSLIDE_PUBLIC()
void osz_read_region(openslide_t *osr, uint32_t *dest, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h);

/**
 * Gets the region.  This is similar to read_region except for the fact that
 * it allocates the memory.
 *
 * @param osr The OpenSlide object
 * @param zlevel The desired zlevel
 * @param x The top left x-coordinate, in the level 0 reference frame.
 * @param y The top left y-coordinate, in the level 0 reference frame.
 * @param level The desired level.
 * @param w The width of the region. Must be non-negative.
 * @param h The height of the region. Must be non-negative.
 * @return A void* which represents a uint32_t* buffer with the appropriate size of an RGBA premultiplied section
 */
OPENSLIDE_PUBLIC()
void* osz_get_region(openslide_t *osr, int32_t zlevel, int64_t x, int64_t y, int32_t level, int64_t w, int64_t h);

/**
 * Releases the region produced by osz_read_region
 *
 * @param region The region created by osz_get_region
 */
OPENSLIDE_PUBLIC()
void osz_free_region(void* region);

/**
 * Set the verbosity of libtiff for this process
 *
 * @param verbosity The verbosity according to the libtiff library
 */
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

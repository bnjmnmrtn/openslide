/**
 * @file openslide-zstack-private.h
 * Internal tools
*/

#ifndef OPENSLIDE_OPENSLIDE_ZSTACK_PRIVATE_H_
#define OPENSLIDE_OPENSLIDE_ZSTACK_PRIVATE_H_

#include "openslide-private.h"

#ifdef __cplusplus
extern "C" {
#endif

struct zlevel_generator {
	GPtrArray *zlevel_array;
};

struct zlevel_generator* build_generator(void);
void destroy_generator(struct zlevel_generator *g, bool destroy_levels);

void register_level(struct zlevel_generator *g, TIFF *tiff, struct _openslide_level *l);
void generate_zlevels(struct zlevel_generator *g, openslide_t *osr);

#ifdef __cplusplus
}
#endif

#endif

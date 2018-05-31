#include "openslide-zstack-private.h"
#include <math.h>

struct zlevel {
	struct _openslide_zlevel base;
	GPtrArray *level_array;
};

static int zoffset_compare(gconstpointer a, gconstpointer b) {
	const struct zlevel *la = *(const struct zlevel **) a;
	const struct zlevel *lb = *(const struct zlevel **) b;

	if (la->base.zoffset < lb->base.zoffset) {
		return -1;	
	} else if (la->base.zoffset == lb->base.zoffset) {
		return 0;
	} else {
		return 1;
	}
}

static struct zlevel* find_zlevel(GPtrArray *array, double zoffset) {
	for (guint i = 0; i < array->len; i++) {
		struct zlevel *lvl = array->pdata[i];
		if (zoffset == lvl->base.zoffset) {
			return lvl;
		}
	}
	return NULL;
}

struct zlevel_generator* build_generator() {
	struct zlevel_generator *g = g_slice_new0(struct zlevel_generator);
  	g->zlevel_array = g_ptr_array_new();
	return g;
}

void destroy_generator(struct zlevel_generator *g, bool destroy_levels) {
	if (g->zlevel_array) {
		if (destroy_levels) {
			for (uint32_t n = 0; n < g->zlevel_array->len; n++) {
				struct _openslide_zlevel *l = g->zlevel_array->pdata[n];
				g_slice_free(struct _openslide_zlevel, l);
			}
			g_ptr_array_free(g->zlevel_array, true);
		}
		g_ptr_array_free(g->zlevel_array, true);
	}
	g_slice_free(struct zlevel_generator, g);
}

void register_level(struct zlevel_generator *g, TIFF *tiff, struct _openslide_level *level) {
	double zoffset = 0.0;

	// read the Z Offset level
	char *image_desc;
	if (TIFFGetField(tiff, TIFFTAG_IMAGEDESCRIPTION, &image_desc)) {
		char **props = g_strsplit(image_desc, "|", -1);
		int propIdx = 0;
		while (props[propIdx] != NULL) {
			if (sscanf(props[propIdx], "OffsetZ = %lf", &zoffset) == 1) {
				break;
			}
			propIdx += 1;
		}
		g_strfreev(props);
	}

	// round the z-offset in case of precision issues
	zoffset = floorf(zoffset * 10000) / 10000.0;

	// get or create level
	struct zlevel *zl = find_zlevel(g->zlevel_array, zoffset);
	if (zl == NULL) {
		zl = g_slice_new0(struct zlevel);
		zl->level_array = g_ptr_array_new();
		zl->base.zoffset = zoffset;
		g_ptr_array_add(g->zlevel_array, zl);
	}

	// add the level to the zlevel
	g_ptr_array_add(zl->level_array, level);
}


void generate_zlevels(struct zlevel_generator *g, openslide_t *osr) {
	g_ptr_array_sort(g->zlevel_array, zoffset_compare);
  
	// unwrap zlevel array
	int32_t zlevel_count = g->zlevel_array->len;
	struct zlevel **zlevels = (struct zlevel **) g_ptr_array_free(g->zlevel_array, false);
	g->zlevel_array = NULL;

	// populate the openslide z levels
	for (int32_t i = 0; i < zlevel_count; i++) {
		zlevels[i]->base.level_count = zlevels[i]->level_array->len;
		zlevels[i]->base.levels = (struct _openslide_level **) g_ptr_array_free(zlevels[i]->level_array, false);
	}

	osr->zlevels = g_slice_alloc(sizeof(void*) * zlevel_count);
	for (int32_t i = 0; i < zlevel_count; i++) {
		osr->zlevels[i] = &zlevels[i]->base;
	}

	osr->zlevel_count = zlevel_count;
      
	destroy_generator(g, false);
}


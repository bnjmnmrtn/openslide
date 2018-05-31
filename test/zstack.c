#define _GNU_SOURCE

#include "config.h"
#include "openslide.h"
#include "openslide-zstack.h"
#include "openslide-common.h"

#ifdef HAVE_VALGRIND
#include <callgrind.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/time.h>

#include <glib.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include <math.h>


static void print_downsamples(openslide_t *osr) {
	int32_t zlevel_count = osz_get_zlevel_count(osr);
  for (int32_t z = 0; z < zlevel_count; z++) {
		int32_t level_count = osz_get_level_count(osr, z);
		for (int32_t level = 0; level < level_count; level++) {
			printf("z %d: level %d: downsample: %g\n", z, level, osz_get_level_downsample(osr, z, level));
		}
		printf("\n");
	}
}

static void test_next_biggest(openslide_t *osr, double downsample) {
	int32_t z = 0;
	int32_t level = osz_get_best_level_for_downsample(osr, z, downsample);
	printf("zlevel(%d) level for downsample %g: %d (%g)\n", z, downsample, level, openslide_get_level_downsample(osr, level));
}

static uint8_t apply_alpha(uint8_t s, uint8_t a, uint8_t d) {
  double ss = s / 255.0;
  double aa = a / 255.0;
  double dd = d / 255.0;
  return round((ss + (1 - aa) * dd) * 255.0);
}

static void write_as_ppm(const char *filename,
			 int64_t w, int64_t h, uint32_t *buf,
			 uint8_t br, uint8_t bg, uint8_t bb) {
  FILE *f = fopen(filename, "wb");
  if (f == NULL) {
    perror("Cannot open file");
    return;
  }

  fprintf(f, "P6\n%"PRId64" %"PRId64"\n255\n", w, h);
  for (int64_t i = 0; i < w * h; i++) {
    uint32_t val = buf[i];
    uint8_t a = (val >> 24) & 0xFF;
    uint8_t r = (val >> 16) & 0xFF;
    uint8_t g = (val >> 8) & 0xFF;
    uint8_t b = (val >> 0) & 0xFF;

    // composite against background with OVER
    r = apply_alpha(r, a, br);
    g = apply_alpha(g, a, bg);
    b = apply_alpha(b, a, bb);

    putc(r, f);
    putc(g, f);
    putc(b, f);
  }

  fclose(f);
}

static void test_image_fetch(openslide_t *osr,
			     const char *name,
			     int64_t x, int64_t y,
			     int64_t w, int64_t h,
			     bool skip_write) {
  char *filename;

  uint8_t bg_r = 0xFF;
  uint8_t bg_g = 0xFF;
  uint8_t bg_b = 0xFF;

  const char *bgcolor = openslide_get_property_value(osr, OPENSLIDE_PROPERTY_NAME_BACKGROUND_COLOR);
  if (bgcolor) {
    uint64_t bg = g_ascii_strtoull(bgcolor, NULL, 16);
    bg_r = (bg >> 16) & 0xFF;
    bg_g = (bg >> 8) & 0xFF;
    bg_b = bg & 0xFF;
    printf("background: (%d, %d, %d)\n", bg_r, bg_g, bg_b);
  }

  printf("test image fetch %s\n", name);
  //  for (int32_t level = 0; level < 1; level++) {
  for (int32_t level = 0; level < openslide_get_level_count(osr); level++) {
    filename = g_strdup_printf("%s-%.2d.ppm", name, level);
    int64_t num_bytes = w * h * 4;
    printf("Going to allocate %"PRId64" bytes...\n", num_bytes);
    uint32_t *buf = malloc(num_bytes);

    printf("x: %"PRId64", y: %"PRId64", level: %d, w: %"PRId64", h: %"PRId64"\n", x, y, level, w, h);
    openslide_read_region(osr, buf, x, y, level, w, h);

    // write as PPM
    if (!skip_write) {
      write_as_ppm(filename, w, h, buf, bg_r, bg_g, bg_b);
    }

    free(buf);
    g_free(filename);
  }
}

int main(int argc, char **argv) {
  
  set_tiff_message_verbosity(2);

  common_fix_argv(&argc, &argv);
  if (argc != 2) {
    printf("give file!\n");
    return 1;
  }

  printf("version: %s\n", openslide_get_version());

  printf("openslide_detect_vendor returns %s\n", openslide_detect_vendor(argv[1]));
  openslide_t *osr = openslide_open(argv[1]);

  if (osr == NULL || openslide_get_error(osr) != NULL) {
    printf("oh no\n");
    exit(1);
  }

  printf("------------------------------------------\n");

  int32_t zlevels = osz_get_zlevel_count(osr);
  for (int32_t z = 0; z < zlevels; z++) {
		double zoffset;
		osz_get_zlevel_offset(osr, z, &zoffset);
		printf("-- Z-Level %d (%f) --\n", z, zoffset);
		
		int32_t levels = osz_get_level_count(osr, z);
		printf("layers: %d\n", levels);

    for (int32_t i = 0; i < levels; i++) {
      int64_t ww, hh;
      osz_get_level_dimensions(osr, z, i, &ww, &hh);
      printf("zstack %d (%f) level %d dimensions: %"PRId64" x %"PRId64"\n", z, zoffset, i, ww, hh);
    }
		printf("-- -- --\n");
  }

  print_downsamples(osr);

  test_next_biggest(osr, 0.8);
  test_next_biggest(osr, 1.0);
  test_next_biggest(osr, 1.5);
  test_next_biggest(osr, 2.0);
  test_next_biggest(osr, 3.0);
  test_next_biggest(osr, 3.1);
  test_next_biggest(osr, 10);
  test_next_biggest(osr, 20);
  test_next_biggest(osr, 25);
  test_next_biggest(osr, 100);
  test_next_biggest(osr, 1000);
  test_next_biggest(osr, 10000);

  //  int64_t elapsed;

  // test NULL dest
  osz_read_region(osr, NULL, 0, 0, 0, 0, 1000, 1000);

  // test empty dest
  uint32_t* item = 0;
  osz_read_region(osr, item, 0, 0, 0, 0, 0, 0);

  // read properties
  const char * const *property_names = openslide_get_property_names(osr);
  while (*property_names) {
    const char *name = *property_names;
    const char *value = openslide_get_property_value(osr, name);
    printf("property: %s -> %s\n", name, value);

    property_names++;
  }

  // read associated images
  const char * const *associated_image_names = openslide_get_associated_image_names(osr);
  while (*associated_image_names) {
    int64_t w;
    int64_t h;
    const char *name = *associated_image_names;
    openslide_get_associated_image_dimensions(osr, name, &w, &h);

    printf("associated image: %s -> (%"PRId64"x%"PRId64")\n", name, w, h);

    uint32_t *buf = g_new(uint32_t, w * h);
    openslide_read_associated_image(osr, name, buf);
    g_free(buf);

    associated_image_names++;
  }

#ifdef HAVE_VALGRIND
  CALLGRIND_START_INSTRUMENTATION;
#endif

  bool skip = true;

  test_image_fetch(osr, "test7", 0, 0, 200, 200, skip);

  // active region
  const char *bounds_x = openslide_get_property_value(osr, OPENSLIDE_PROPERTY_NAME_BOUNDS_X);
  const char *bounds_y = openslide_get_property_value(osr, OPENSLIDE_PROPERTY_NAME_BOUNDS_Y);
  if (bounds_x && bounds_y) {
    int64_t x = g_ascii_strtoll(bounds_x, NULL, 10);
    int64_t y = g_ascii_strtoll(bounds_y, NULL, 10);
    test_image_fetch(osr, "test8", x, y, 200, 200, skip);
  }

#ifdef HAVE_VALGRIND
  CALLGRIND_STOP_INSTRUMENTATION;
#endif

  openslide_close(osr);

  return 0;
}

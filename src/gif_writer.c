/* This file is a part of libgif
 *
 * Copyright 2019, Jeffery Stager
 *
 * libgif is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libgif is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libgif.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gif.h"
#include <darray.h>
#include <lzw.h>
#include <stdio.h>
#include <stdlib.h>

#define WRITE2BYTES(dst, val) ((dst)[0] = val & 0xff, \
                               (dst)[1] = (val >> 8) & 0xff);

static unsigned char DEFAULT_PALETTE[] = {
    0,   0,   0,
    0,   0,  64,
    0,   0, 128,
    0,   0, 192,
    0,  32,   0,
    0,  32,  64,
    0,  32, 128,
    0,  32, 192,
    0,  64,   0,
    0,  64,  64,
    0,  64, 128,
    0,  64, 192,
    0,  96,   0,
    0,  96,  64,
    0,  96, 128,
    0,  96, 192,
    0, 128,   0,
    0, 128,  64,
    0, 128, 128,
    0, 128, 192,
    0, 160,   0,
    0, 160,  64,
    0, 160, 128,
    0, 160, 192,
    0, 192,   0,
    0, 192,  64,
    0, 192, 128,
    0, 192, 192,
    0, 224,   0,
    0, 224,  64,
    0, 224, 128,
    0, 224, 192,
   32,   0,   0,
   32,   0,  64,
   32,   0, 128,
   32,   0, 192,
   32,  32,   0,
   32,  32,  64,
   32,  32, 128,
   32,  32, 192,
   32,  64,   0,
   32,  64,  64,
   32,  64, 128,
   32,  64, 192,
   32,  96,   0,
   32,  96,  64,
   32,  96, 128,
   32,  96, 192,
   32, 128,   0,
   32, 128,  64,
   32, 128, 128,
   32, 128, 192,
   32, 160,   0,
   32, 160,  64,
   32, 160, 128,
   32, 160, 192,
   32, 192,   0,
   32, 192,  64,
   32, 192, 128,
   32, 192, 192,
   32, 224,   0,
   32, 224,  64,
   32, 224, 128,
   32, 224, 192,
   64,   0,   0,
   64,   0,  64,
   64,   0, 128,
   64,   0, 192,
   64,  32,   0,
   64,  32,  64,
   64,  32, 128,
   64,  32, 192,
   64,  64,   0,
   64,  64,  64,
   64,  64, 128,
   64,  64, 192,
   64,  96,   0,
   64,  96,  64,
   64,  96, 128,
   64,  96, 192,
   64, 128,   0,
   64, 128,  64,
   64, 128, 128,
   64, 128, 192,
   64, 160,   0,
   64, 160,  64,
   64, 160, 128,
   64, 160, 192,
   64, 192,   0,
   64, 192,  64,
   64, 192, 128,
   64, 192, 192,
   64, 224,   0,
   64, 224,  64,
   64, 224, 128,
   64, 224, 192,
   96,   0,   0,
   96,   0,  64,
   96,   0, 128,
   96,   0, 192,
   96,  32,   0,
   96,  32,  64,
   96,  32, 128,
   96,  32, 192,
   96,  64,   0,
   96,  64,  64,
   96,  64, 128,
   96,  64, 192,
   96,  96,   0,
   96,  96,  64,
   96,  96, 128,
   96,  96, 192,
   96, 128,   0,
   96, 128,  64,
   96, 128, 128,
   96, 128, 192,
   96, 160,   0,
   96, 160,  64,
   96, 160, 128,
   96, 160, 192,
   96, 192,   0,
   96, 192,  64,
   96, 192, 128,
   96, 192, 192,
   96, 224,   0,
   96, 224,  64,
   96, 224, 128,
   96, 224, 192,
  128,   0,   0,
  128,   0,  64,
  128,   0, 128,
  128,   0, 192,
  128,  32,   0,
  128,  32,  64,
  128,  32, 128,
  128,  32, 192,
  128,  64,   0,
  128,  64,  64,
  128,  64, 128,
  128,  64, 192,
  128,  96,   0,
  128,  96,  64,
  128,  96, 128,
  128,  96, 192,
  128, 128,   0,
  128, 128,  64,
  128, 128, 128,
  128, 128, 192,
  128, 160,   0,
  128, 160,  64,
  128, 160, 128,
  128, 160, 192,
  128, 192,   0,
  128, 192,  64,
  128, 192, 128,
  128, 192, 192,
  128, 224,   0,
  128, 224,  64,
  128, 224, 128,
  128, 224, 192,
  160,   0,   0,
  160,   0,  64,
  160,   0, 128,
  160,   0, 192,
  160,  32,   0,
  160,  32,  64,
  160,  32, 128,
  160,  32, 192,
  160,  64,   0,
  160,  64,  64,
  160,  64, 128,
  160,  64, 192,
  160,  96,   0,
  160,  96,  64,
  160,  96, 128,
  160,  96, 192,
  160, 128,   0,
  160, 128,  64,
  160, 128, 128,
  160, 128, 192,
  160, 160,   0,
  160, 160,  64,
  160, 160, 128,
  160, 160, 192,
  160, 192,   0,
  160, 192,  64,
  160, 192, 128,
  160, 192, 192,
  160, 224,   0,
  160, 224,  64,
  160, 224, 128,
  160, 224, 192,
  192,   0,   0,
  192,   0,  64,
  192,   0, 128,
  192,   0, 192,
  192,  32,   0,
  192,  32,  64,
  192,  32, 128,
  192,  32, 192,
  192,  64,   0,
  192,  64,  64,
  192,  64, 128,
  192,  64, 192,
  192,  96,   0,
  192,  96,  64,
  192,  96, 128,
  192,  96, 192,
  192, 128,   0,
  192, 128,  64,
  192, 128, 128,
  192, 128, 192,
  192, 160,   0,
  192, 160,  64,
  192, 160, 128,
  192, 160, 192,
  192, 192,   0,
  192, 192,  64,
  192, 192, 128,
  192, 192, 192,
  192, 224,   0,
  192, 224,  64,
  192, 224, 128,
  192, 224, 192,
  224,   0,   0,
  224,   0,  64,
  224,   0, 128,
  224,   0, 192,
  224,  32,   0,
  224,  32,  64,
  224,  32, 128,
  224,  32, 192,
  224,  64,   0,
  224,  64,  64,
  224,  64, 128,
  224,  64, 192,
  224,  96,   0,
  224,  96,  64,
  224,  96, 128,
  224,  96, 192,
  224, 128,   0,
  224, 128,  64,
  224, 128, 128,
  224, 128, 192,
  224, 160,   0,
  224, 160,  64,
  224, 160, 128,
  224, 160, 192,
  224, 192,   0,
  224, 192,  64,
  224, 192, 128,
  224, 192, 192,
  224, 224,   0,
  224, 224,  64,
  224, 224, 128,
  224, 224, 192
};

static void write_bytes(struct gif_writer *g,
                        size_t len,
                        unsigned char *bytes) {
  if (g->meta.dst_type == GIF_FILE) {
    fwrite(bytes, len, 1, g->meta.dst.file);
  } else {
    dapushn(g->meta.dst.ptr, len, bytes);
  }
}

static void write_byte(struct gif_writer *g, unsigned char byte) {
  write_bytes(g, 1, &byte);
}

static void header(struct gif_writer *g) {
#define GIF_HEADER_SIZE 6

  unsigned char version[] = { 'G', 'I', 'F', '8', '9', 'a' };

  write_bytes(g, GIF_HEADER_SIZE, version);

#undef GIF_HEADER_SIZE
}

static void logical_screen(struct gif_writer *g) {
#define LOGICAL_SCREEN_DESCRIPTOR_SIZE 7
#define GLOBAL_COLOR_TABLE_FLAG 0x80
#define GLOBAL_COLOR_TABLE_SIZE 0x07

  unsigned char lsd[LOGICAL_SCREEN_DESCRIPTOR_SIZE];

  WRITE2BYTES(lsd, g->width);
  WRITE2BYTES(lsd + 2, g->height);
  /* color table flag */
  lsd[4] = GLOBAL_COLOR_TABLE_FLAG;
  /* color table size */
  lsd[4] |= (unsigned char) (g->code_size - 1);
  /* background color index */
  lsd[5] = 0;
  /* aspect */
  lsd[6] = 0;
  write_bytes(g, LOGICAL_SCREEN_DESCRIPTOR_SIZE, lsd);
  /* write the palette: n_colors * 3 bytes (1 byte per channel RGB) */
  write_bytes(g, g->n_colors * 3u, g->palette);

#undef LOGICAL_SCREEN_DESCRIPTOR_SIZE
#undef GLOBAL_COLOR_TABLE_FLAG
#undef GLOBAL_COLOR_TABLE_SIZE
}

static void netscape_loop(struct gif_writer *g) {
#define APPLICATION_HEADER_SIZE 12

  unsigned char header[APPLICATION_HEADER_SIZE];

  write_byte(g, TAG_GRAPHIC_EXTENSION);
  write_byte(g, TAG_APPLICATION_LABEL);
  /* block size */
  header[0] = APPLICATION_HEADER_SIZE - 1;
  sprintf((char *) (header + 1), "NETSCAPE");
  header[9] = '2';
  header[10] = '.';
  header[11] = '0';
  write_bytes(g, APPLICATION_HEADER_SIZE, header);
  /* subblock size */
  write_byte(g, 3);
  /* subblock id */
  write_byte(g, 1);
  /* 2-byte loop count (0 = infinite) */
  write_byte(g, 0);
  write_byte(g, 0);
  /* terminator */
  write_byte(g, 0);

#undef APPLICATION_HEADER_SIZE
}

static void graphic_control(struct gif_writer *g, struct gif_opts *opts) {
#define GRAPHIC_CONTROL_HEADER_SIZE 5

  unsigned char header[GRAPHIC_CONTROL_HEADER_SIZE];

  write_byte(g, TAG_GRAPHIC_EXTENSION);
  write_byte(g, TAG_GRAPHIC_CONTROL_LABEL);
  /* block size */
  header[0] = GRAPHIC_CONTROL_HEADER_SIZE - 1;
  header[1] = opts->flags;
  WRITE2BYTES(header + 2, opts->delay);
  header[4] = opts->trans_index;
  write_bytes(g, GRAPHIC_CONTROL_HEADER_SIZE, header);
  /* terminator */
  write_byte(g, 0);

#undef GRAPHIC_CONTROL_HEADER_SIZE
}

static void image_descriptor(struct gif_writer *g,
                             unsigned int left,
                             unsigned int top,
                             unsigned int width,
                             unsigned int height) {
#define IMAGE_DESCRIPTOR_SIZE 9

  unsigned char header[IMAGE_DESCRIPTOR_SIZE];

  write_byte(g, TAG_IMAGE_DESCRIPTOR);
  WRITE2BYTES(header + 0, left);
  WRITE2BYTES(header + 2, top);
  WRITE2BYTES(header + 4, width);
  WRITE2BYTES(header + 6, height);
  /* flags */
  header[8] = 0;
  write_bytes(g, IMAGE_DESCRIPTOR_SIZE, header);

#undef IMAGE_DESCRIPTOR_SIZE
}

static unsigned char calc_color(struct gif_writer *gw,
                                unsigned char r,
                                unsigned char g,
                                unsigned char b) {
  unsigned int i;
  struct {
    unsigned long min;
    unsigned char index;
  } result;

  /* max out the value */
  result.min = ~0UL;
  result.index = 0;
  /* cycle through all colors in the palette */
  for (i = 0; i < gw->n_colors; ++i) {
    /* palette RGB */
    unsigned char pr, pg, pb;
    unsigned int index, delta;

    index = 3 * i;
    pr = gw->palette[index + 0];
    pg = gw->palette[index + 1];
    pb = gw->palette[index + 2];
    delta = (unsigned int) ((pr - r) * (pr - r));
    delta += (unsigned int) ((pg - g) * (pg - g));
    delta += (unsigned int) ((pb - b) * (pb - b));
    if (delta < result.min) {
      result.min = delta;
      result.index = (unsigned char) i;
    }
  }
  return result.index;
}

static int write_image(struct gif_writer *g,
                       unsigned int width,
                       unsigned int height,
                       unsigned char *img) {
  unsigned char *indexed_img, *compr, *tmp;
  unsigned int i, j;
  size_t len;

  indexed_img = malloc(width * height);
  if (!indexed_img) return -1;
  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      unsigned char red, green, blue;
      size_t index;

      index = (i * width) + j;
      red = img[(3 * index) + 0];
      green = img[(3 * index) + 1];
      blue = img[(3 * index) + 2];
      indexed_img[(i * width) + j] = calc_color(g, red, green, blue);
    }
  }
  lzw_compress_gif(g->code_size, width * height, indexed_img, &len, &compr);
  tmp = compr;
  write_byte(g, g->code_size);
  while (len > 255) {
    write_byte(g, 255);
    write_bytes(g, 255, tmp);
    tmp += 255;
    len -= 255;
  }
  write_byte(g, (unsigned char) len);
  write_bytes(g, len, tmp);
  write_byte(g, 0);
  free(compr);
  free(indexed_img);
  return 0;
}

/* **************************************** */
/* Public */
/* **************************************** */

int gifw_init(struct gif_writer *g,
              unsigned char code_size,
              unsigned char *palette,
              unsigned int width,
              unsigned int height) {
  if (!g) return -1;
  g->width = width;
  g->height = height;
  if (palette) {
    g->code_size = code_size;
    g->n_colors = 1u << g->code_size;
    g->palette = palette;
  } else {
    g->code_size = 8;
    g->n_colors = 256;
    g->palette = DEFAULT_PALETTE;
  }
  g->meta.dst_type = GIF_BUFFER;
  g->meta.dst.ptr = danew(g->width * g->height);
  if (!g->meta.dst.ptr) return -1;
  header(g);
  logical_screen(g);
  netscape_loop(g);
  return 0;
}

void gifw_push(struct gif_writer *g,
               struct gif_opts *opts,
               unsigned int left,
               unsigned int top,
               unsigned int width,
               unsigned int height,
               unsigned char *img) {
  if (!g) return;
  if (opts) graphic_control(g, opts);
  image_descriptor(g, left, top, width, height);
  write_image(g, width, height, img);
  return;

#undef CODE_SIZE_DEFAULT
}

void gifw_end(struct gif_writer *g,
              size_t *out_len,
              unsigned char **out_img) {
  if (!g) return;
  write_byte(g, TAG_TRAILER);
  *out_len = dalen(g->meta.dst.ptr);
  *out_img = dapeel(g->meta.dst.ptr);
}

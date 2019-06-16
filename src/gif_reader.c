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
#include <lzw.h>
#include <stdlib.h>
#include <string.h>

#define READ2BYTES(dst, src) dst = ((((src)[1] << 8) | ((src)[0])) & 0xffff)

#define GRAPHIC_CONTROL_HEADER_SIZE 6
#define APPLICATION_HEADER_SIZE 12
#define PLAIN_TEXT_HEADER_SIZE 13
#define IMAGE_DESCRIPTOR_HEADER_SIZE 9

static void advance_read(struct gif_reader *g, unsigned long size, void *dst) {
  if (g->meta.src_type == GIF_FILE) {
    fread(dst, size, 1, g->meta.src.file);
  } else {
    memcpy(dst, g->meta.src.ptr, size);
    g->meta.src.ptr += size;
  }
}

static void advance(struct gif_reader *g, unsigned long size) {
  if (g->meta.src_type == GIF_FILE) {
    fseek(g->meta.src.file, (long) size, SEEK_CUR);
  } else {
    g->meta.src.ptr += size;
  }
}

static void skip_data(struct gif_reader *g) {
  unsigned char size;

  do {
    advance_read(g, 1, &size);
    advance(g, size);
  } while (size);
}

static int header(struct gif_reader *g) {
#define GIF_HEADER_SIZE 6

  unsigned char signature[GIF_HEADER_SIZE + 1];
  int set_flag = 0;

  advance_read(g, GIF_HEADER_SIZE, signature);
  signature[GIF_HEADER_SIZE] = '\0';
  if (!strcmp((char *) signature, "GIF87a")) {
    set_flag = 1;
    g->version = GIF_87a;
  }
  if (!strcmp((char *) signature, "GIF89a")) {
    set_flag = 1;
    g->version = GIF_89a;
  }
  if (!set_flag) return -1;
  return 0;

#undef GIF_HEADER_SIZE
}

static int logical_screen(struct gif_reader *g) {
#define LOGICAL_SCREEN_DESCRIPTOR_SIZE 7
#define GLOBAL_COLOR_TABLE_FLAG 0x80
#define GLOBAL_COLOR_TABLE_SIZE 0x07

  unsigned char flags, lsd[LOGICAL_SCREEN_DESCRIPTOR_SIZE];

  advance_read(g, LOGICAL_SCREEN_DESCRIPTOR_SIZE, lsd);
  READ2BYTES(g->width, lsd);
  READ2BYTES(g->height, lsd + 2);
  flags = lsd[4];
  g->bg_color_index = lsd[5];
  g->aspect = lsd[6];
  if (flags & GLOBAL_COLOR_TABLE_FLAG) {
    unsigned int n_colors, size;

    n_colors = 1 << ((flags & GLOBAL_COLOR_TABLE_SIZE) + 1);
    size = 3 * n_colors;
    g->n_colors = n_colors;
    advance_read(g, size, g->global_clut);
    g->has_global_clut = 1;
  }
  return 0;

#undef LOGICAL_SCREEN_DESCRIPTOR_SIZE
#undef GLOBAL_COLOR_TABLE_FLAG
#undef GLOBAL_COLOR_TABLE_SIZE
}

static void dispose_null(struct gif_reader *g) {
  return;
}

static void dispose_no_disposal(struct gif_reader *g) {
  return;
}

static void dispose_bg_color(struct gif_reader *g) {
  return;
}

static void dispose_previous(struct gif_reader *g) {
  return;
}

static void (*choose_disposal(unsigned int disposal))(struct gif_reader *g) {
  switch (disposal) {
    case 0: return dispose_null;
    case 1: return dispose_no_disposal;
    case 2: return dispose_bg_color;
    case 3: return dispose_previous;
    default: break;
  }
  return dispose_null;
}

static void skip_graphic_control(struct gif_reader *g) {
  advance(g, GRAPHIC_CONTROL_HEADER_SIZE);
}

static void skip_comment(struct gif_reader *g) {
  /* comment extension has no header data */
  skip_data(g);
}

static void skip_application(struct gif_reader *g) {
  advance(g, APPLICATION_HEADER_SIZE);
  skip_data(g);
}

static void skip_plain_text(struct gif_reader *g) {
  advance(g, PLAIN_TEXT_HEADER_SIZE);
  skip_data(g);
}

static unsigned int local_color_table_size(unsigned int size) {
  /* 2 ^ (size + 1) */
  return 3 * (0x01 << (size + 1));
}

static int skip_section(struct gif_reader *g, enum gif_tag tag) {
  FILE *src;

  src = g->meta.src.file;
  switch (tag) {
    case TAG_GRAPHIC_EXTENSION: break;
    case TAG_GRAPHIC_CONTROL_LABEL: skip_graphic_control(g); break;
    case TAG_COMMENT_LABEL: skip_comment(g); break;
    case TAG_APPLICATION_LABEL: skip_application(g); break;
    case TAG_PLAIN_TEXT_LABEL: skip_plain_text(g); break;
    case TAG_IMAGE_DESCRIPTOR: {
      unsigned char header[IMAGE_DESCRIPTOR_HEADER_SIZE];

      advance_read(g, IMAGE_DESCRIPTOR_HEADER_SIZE, header);
      /* check if there is a local color table */
      if (header[8] & 0x80) {
        advance(g, local_color_table_size(header[8] & 0x07));
      }
      /* skip LZW code size byte */
      advance(g, 1);
      skip_data(g);
    } break;
    default: return -1;
  }
  return 0;
}

static void count_images(struct gif_reader *g) {
  enum gif_tag tag;

  tag = TAG_TRAILER;
  do {
    advance_read(g, 1, &tag);
    if (tag == TAG_IMAGE_DESCRIPTOR) g->n_frames++;
    skip_section(g, tag);
  } while (tag != TAG_TRAILER);
}

static void graphic_control(struct gif_reader *g) {
  unsigned char header[GRAPHIC_CONTROL_HEADER_SIZE];

  advance_read(g, GRAPHIC_CONTROL_HEADER_SIZE, header);
  g->dispose = choose_disposal((header[1] & 0x1c) >> 2);
  READ2BYTES(g->delay, header + 2);
  if (header[1] & 0x01) {
    g->has_trans = 1;
    g->trans_index = header[4];
  } else {
    g->has_trans = 0;
  }
}

static long get_data_size(struct gif_reader *g) {
  unsigned char size;
  long total;

  total = 0;
  if (g->meta.src_type == GIF_FILE) {
    long start;

    start = ftell(g->meta.src.file);
    do {
      fread(&size, 1, 1, g->meta.src.file);
      total += size;
      fseek(g->meta.src.file, size, SEEK_CUR);
    } while (size);
    fseek(g->meta.src.file, start, SEEK_SET);
  } else {
    unsigned char *start;

    start = g->meta.src.ptr;
    do {
      size = *g->meta.src.ptr++;
      total += size;
      g->meta.src.ptr += size;
    } while (size);
    g->meta.src.ptr = start;
  }
  return total;
}

static void parse_image_descriptor(struct gif_reader *g,
                                   unsigned int *out_left,
                                   unsigned int *out_top,
                                   unsigned int *out_width,
                                   unsigned int *out_height) {
  unsigned char header[IMAGE_DESCRIPTOR_HEADER_SIZE];
  unsigned int left, top, width, height;

  advance_read(g, IMAGE_DESCRIPTOR_HEADER_SIZE, header);
  READ2BYTES(left, header);
  READ2BYTES(top, header + 2);
  READ2BYTES(width, header + 4);
  READ2BYTES(height, header + 6);
  /* check for local color table */
  if (header[8] & 0x80) {
    unsigned int size;

    g->has_local_clut = 1;
    /* local CLUT size: 3 * 2 ^ (table size + 1) */
    size = local_color_table_size(header[8] & 0x07);
    advance_read(g, size, g->local_clut);
  }
  *out_left = left;
  *out_top = top;
  *out_width = width;
  *out_height = height;
}

static int decompress_image(struct gif_reader *g,
                            unsigned long *out_len,
                            unsigned char **out_img) {
  unsigned char code_size, *img_compressed, *img;
  long total_bytes;
  unsigned long pos, size, decompressed_size;

  advance_read(g, 1, &code_size);
  total_bytes = get_data_size(g);
  img_compressed = malloc((unsigned long) total_bytes);
  if (!img_compressed) return -1;
  size = 0;
  pos = 0;
  do {
    advance_read(g, 1, &size);
    advance_read(g, size, img_compressed + pos);
    pos += size;
  } while (size);
  lzw_decompress(code_size,
                 (unsigned long) total_bytes,
                 img_compressed,
                 &decompressed_size,
                 &img);
  *out_len = decompressed_size;
  *out_img = img;
  free(img_compressed);
  return 0;
}

static void write_image(struct gif_reader *g,
                        unsigned int left,
                        unsigned int top,
                        unsigned int width,
                        unsigned int height,
                        unsigned long len,
                        unsigned char *img) {
  unsigned char *pal;
  unsigned int i;

  pal = g->has_local_clut ? g->local_clut : g->global_clut;
  if (g->has_trans) {
    for (i = 0; i < height; ++i) {
      unsigned char *dst, *src;
      unsigned int w;

      src = img + (i * width);
      dst = g->image + 3 * (((i + top) * g->width) + left);
      w = width;
      while (w--) {
        if (*src != g->trans_index) {
          unsigned int index;

          index = *src * 3;
          *dst++ = pal[index + 0];
          *dst++ = pal[index + 1];
          *dst++ = pal[index + 2];
        } else {
          dst++;
          dst++;
          dst++;
        }
        src++;
      }
    }
  } else {
    for (i = 0; i < height; ++i) {
      unsigned char *dst, *src;
      unsigned int w;

      src = img + (i * width);
      dst = g->image + 3 * (((i + top) * g->width) + left);
      w = width;
      while (w--) {
        unsigned int index;

        index = *src * 3;
        *dst++ = pal[index + 0];
        *dst++ = pal[index + 1];
        *dst++ = pal[index + 2];
        src++;
      }
    }
  }
}

/* **************************************** */
/* Public */
/* **************************************** */

int gifr_init(struct gif_reader *g, enum gif_source_type type, void *src) {
  if (!g) return -1;
  if (!src) return -1;
  memset(g, 0, sizeof(struct gif_reader));
  g->meta.src_type = type;
  switch (g->meta.src_type) {
    case GIF_FILE:
      g->meta.src.file = (FILE *) src;
      break;
    case GIF_BUFFER:
      g->meta.src.ptr = (unsigned char *) src;
      break;
    default:
      goto fail;
  }
  g->global_clut = malloc(3 * 256);
  if (!g->global_clut) goto fail;
  g->local_clut = malloc(3 * 256);
  if (!g->local_clut) goto fail;
  if (header(g)) goto fail;
  if (logical_screen(g)) goto fail;
  g->image = malloc(g->width * g->height * 3);
  if (!g->image) goto fail;
  if (g->meta.src_type == GIF_FILE) {
    g->meta.start.value = ftell(g->meta.src.file);
  } else {
    g->meta.start.ptr = g->meta.src.ptr;
  }
  count_images(g);
  gifr_head(g);
  return 0;

fail:
  gifr_deinit(g);
  return -1;
}

void gifr_deinit(struct gif_reader *g) {
  if (!g) return;
  if (g->global_clut) free(g->global_clut);
  if (g->local_clut) free(g->local_clut);
  if (g->image) free(g->image);
}

void gifr_head(struct gif_reader *g) {
  if (!g) return;
  if (g->meta.src_type == GIF_FILE) {
    fseek(g->meta.src.file, g->meta.start.value, SEEK_SET);
  } else {
    g->meta.src.ptr = g->meta.start.ptr;
  }
}

int gifr_next(struct gif_reader *g) {
  unsigned char *image;
  unsigned int top, left, width, height;
  unsigned long len;
  enum gif_tag tag;

  if (!g) return 0;
  tag = TAG_TRAILER;
  for (;;) {
    advance_read(g, 1, &tag);
    switch (tag) {
      case TAG_GRAPHIC_CONTROL_LABEL:
        graphic_control(g);
        break;
      case TAG_IMAGE_DESCRIPTOR:
        goto gif_next_process_image;
      default:
        if (skip_section(g, tag)) return 0;
    }
  }
gif_next_process_image:
  parse_image_descriptor(g, &left, &top, &width, &height);
  if (decompress_image(g, &len, &image)) return 0;
  write_image(g, left, top, width, height, len, image);
  free(image);
  return 1;
}

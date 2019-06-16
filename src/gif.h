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

#ifndef GIF_H
#define GIF_H

#define GIF_VERSION_MAJOR 0
#define GIF_VERSION_MINOR 6
#define GIF_VERSION_PATCH 0

#define __GIF_VERSION_STR(maj, min, ptch) (#maj "." #min "." #ptch)
#define _GIF_VERSION_STR(maj, min, ptch) __GIF_VERSION_STR(maj, min, ptch)
#define GIF_VERSION_STR _GIF_VERSION_STR(GIF_VERSION_MAJOR, \
                                         GIF_VERSION_MINOR, \
                                         GIF_VERSION_PATCH)

#include <darray.h>
#include <stdio.h>

enum gif_mode {
  GIF_READ,
  GIF_WRITE
};

enum gif_source_type {
  GIF_FILE,
  GIF_BUFFER
};

enum gif_version {
  GIF_87a,
  GIF_89a
};

enum gif_tag {
  TAG_GRAPHIC_EXTENSION = 0x21,
  TAG_GRAPHIC_CONTROL_LABEL = 0xf9,
  TAG_COMMENT_LABEL = 0xfe,
  TAG_APPLICATION_LABEL = 0xff,
  TAG_PLAIN_TEXT_LABEL = 0x01,
  TAG_IMAGE_DESCRIPTOR = 0x2c,
  TAG_TRAILER = 0x3b
};

/* **************************************** */
/* gif_reader.c */
struct gif_reader {
  struct {
    enum gif_source_type src_type;
    union {
      unsigned char *ptr;
      FILE *file;
    } src;
    union {
      unsigned char *ptr;
      long value;
    } start;
  } meta;
  enum gif_version version;
  unsigned int width;
  unsigned int height;
  unsigned int n_frames;
  unsigned int delay;
  unsigned int n_colors;
  unsigned char aspect;
  unsigned char bg_color_index;
  unsigned char has_global_clut;
  unsigned char *global_clut;
  unsigned char *image;
  unsigned char has_local_clut;
  unsigned char *local_clut;
  unsigned char has_trans;
  unsigned char trans_index;
  void (*dispose)(struct gif_reader *g);
};

int gifr_init(struct gif_reader *g, enum gif_source_type type, void *src);
void gifr_deinit(struct gif_reader *g);
void gifr_head(struct gif_reader *g);
int gifr_next(struct gif_reader *g);
/* **************************************** */

/* **************************************** */
/* gif_writer.c */
struct gif_writer {
  struct {
    enum gif_source_type dst_type;
    union {
      struct darray *ptr;
      FILE *file;
    } dst;
  } meta;
  unsigned int width;
  unsigned int height;
  unsigned int n_colors;
  unsigned char code_size;
  unsigned char *palette;
};

struct gif_opts {
  unsigned int delay;
  unsigned char flags;
  unsigned char trans_index;
};

int gifw_init(struct gif_writer *g,
              unsigned char code_size,
              unsigned char *palette,
              unsigned int width,
              unsigned int height);
void gifw_push(struct gif_writer *g,
               struct gif_opts *opts,
               unsigned int left,
               unsigned int top,
               unsigned int width,
               unsigned int height,
               unsigned char *img);
void gifw_end(struct gif_writer *g,
              size_t *out_len,
              unsigned char **out_img);
/* **************************************** */

#endif

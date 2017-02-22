/*  Copyright (C) 2015-2016 Setplex. All right reserved.
    This file is part of Rixjob.
    Rixjob is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Rixjob is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Rixjob.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "png/png_reader.h"

#include <stdlib.h>

#define ERROR_RESULT_VALUE -1
#define ZERO_RESULT_VALUE 0
#define SUCCESS_RESULT_VALUE ZERO_RESULT_VALUE
#define SIZEOFMASS(type) sizeof(type) / sizeof(*type)

struct PngReader* alloc_png_reader() {
  return calloc(1, sizeof(struct PngReader));
}

static void clear_png_reader(struct PngReader* reader) {
  if (!reader) {
    return;
  }

  if (reader->row_pointers) {
    png_uint_32 y;
    for (y = 0; y < png_img_height(reader); ++y) {
      free(reader->row_pointers[y]);
    }
    free(reader->row_pointers);
    reader->row_pointers = NULL;
  }
  if (reader->png_ptr && reader->info_ptr) {
    png_destroy_read_struct(&reader->png_ptr, &reader->info_ptr, NULL);
    reader->png_ptr = NULL;
    reader->info_ptr = NULL;
  }
}

int read_png_file(struct PngReader* reader, const char* path) {
  if (!path) {
    return ERROR_RESULT_VALUE;
  }

  if (!reader) {
    return ERROR_RESULT_VALUE;
  }

  unsigned char header[8];  // 8 is the maximum size that can be checked
  /* open file and test for it being a png */
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    return ERROR_RESULT_VALUE;
  }
  int res = fread(header, sizeof(unsigned char), SIZEOFMASS(header), fp);
  if (res != SIZEOFMASS(header) || png_sig_cmp(header, 0, SIZEOFMASS(header))) {
    fclose(fp);
    return ERROR_RESULT_VALUE;
  }

  /* initialize stuff */
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return ERROR_RESULT_VALUE;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return ERROR_RESULT_VALUE;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return ERROR_RESULT_VALUE;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  if (bit_depth == 16) {
    png_set_strip_16(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
  }

  // These color_type don't have an alpha channel then fill it with 0xff.
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }

  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return ERROR_RESULT_VALUE;
  }

  png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  png_uint_32 rb = png_get_rowbytes(png_ptr, info_ptr);
  png_uint_32 y;
  for (y = 0; y < height; ++y) {
    row_pointers[y] = (png_byte*)malloc(rb);
  }

  png_read_image(png_ptr, row_pointers);
  fclose(fp);

  clear_png_reader(reader);
  reader->row_pointers = row_pointers;
  reader->png_ptr = png_ptr;
  reader->info_ptr = info_ptr;
  return SUCCESS_RESULT_VALUE;
}

png_uint_32 png_img_width(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_image_width(reader->png_ptr, reader->info_ptr);
}

png_uint_32 png_img_height(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_image_height(reader->png_ptr, reader->info_ptr);
}

png_byte png_img_color_type(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_color_type(reader->png_ptr, reader->info_ptr);
}

png_byte png_img_bit_depth(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_bit_depth(reader->png_ptr, reader->info_ptr);
}

png_byte png_img_channels(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_channels(reader->png_ptr, reader->info_ptr);
}

int png_img_number_of_passes(struct PngReader* reader) {
  if (!reader) {
    return ERROR_RESULT_VALUE;
  }

  return png_set_interlace_handling(reader->png_ptr);
}

png_uint_32 png_img_row_bytes(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  return png_get_rowbytes(reader->png_ptr, reader->info_ptr);
}

size_t png_img_file_size(struct PngReader* reader) {
  if (!reader) {
    return 0;
  }

  png_uint_32 row_bytes = png_img_row_bytes(reader);
  png_uint_32 h = png_img_height(reader);
  png_uint_32 ch = png_img_channels(reader);
  return h * row_bytes * ch;
}

void free_png_reader(struct PngReader** reader) {
  if (!reader) {
    return;
  }

  clear_png_reader(*reader);
  free(*reader);
  *reader = NULL;
}


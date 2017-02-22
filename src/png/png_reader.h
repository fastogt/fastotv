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

#pragma once

#include <png.h>

struct RGBA {
  png_byte r;
  png_byte g;
  png_byte b;
  png_byte a;
};

struct PngReader {
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep* row_pointers;
} typedef png_reader_t;

struct PngReader* alloc_png_reader();
int read_png_file(struct PngReader* reader, const char* path) __attribute__((warn_unused_result));

png_uint_32 png_img_width(struct PngReader* reader);
png_uint_32 png_img_height(struct PngReader* reader);
png_byte png_img_color_type(struct PngReader* reader);
png_byte png_img_bit_depth(struct PngReader* reader);
png_byte png_img_channels(struct PngReader* reader);
int png_img_number_of_passes(struct PngReader* reader);
png_uint_32 png_img_row_bytes(struct PngReader* reader);
size_t png_img_file_size(struct PngReader* reader);

void free_png_reader(struct PngReader** reader);


/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <inttypes.h>
#include <stdbool.h>

#include "journal-def.h"

const char* object_compressed_to_string(int compression);
int object_compressed_from_string(const char *compression);

int compress_blob_xz(const void *src, uint64_t src_size, void *dst, uint64_t *dst_size);

static inline int compress_blob(const void *src, uint64_t src_size, void *dst, uint64_t *dst_size) {
        int r;

        r = compress_blob_xz(src, src_size, dst, dst_size);
        if (r == 0)
                return OBJECT_COMPRESSED_XZ;

        return r;
}

int decompress_blob_xz(const void *src, uint64_t src_size,
                       void **dst, uint64_t *dst_alloc_size, uint64_t* dst_size, uint64_t dst_max);

int decompress_blob(int compression,
                    const void *src, uint64_t src_size,
                    void **dst, uint64_t *dst_alloc_size, uint64_t* dst_size, uint64_t dst_max);

int decompress_startswith_xz(const void *src, uint64_t src_size,
                             void **buffer, uint64_t *buffer_size,
                             const void *prefix, uint64_t prefix_len,
                             uint8_t extra);

int decompress_startswith(int compression,
                          const void *src, uint64_t src_size,
                          void **buffer, uint64_t *buffer_size,
                          const void *prefix, uint64_t prefix_len,
                          uint8_t extra);

/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_XZ
#	include <lzma.h>
#endif

#ifdef HAVE_LZ4
#  include <lz4.h>
#endif

#include "compress.h"
#include "macro.h"
#include "util.h"
#include "journal-def.h"

#define ALIGN_8(l) ALIGN_TO(l, sizeof(size_t))

static const char* const object_compressed_table[_OBJECT_COMPRESSED_MAX] = {
        [OBJECT_COMPRESSED_XZ] = "XZ",
        [OBJECT_COMPRESSED_LZ4] = "LZ4",
};

DEFINE_STRING_TABLE_LOOKUP(object_compressed, int);


int compress_blob_xz(const void *src, uint64_t src_size, void *dst, uint64_t *dst_size) {
#ifdef HAVE_XZ
        static const lzma_options_lzma opt = {
                1u << 20u, NULL, 0, LZMA_LC_DEFAULT, LZMA_LP_DEFAULT,
                LZMA_PB_DEFAULT, LZMA_MODE_FAST, 128, LZMA_MF_HC3, 4};
        static const lzma_filter filters[] = {
                {LZMA_FILTER_LZMA2, (lzma_options_lzma*) &opt},
                {LZMA_VLI_UNKNOWN, NULL}
        };
        lzma_ret ret;
        size_t out_pos = 0;

        assert(src);
        assert(src_size > 0);
        assert(dst);
        assert(dst_size);

        /* Returns < 0 if we couldn't compress the data or the
         * compressed result is longer than the original */

        if (src_size < 80)
                return -ENOBUFS;

        ret = lzma_stream_buffer_encode((lzma_filter*) filters, LZMA_CHECK_NONE, NULL,
                                        src, src_size, dst, &out_pos, src_size - 1);
        if (ret != LZMA_OK)
                return -ENOBUFS;

        *dst_size = out_pos;
        return 0;
#else
        return -EPROTONOSUPPORT;
#endif
}

int compress_blob_lz4(const void *src, uint64_t src_size, void *dst, uint64_t *dst_size) {
#ifdef HAVE_LZ4
        int r;

        assert(src);
        assert(src_size > 0);
        assert(dst);
        assert(dst_size);

        /* Returns < 0 if we couldn't compress the data or the
         * compressed result is longer than the original */

        if (src_size < 9)
                return -ENOBUFS;

        r = LZ4_compress_limitedOutput(src, dst + 8, src_size, src_size - 8 - 1);
        if (r <= 0)
                return -ENOBUFS;

        *(le64_t*) dst = htole64(src_size);
        *dst_size = r + 8;

        return 0;
#else
        return -EPROTONOSUPPORT;
#endif
}

int decompress_blob_xz(const void *src, uint64_t src_size,
                       void **dst, uint64_t *dst_alloc_size, uint64_t* dst_size, uint64_t dst_max) {

#ifdef HAVE_XZ
        _cleanup_(lzma_end) lzma_stream s = LZMA_STREAM_INIT;
        lzma_ret ret;
        uint64_t space;

        assert(src);
        assert(src_size > 0);
        assert(dst);
        assert(dst_alloc_size);
        assert(dst_size);
        assert(*dst_alloc_size == 0 || *dst);

        ret = lzma_stream_decoder(&s, UINT64_MAX, 0);
        if (ret != LZMA_OK)
                return -ENOMEM;

        space = MIN(src_size * 2, dst_max ?: (uint64_t) -1);
        if (!greedy_realloc(dst, dst_alloc_size, space, 1))
                return -ENOMEM;

        s.next_in = src;
        s.avail_in = src_size;

        s.next_out = *dst;
        s.avail_out = space;

        for (;;) {
                uint64_t used;

                ret = lzma_code(&s, LZMA_FINISH);

                if (ret == LZMA_STREAM_END)
                        break;

                if (ret != LZMA_OK)
                        return -ENOMEM;

                if (dst_max > 0 && (space - s.avail_out) >= dst_max)
                        break;

                if (dst_max > 0 && space == dst_max)
                        return -ENOBUFS;

                used = space - s.avail_out;
                space = MIN(2 * space, dst_max ?: (uint64_t) -1);
                if (!greedy_realloc(dst, dst_alloc_size, space, 1))
                        return -ENOMEM;

                s.avail_out = space - used;
                s.next_out = *dst + used;
        }

        *dst_size = space - s.avail_out;
        return 0;
#else
        return -EPROTONOSUPPORT;
#endif
}

int decompress_blob_lz4(const void *src, uint64_t src_size,
                        void **dst, uint64_t *dst_alloc_size, uint64_t* dst_size, uint64_t dst_max) {

#ifdef HAVE_LZ4
        char* out;
        uint64_t size;
        int r;

        assert(src);
        assert(src_size > 0);
        assert(dst);
        assert(dst_alloc_size);
        assert(dst_size);
        assert(*dst_alloc_size == 0 || *dst);

        if (src_size <= 8)
                return -EBADMSG;

        size = le64toh( *(le64_t*)src );
        if (size > *dst_alloc_size) {
                out = realloc(*dst, size);
                if (!out)
                        return -ENOMEM;
                *dst = out;
                *dst_alloc_size = size;
        } else
                out = *dst;

        r = LZ4_decompress_safe(src + 8, out, src_size - 8, size);
        if (r < 0 || (uint64_t) r != size)
                return -EBADMSG;

        *dst_size = size;
        return 0;
#else
        return -EPROTONOSUPPORT;
#endif
}

int decompress_blob(int compression,
                    const void *src, uint64_t src_size,
                    void **dst, uint64_t *dst_alloc_size, uint64_t* dst_size, uint64_t dst_max) {

        if (compression == OBJECT_COMPRESSED_XZ)
                return decompress_blob_xz(src, src_size,
                                          dst, dst_alloc_size, dst_size, dst_max);

        if (compression == OBJECT_COMPRESSED_LZ4)
                return decompress_blob_lz4(src, src_size,
                                           dst, dst_alloc_size, dst_size, dst_max);

        return -EBADMSG;
}

int decompress_startswith_xz(const void *src, uint64_t src_size,
                             void **buffer, uint64_t *buffer_size,
                             const void *prefix, uint64_t prefix_len,
                             uint8_t extra) {

#ifdef HAVE_XZ
        _cleanup_(lzma_end) lzma_stream s = LZMA_STREAM_INIT;
        lzma_ret ret;

        /* Checks whether the decompressed blob starts with the
         * mentioned prefix. The byte extra needs to follow the
         * prefix */

        assert(src);
        assert(src_size > 0);
        assert(buffer);
        assert(buffer_size);
        assert(prefix);
        assert(*buffer_size == 0 || *buffer);

        ret = lzma_stream_decoder(&s, UINT64_MAX, 0);
        if (ret != LZMA_OK)
                return -EBADMSG;

        if (!(greedy_realloc(buffer, buffer_size, ALIGN_8(prefix_len + 1), 1)))
                return -ENOMEM;

        s.next_in = src;
        s.avail_in = src_size;

        s.next_out = *buffer;
        s.avail_out = *buffer_size;

        for (;;) {
                ret = lzma_code(&s, LZMA_FINISH);

                if (ret != LZMA_STREAM_END && ret != LZMA_OK)
                        return -EBADMSG;

                if (*buffer_size - s.avail_out > prefix_len)
                        break;

                if (ret == LZMA_STREAM_END)
                        return 0;

                s.avail_out += *buffer_size;

                if (!(greedy_realloc(buffer, buffer_size, *buffer_size * 2, 1)))
                        return -ENOMEM;

                s.next_out = *buffer + *buffer_size - s.avail_out;
        }

        if (memcmp(*buffer, prefix, prefix_len) != 0)
                return 0;

        return ((const uint8_t*) *buffer)[prefix_len] == extra;
#else
        return -EPROTONOSUPPORT;
#endif
}

int decompress_startswith_lz4(const void *src, uint64_t src_size,
                              void **buffer, uint64_t *buffer_size,
                              const void *prefix, uint64_t prefix_len,
                              uint8_t extra) {
#ifdef HAVE_LZ4
        /* Checks whether the decompressed blob starts with the
         * mentioned prefix. The byte extra needs to follow the
         * prefix */

        int r;

        assert(src);
        assert(src_size > 0);
        assert(buffer);
        assert(buffer_size);
        assert(prefix);
        assert(*buffer_size == 0 || *buffer);

        if (src_size <= 8)
                return -EBADMSG;

        if (!(greedy_realloc(buffer, buffer_size, ALIGN_8(prefix_len + 1), 1)))
                return -ENOMEM;

        r = LZ4_decompress_safe_partial(src + 8, *buffer, src_size - 8,
                                        prefix_len + 1, *buffer_size);

        if (r < 0)
                return -EBADMSG;
        if ((unsigned) r >= prefix_len + 1)
                return memcmp(*buffer, prefix, prefix_len) == 0 &&
                        ((const uint8_t*) *buffer)[prefix_len] == extra;
        else
                return 0;

#else
        return -EPROTONOSUPPORT;
#endif
}

int decompress_startswith(int compression,
                          const void *src, uint64_t src_size,
                          void **buffer, uint64_t *buffer_size,
                          const void *prefix, uint64_t prefix_len,
                          uint8_t extra) {

        if (compression == OBJECT_COMPRESSED_XZ)
                return decompress_startswith_xz(src, src_size,
                                                buffer, buffer_size,
                                                prefix, prefix_len,
                                                extra);

        if (compression == OBJECT_COMPRESSED_LZ4)
                return decompress_startswith_lz4(src, src_size,
                                                 buffer, buffer_size,
                                                 prefix, prefix_len,
                                                 extra);

        return -EBADMSG;
}
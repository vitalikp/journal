/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

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

#include <stdio.h>
#include <stdbool.h>

#include "macro.h"

/* An abstract parser for simple, line based, shallow configuration
 * files consisting of variable assignments only. */

/* Prototype for a parser for a specific configuration setting */
typedef int (*ConfigParserCallback)(const char *filename,
                                    unsigned line,
                                    const char *rvalue,
                                    void *data);

/* Wraps information for parsing a specific configuration variable, to
 * be stored in a gperf perfect hashtable */
typedef struct ConfigPerfItem {
        const char *section_and_lvalue; /* Section + "." + name of the variable */
        ConfigParserCallback parse;     /* Function that is called to parse the variable's value */
        int ltype;                      /* Distinguish different variables passed to the same callback */
        size_t offset;                  /* Offset where to store data, from the beginning of userdata */
} ConfigPerfItem;

/* Prototype for a low-level gperf lookup function */
typedef const ConfigPerfItem* (*ConfigPerfItemLookup)(const char *section_and_lvalue, unsigned length);

/* Prototype for a generic high-level lookup function */
typedef int (*ConfigItemLookup)(
                const void *table,
                const char *section,
                const char *lvalue,
                ConfigParserCallback *func,
                int *ltype,
                void **data,
                void *userdata);

/* gperf implementation of ConfigItemLookup, based on gperf
 * ConfigPerfItem tables */
int config_item_perf_lookup(const void *table, const char *section, const char *lvalue, ConfigParserCallback *func, int *ltype, void **data, void *userdata);

int config_parse(const char *filename,
                 const char *sections,  /* nulstr */
                 ConfigItemLookup lookup,
                 const void *table,
                 void *userdata);

/* Generic parsers */
int config_parse_unsigned(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_iec_off(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_bool(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_string(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_path(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_sec(const char *filename, unsigned line, const char *rvalue, void *data);
int config_parse_log_level(const char *filename, unsigned line, const char *rvalue, void *data);

int log_syntax_internal(int level,
                        const char *file, unsigned line, const char *func,
                        const char *config_file, unsigned config_line,
                        int error, const char *format, ...) _printf_(8, 9);

#define log_syntax(level, config_file, config_line, error, ...)   \
        log_syntax_internal(level,                                \
                            __FILE__, __LINE__, __func__,               \
                            config_file, config_line,                   \
                            error, __VA_ARGS__)

#define log_invalid_utf8(level, config_file, config_line, error, rvalue) { \
        _cleanup_free_ char *__p = utf8_escape_invalid(rvalue);                  \
        log_syntax(level, config_file, config_line, error,                 \
                   "String is not UTF-8 clean, ignoring assignment: %s", __p);   \
        }

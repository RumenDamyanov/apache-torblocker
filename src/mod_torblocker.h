/*
 * Copyright 2026 Rumen Damyanov <contact@rumenx.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * mod_torblocker — Apache module shim header
 */

#ifndef MOD_TORBLOCKER_H
#define MOD_TORBLOCKER_H

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_config.h"
#include "apr_strings.h"
#include "apr_thread_proc.h"

#define TORBLOCKER_VERSION "0.1.0"

#define TORBLOCKER_DEFAULT_URL       "https://check.torproject.org/torbulkexitlist"
#define TORBLOCKER_DEFAULT_REFRESH   3600    /* seconds */
#define TORBLOCKER_MIN_REFRESH       300     /* 5 minutes */

/* Blocking modes */
#define TORBLOCKER_OFF   0   /* Allow all traffic */
#define TORBLOCKER_ON    1   /* Block Tor exit nodes */
#define TORBLOCKER_ONLY  2   /* Allow only Tor exit nodes */

/* Server configuration */
typedef struct {
    const char *list_url;
    int         refresh_interval;      /* seconds */
} torblocker_server_conf;

/* Per-directory configuration */
typedef struct {
    int mode;    /* TORBLOCKER_OFF / ON / ONLY */
    int mode_set;
} torblocker_dir_conf;

/* Rust FFI declarations */
extern int          torblocker_update(const char *url);
extern int          torblocker_check_ip(const char *ip_str);
extern unsigned int torblocker_ip_count(void);
extern const char  *torblocker_version(void);

#endif /* MOD_TORBLOCKER_H */

/*
 * Copyright 2026 Rumen Damyanov <contact@rumenx.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * mod_torblocker — Apache module for Tor exit node access control
 *
 * Thin C shim. All business logic (HTTP fetch, IP set, lookups) is in Rust.
 */

#include "mod_torblocker.h"

#include <unistd.h>

module AP_MODULE_DECLARE_DATA torblocker_module;

static apr_thread_t *update_thread = NULL;
static volatile int  shutdown_flag = 0;

/* ----------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------- */

static void *
create_server_conf(apr_pool_t *p, server_rec *s)
{
    torblocker_server_conf *conf = apr_pcalloc(p, sizeof(*conf));
    conf->list_url         = TORBLOCKER_DEFAULT_URL;
    conf->refresh_interval = TORBLOCKER_DEFAULT_REFRESH;
    return conf;
}

static void *
create_dir_conf(apr_pool_t *p, char *dir)
{
    torblocker_dir_conf *conf = apr_pcalloc(p, sizeof(*conf));
    conf->mode     = TORBLOCKER_OFF;
    conf->mode_set = 0;
    return conf;
}

static void *
merge_dir_conf(apr_pool_t *p, void *base_conf, void *new_conf)
{
    torblocker_dir_conf *base = base_conf;
    torblocker_dir_conf *add  = new_conf;
    torblocker_dir_conf *conf = apr_pcalloc(p, sizeof(*conf));

    conf->mode     = add->mode_set ? add->mode : base->mode;
    conf->mode_set = add->mode_set || base->mode_set;

    return conf;
}

/* ----------------------------------------------------------------
 * Directive handlers
 * ---------------------------------------------------------------- */

static const char *
set_mode(cmd_parms *cmd, void *dcfg, const char *val)
{
    torblocker_dir_conf *conf = dcfg;

    if (strcasecmp(val, "on") == 0) {
        conf->mode = TORBLOCKER_ON;
    } else if (strcasecmp(val, "off") == 0) {
        conf->mode = TORBLOCKER_OFF;
    } else if (strcasecmp(val, "only") == 0) {
        conf->mode = TORBLOCKER_ONLY;
    } else {
        return apr_psprintf(cmd->pool, "TorBlock must be 'on', 'off', or 'only'");
    }

    conf->mode_set = 1;
    return NULL;
}

static const char *
set_source_url(cmd_parms *cmd, void *dummy, const char *url)
{
    torblocker_server_conf *conf =
        ap_get_module_config(cmd->server->module_config, &torblocker_module);
    conf->list_url = apr_pstrdup(cmd->pool, url);
    return NULL;
}

static const char *
set_refresh_interval(cmd_parms *cmd, void *dummy, const char *val)
{
    torblocker_server_conf *conf =
        ap_get_module_config(cmd->server->module_config, &torblocker_module);
    int interval = atoi(val);

    if (interval < TORBLOCKER_MIN_REFRESH) {
        return apr_psprintf(cmd->pool,
            "TorBlockRefreshInterval must be >= %d seconds",
            TORBLOCKER_MIN_REFRESH);
    }

    conf->refresh_interval = interval;
    return NULL;
}

/* ----------------------------------------------------------------
 * Background update thread
 * ---------------------------------------------------------------- */

static void * APR_THREAD_FUNC
update_thread_func(apr_thread_t *thread, void *data)
{
    torblocker_server_conf *conf = data;
    int count;

    apr_sleep(apr_time_from_sec(2));

    while (!shutdown_flag) {
        count = torblocker_update(conf->list_url);

        if (count >= 0) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL,
                "torblocker: updated exit list (%d IPs)", count);
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                "torblocker: failed to update exit list");
        }

        for (int i = 0; i < conf->refresh_interval && !shutdown_flag; i++) {
            apr_sleep(apr_time_from_sec(1));
        }
    }

    return NULL;
}

/* ----------------------------------------------------------------
 * Access checker hook
 * ---------------------------------------------------------------- */

static int
access_checker(request_rec *r)
{
    torblocker_dir_conf *conf =
        ap_get_module_config(r->per_dir_config, &torblocker_module);

    if (conf->mode == TORBLOCKER_OFF) {
        return DECLINED;
    }

    const char *client_ip = r->useragent_ip ? r->useragent_ip : r->connection->client_ip;
    int is_tor = torblocker_check_ip(client_ip);

    if (is_tor < 0) {
        /* Invalid IP — let other modules handle it */
        return DECLINED;
    }

    if (conf->mode == TORBLOCKER_ON && is_tor) {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
            "torblocker: blocked Tor exit node %s", client_ip);
        return HTTP_FORBIDDEN;
    }

    if (conf->mode == TORBLOCKER_ONLY && !is_tor) {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
            "torblocker: blocked non-Tor client %s", client_ip);
        return HTTP_FORBIDDEN;
    }

    return DECLINED;
}

/* ----------------------------------------------------------------
 * Hooks
 * ---------------------------------------------------------------- */

static int
post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    /* Avoid double-init during two-pass config loading */
    void *data = NULL;
    const char *key = "torblocker_post_config";
    apr_pool_userdata_get(&data, key, s->process->pool);
    if (data == NULL) {
        apr_pool_userdata_set((void *)1, key, apr_pool_cleanup_null, s->process->pool);
        return OK;
    }

    torblocker_server_conf *conf =
        ap_get_module_config(s->module_config, &torblocker_module);

    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s,
        "torblocker v%s: starting (refresh=%ds, url=%s)",
        torblocker_version(), conf->refresh_interval, conf->list_url);

    shutdown_flag = 0;

    apr_status_t rv = apr_thread_create(&update_thread, NULL,
        update_thread_func, conf, pconf);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, s,
            "torblocker: failed to create update thread");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    return OK;
}

static apr_status_t
child_cleanup(void *data)
{
    shutdown_flag = 1;
    if (update_thread) {
        apr_status_t rv;
        apr_thread_join(&rv, update_thread);
        update_thread = NULL;
    }
    return APR_SUCCESS;
}

static void
child_init(apr_pool_t *p, server_rec *s)
{
    apr_pool_cleanup_register(p, NULL, child_cleanup, apr_pool_cleanup_null);
}

/* ----------------------------------------------------------------
 * Module definition
 * ---------------------------------------------------------------- */

static const command_rec directives[] = {
    AP_INIT_TAKE1("TorBlock", set_mode, NULL, ACCESS_CONF | OR_AUTHCFG,
        "Tor blocking mode: on|off|only"),
    AP_INIT_TAKE1("TorBlockSourceUrl", set_source_url, NULL, RSRC_CONF,
        "URL for Tor exit list (default: torproject.org)"),
    AP_INIT_TAKE1("TorBlockRefreshInterval", set_refresh_interval, NULL, RSRC_CONF,
        "Refresh interval in seconds (default: 3600, min: 300)"),
    { NULL }
};

static void
register_hooks(apr_pool_t *p)
{
    ap_hook_post_config(post_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_access_checker(access_checker, NULL, NULL, APR_HOOK_MIDDLE);
}

AP_DECLARE_MODULE(torblocker) = {
    STANDARD20_MODULE_STUFF,
    create_dir_conf,        /* create per-directory config */
    merge_dir_conf,         /* merge per-directory config */
    create_server_conf,     /* create per-server config */
    NULL,                   /* merge per-server config */
    directives,             /* directive table */
    register_hooks          /* register hooks */
};

// Copyright 2026 Rumen Damyanov <contact@rumenx.com>
// SPDX-License-Identifier: Apache-2.0

//! apache-torblocker — Rust core library
//!
//! Fetches Tor exit node lists and provides O(1) IP lookups.
//! Exported as a static library for linking with the C Apache module shim.

pub mod fetch;
pub mod ipset;

use std::ffi::CStr;
use std::net::IpAddr;
use std::os::raw::c_char;
use std::sync::LazyLock;

/// Global IP set shared between the update thread and request handlers.
static TOR_IPS: LazyLock<ipset::IpSet> = LazyLock::new(ipset::IpSet::new);

/// Fetches the Tor exit list and updates the global IP set.
///
/// # Safety
/// `url` must be a valid, non-null, NUL-terminated C string.
///
/// Returns the number of IPs loaded, or -1 on error.
#[no_mangle]
pub unsafe extern "C" fn torblocker_update(url: *const c_char) -> i32 {
    let url_str = match ptr_to_str(url) {
        Some(s) => s,
        None => return -1,
    };

    match fetch::fetch_tor_exits(url_str) {
        Ok(ips) => {
            let count = TOR_IPS.replace(ips);
            count as i32
        }
        Err(e) => {
            eprintln!("torblocker: fetch failed: {}", e);
            -1
        }
    }
}

/// Checks if an IP address is a known Tor exit node.
///
/// # Safety
/// `ip_str` must be a valid, non-null, NUL-terminated C string.
///
/// Returns: 1 = Tor exit, 0 = not Tor, -1 = invalid IP
#[no_mangle]
pub unsafe extern "C" fn torblocker_check_ip(ip_str: *const c_char) -> i32 {
    let ip = match ptr_to_str(ip_str) {
        Some(s) => s,
        None => return -1,
    };

    match ip.parse::<IpAddr>() {
        Ok(addr) => {
            if TOR_IPS.contains(&addr) {
                1
            } else {
                0
            }
        }
        Err(_) => -1,
    }
}

/// Returns the number of IPs currently in the Tor exit list.
#[no_mangle]
pub extern "C" fn torblocker_ip_count() -> u32 {
    TOR_IPS.len() as u32
}

/// Returns the module version. Caller must NOT free the result.
#[no_mangle]
pub extern "C" fn torblocker_version() -> *const c_char {
    static VERSION: &[u8] = b"0.1.0\0";
    VERSION.as_ptr() as *const c_char
}

unsafe fn ptr_to_str<'a>(ptr: *const c_char) -> Option<&'a str> {
    if ptr.is_null() {
        return None;
    }
    CStr::from_ptr(ptr).to_str().ok()
}

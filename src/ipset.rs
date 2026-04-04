// Copyright 2026 Rumen Damyanov <contact@rumenx.com>
// SPDX-License-Identifier: Apache-2.0

//! Thread-safe IP hash set for Tor exit node lookups.

use std::collections::HashSet;
use std::net::IpAddr;
use std::sync::RwLock;

/// A thread-safe set of IP addresses for O(1) lookups.
pub struct IpSet {
    inner: RwLock<HashSet<IpAddr>>,
}

impl Default for IpSet {
    fn default() -> Self {
        Self::new()
    }
}

impl IpSet {
    pub fn new() -> Self {
        IpSet {
            inner: RwLock::new(HashSet::new()),
        }
    }

    /// Replaces the current set with a new one. Returns the number of IPs loaded.
    pub fn replace(&self, ips: Vec<IpAddr>) -> usize {
        let count = ips.len();
        let set: HashSet<IpAddr> = ips.into_iter().collect();
        if let Ok(mut guard) = self.inner.write() {
            *guard = set;
        }
        count
    }

    /// Checks if an IP is in the set.
    pub fn contains(&self, ip: &IpAddr) -> bool {
        self.inner
            .read()
            .map(|guard| guard.contains(ip))
            .unwrap_or(false)
    }

    /// Returns the number of IPs in the set.
    pub fn len(&self) -> usize {
        self.inner.read().map(|guard| guard.len()).unwrap_or(0)
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ipset_basic() {
        let set = IpSet::new();
        assert_eq!(set.len(), 0);

        let ips = vec![
            "10.0.0.1".parse().unwrap(),
            "192.168.1.1".parse().unwrap(),
        ];
        set.replace(ips);
        assert_eq!(set.len(), 2);
        assert!(set.contains(&"10.0.0.1".parse().unwrap()));
        assert!(!set.contains(&"10.0.0.2".parse().unwrap()));
    }

    #[test]
    fn test_ipset_replace() {
        let set = IpSet::new();
        set.replace(vec!["10.0.0.1".parse().unwrap()]);
        assert!(set.contains(&"10.0.0.1".parse().unwrap()));

        set.replace(vec!["10.0.0.2".parse().unwrap()]);
        assert!(!set.contains(&"10.0.0.1".parse().unwrap()));
        assert!(set.contains(&"10.0.0.2".parse().unwrap()));
    }

    #[test]
    fn test_ipset_ipv6() {
        let set = IpSet::new();
        set.replace(vec!["2001:db8::1".parse().unwrap()]);
        assert!(set.contains(&"2001:db8::1".parse().unwrap()));
        assert!(!set.contains(&"2001:db8::2".parse().unwrap()));
    }

    #[test]
    fn test_ipset_dedup() {
        let set = IpSet::new();
        set.replace(vec![
            "10.0.0.1".parse().unwrap(),
            "10.0.0.1".parse().unwrap(),
        ]);
        assert_eq!(set.len(), 1);
    }
}

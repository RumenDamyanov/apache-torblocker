// Copyright 2026 Rumen Damyanov <contact@rumenx.com>
// SPDX-License-Identifier: Apache-2.0

//! Tor exit list fetcher and parser.

use std::net::IpAddr;

const USER_AGENT: &str = "apache-torblocker/0.1.0";

/// Fetches the Tor bulk exit list and returns parsed IP addresses.
pub fn fetch_tor_exits(url: &str) -> Result<Vec<IpAddr>, String> {
    let resp = ureq::agent()
        .get(url)
        .set("User-Agent", USER_AGENT)
        .call()
        .map_err(|e| format!("fetch {}: {}", url, e))?;

    if resp.status() != 200 {
        return Err(format!("fetch {}: HTTP {}", url, resp.status()));
    }

    let body = resp
        .into_string()
        .map_err(|e| format!("read body: {}", e))?;

    parse_exit_list(&body)
}

/// Parses the Tor bulk exit list format.
///
/// Each line is either an IP address or a comment (starting with #).
/// The format from check.torproject.org/torbulkexitlist has one IP per line.
fn parse_exit_list(text: &str) -> Result<Vec<IpAddr>, String> {
    let mut ips = Vec::new();

    for line in text.lines() {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with('#') {
            continue;
        }

        // The list might have extra fields; take only the first token
        let token = trimmed.split_whitespace().next().unwrap_or("");
        if let Ok(ip) = token.parse::<IpAddr>() {
            ips.push(ip);
        }
    }

    Ok(ips)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_exit_list() {
        let input = "# Tor exit list\n\
                      176.10.99.200\n\
                      185.220.101.1\n\
                      # comment\n\
                      \n\
                      198.98.56.149\n";
        let ips = parse_exit_list(input).unwrap();
        assert_eq!(ips.len(), 3);
        assert_eq!(ips[0], "176.10.99.200".parse::<IpAddr>().unwrap());
    }

    #[test]
    fn test_parse_exit_list_invalid_lines() {
        let input = "176.10.99.200\nnot-an-ip\n192.168.1.1\n";
        let ips = parse_exit_list(input).unwrap();
        assert_eq!(ips.len(), 2);
    }

    #[test]
    fn test_parse_exit_list_empty() {
        let ips = parse_exit_list("# only comments\n\n").unwrap();
        assert!(ips.is_empty());
    }
}

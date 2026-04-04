#
# spec file for package apache-torblocker
#
# Copyright (c) 2026 Rumen Damyanov
# SPDX-License-Identifier: Apache-2.0

%global debug_package %{nil}

Name:           apache-torblocker
Version:        0.1.0
Release:        1%{?dist}
Summary:        apache-torblocker Apache module

License:        Apache-2.0
URL:            https://github.com/RumenDamyanov/apache-torblocker
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  rust
BuildRequires:  cargo
BuildRequires:  httpd-devel

Requires:       httpd >= 2.4.0

%description
Apache httpd module. See project README for details.

%prep
%setup -q

%build
export CARGO_HOME=$(pwd)/target/cargo_home
cargo build --release
apxs -c -Wc,-Wall -Wc,-std=c99 \
    -Wl,target/release/libtorblocker.a \
    -lpthread -ldl -lm \
    src/mod_torblocker.c

%install
%if 0%{?suse_version}
MODULES_DIR=%{_usr}/lib64/apache2
%else
MODULES_DIR=%{_libdir}/httpd/modules
%endif

install -d %{buildroot}${MODULES_DIR}
install -m 0644 src/.libs/mod_torblocker.so %{buildroot}${MODULES_DIR}/

%files
%license LICENSE.md
%doc README.md
%if 0%{?suse_version}
%dir %{_usr}/lib64/apache2
%{_usr}/lib64/apache2/mod_torblocker.so
%else
%dir %{_libdir}/httpd
%dir %{_libdir}/httpd/modules
%{_libdir}/httpd/modules/mod_torblocker.so
%endif

%changelog
* Sat Apr 04 2026 Rumen Damyanov <contact@rumenx.com> - 0.1.0-1
- Initial release

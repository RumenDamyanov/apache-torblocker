# Copyright 2026 Rumen Damyanov <contact@rumenx.com>
# SPDX-License-Identifier: Apache-2.0
#
# apache-torblocker — Build system

APXS      ?= apxs
CARGO     ?= cargo
INSTALL   ?= install

MODULE_NAME = mod_torblocker
RUST_LIB    = torblocker
RUST_TARGET = target/release

RUST_STATIC_LIB = $(RUST_TARGET)/lib$(RUST_LIB).a
RUST_SYSLIBS = -lpthread -ldl -lm

.PHONY: all clean install test rust-build rust-test

all: rust-build module

rust-build:
	$(CARGO) build --release

module: rust-build
	$(APXS) -c -o $(MODULE_NAME).so \
		-Wc,-Wall -Wc,-Wextra \
		-Wl,$(RUST_STATIC_LIB) \
		$(RUST_SYSLIBS) \
		src/$(MODULE_NAME).c

rust-test:
	$(CARGO) test

test: module
	@echo "Running integration tests..."
	@if [ -d test ]; then \
		for t in test/*_test.sh; do \
			[ -f "$$t" ] && bash "$$t"; \
		done; \
	fi

install: module
	$(APXS) -i -a -n $(RUST_LIB) src/.libs/$(MODULE_NAME).so

clean:
	$(CARGO) clean
	rm -rf src/.libs src/*.o src/*.lo src/*.la src/*.slo
	rm -f $(MODULE_NAME).so

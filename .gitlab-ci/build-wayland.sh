#!/usr/bin/env bash
# shellcheck disable=SC2086 # we want word splitting

set -ex

# When changing this file, you need to bump the following .gitlab-ci.yml tag:
# FDO_DISTRIBUTION_TAG

export WAYLAND_PROTOCOLS_VERSION="1.24"

git clone https://gitlab.freedesktop.org/wayland/wayland-protocols
cd wayland-protocols
git checkout "$WAYLAND_PROTOCOLS_VERSION"
meson setup _build $EXTRA_MESON_ARGS
meson install -C _build
cd ..
rm -rf wayland-protocols

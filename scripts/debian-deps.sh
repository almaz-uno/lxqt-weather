#!/bin/sh
# Packages that building, testing and packaging lxqt-weather need on Debian 12
# — the environment of CI and of release builds (specs/001-releases): git, the
# C++ toolchain and CMake, Qt 5 with Qt Test, liblxqt with Qt X11 Extras, which
# its pkg-config module requires and its package does not pull, and
# lxqt-panel, which carries the panel plugin headers in Debian 12; dpkg-dev and
# file for dpkg-shlibdeps; gh publishes releases. Run as root.
set -eu

apt-get update
apt-get install -y --no-install-recommends \
	ca-certificates git cmake make g++ pkg-config \
	qtbase5-dev libqt5x11extras5-dev liblxqt1-dev lxqt-panel \
	dpkg-dev file gh

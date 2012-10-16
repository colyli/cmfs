#!/bin/bash -x

set -e

aclocal
autoconf
automake -a
./configure
make

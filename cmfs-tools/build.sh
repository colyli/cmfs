#!/bin/bash

set -e

aclocal
autoconf
automake -a
./configure
make

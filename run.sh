#!/bin/bash




set -euo pipefail

g++ -std=c++23 -shared -fPIC -Wall -Wextra \
  $(pkg-config --cflags mpv) \
  plugin.cc -o plugin.so -lfmt



g++ -std=c++23 -shared -fPIC -Wall -Wextra -lfmt 



mpv --script=./plugin.so a.mkv



g++ -std=c++20 -O2 -shared -fPIC \
  -I. -I./include \
  src/plugin.cc \
  -pthread -lfmt \
  -o plugin.so


mpv --script=./plugin.so  ./a.mkv

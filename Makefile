fast_align: src/fast_align.cc src/ttables.h src/da.h src/corpus.h
	g++ -Werror -Wall -O3 -g -std=c++11 -mtune=native -ltcmalloc -fopenmp -I. src/fast_align.cc -o $@

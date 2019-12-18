#include "src/ttables.h"

#include <cmath>
#include <string>
#include <fstream>

#include "src/corpus.h"

void TTable::DeserializeLogProbsFromText(std::istream* in, Dict& d) {
  int c = 0;
  std::string e, f;
  double p;
  while(*in) {
    (*in) >> e >> f >> p;
    if (e.empty()) break;
    ++c;
    unsigned ie = d.Convert(e);
    if (ie >= ttable.size()) ttable.resize(ie + 1);
    ttable[ie][d.Convert(f)] = std::exp(p);
  }
  std::cerr << "Loaded " << c << " translation parameters.\n";
}


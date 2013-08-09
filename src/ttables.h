// Copyright 2013 by Chris Dyer
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef _TTABLES_H_
#define _TTABLES_H_

#include <cmath>
#include <fstream>
#include <tr1/unordered_map>

struct Md {
  static double digamma(double x) {
    double result = 0, xx, xx2, xx4;
    for ( ; x < 7; ++x)
      result -= 1/x;
    x -= 1.0/2.0;
    xx = 1.0/x;
    xx2 = xx*xx;
    xx4 = xx2*xx2;
    result += log(x)+(1./24.)*xx2-(7.0/960.0)*xx4+(31.0/8064.0)*xx4*xx2-(127.0/30720.0)*xx4*xx4;
    return result;
  }
};

class TTable {
 public:
  TTable() {}
  typedef std::tr1::unordered_map<unsigned, double> Word2Double;
  typedef std::tr1::unordered_map<unsigned, Word2Double> Word2Word2Double;
  inline double prob(const int& e, const int& f) const {
    const Word2Word2Double::const_iterator cit = ttable.find(e);
    if (cit != ttable.end()) {
      const Word2Double& cpd = cit->second;
      const Word2Double::const_iterator it = cpd.find(f);
      if (it == cpd.end()) return 1e-9;
      return it->second;
    } else {
      return 1e-9;
    }
  }
  inline void Increment(const int& e, const int& f) {
    counts[e][f] += 1.0;
  }
  inline void Increment(const int& e, const int& f, double x) {
    counts[e][f] += x;
  }
  void NormalizeVB(const double alpha) {
    ttable.swap(counts);
    for (Word2Word2Double::iterator cit = ttable.begin();
         cit != ttable.end(); ++cit) {
      double tot = 0;
      Word2Double& cpd = cit->second;
      for (Word2Double::iterator it = cpd.begin(); it != cpd.end(); ++it)
        tot += it->second + alpha;
      for (Word2Double::iterator it = cpd.begin(); it != cpd.end(); ++it)
        it->second = exp(Md::digamma(it->second + alpha) - Md::digamma(tot));
    }
    counts.clear();
  }
  void Normalize() {
    ttable.swap(counts);
    for (Word2Word2Double::iterator cit = ttable.begin();
         cit != ttable.end(); ++cit) {
      double tot = 0;
      Word2Double& cpd = cit->second;
      for (Word2Double::iterator it = cpd.begin(); it != cpd.end(); ++it)
        tot += it->second;
      for (Word2Double::iterator it = cpd.begin(); it != cpd.end(); ++it)
        it->second /= tot;
    }
    counts.clear();
  }
  // adds counts from another TTable - probabilities remain unchanged
  TTable& operator+=(const TTable& rhs) {
    for (Word2Word2Double::const_iterator it = rhs.counts.begin();
         it != rhs.counts.end(); ++it) {
      const Word2Double& cpd = it->second;
      Word2Double& tgt = counts[it->first];
      for (Word2Double::const_iterator j = cpd.begin(); j != cpd.end(); ++j) {
        tgt[j->first] += j->second;
      }
    }
    return *this;
  }
  void ExportToFile(const char* filename, Dict& d) {
    std::ofstream file(filename);
    for (Word2Word2Double::iterator cit = ttable.begin();
	 cit != ttable.end(); ++cit) {
      const std::string& a = d.Convert(cit->first);
      Word2Double& cpd = cit->second;
      for (Word2Double::iterator it = cpd.begin(); it != cpd.end(); ++it) {
	const std::string& b = d.Convert(it->first);
	double c = it->second;
	file << a << '\t' << b << '\t' << c << std::endl;
      }
    }
    file.close();
  }
 public:
  Word2Word2Double ttable;
  Word2Word2Double counts;
};

#endif

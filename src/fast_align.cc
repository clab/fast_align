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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <utility>
#include <fstream>
#include <getopt.h>

#include "src/port.h"
#include "src/corpus.h"
#include "src/ttables.h"
#include "src/da.h"

using namespace std;

struct PairHash {
  size_t operator()(const pair<short,short>& x) const {
    return (unsigned short)x.first << 16 | (unsigned)x.second;
  }
};

Dict d; // integerization map

void ParseLine(const string& line,
              vector<unsigned>* src,
              vector<unsigned>* trg) {
  static const unsigned kDIV = d.Convert("|||");
  static vector<unsigned> tmp;
  src->clear();
  trg->clear();
  d.ConvertWhitespaceDelimitedLine(line, &tmp);
  unsigned i = 0;
  while(i < tmp.size() && tmp[i] != kDIV) {
    src->push_back(tmp[i]);
    ++i;
  }
  if (i < tmp.size() && tmp[i] == kDIV) {
    ++i;
    for (; i < tmp.size() ; ++i)
      trg->push_back(tmp[i]);
  }
}

string input;
string conditional_probability_filename = "";
int is_reverse = 0;
int ITERATIONS = 5;
int favor_diagonal = 0;
double prob_align_null = 0.08;
double diagonal_tension = 4.0;
int optimize_tension = 0;
int variational_bayes = 0;
double alpha = 0.01;
int no_null_word = 0;
struct option options[] = {
    {"input",             required_argument, 0,                  'i'},
    {"reverse",           no_argument,       &is_reverse,        1  },
    {"iterations",        required_argument, 0,                  'I'},
    {"favor_diagonal",    no_argument,       &favor_diagonal,    0  },
    {"p0",                required_argument, 0,                  'p'},
    {"diagonal_tension",  required_argument, 0,                  'T'},
    {"optimize_tension",  no_argument,       &optimize_tension,  1  },
    {"variational_bayes", no_argument,       &variational_bayes, 1  },
    {"alpha",             required_argument, 0,                  'a'},
    {"no_null_word",      no_argument,       &no_null_word,      1  },
    {"conditional_probabilities", required_argument, 0,          'c'},
    {0,0,0,0}
};

bool InitCommandLine(int argc, char** argv) {
  while (1) {
    int oi;
    int c = getopt_long(argc, argv, "i:rI:dp:T:ova:Nc:", options, &oi);
    if (c == -1) break;
    switch(c) {
      case 'i': input = optarg; break;
      case 'r': is_reverse = 1; break;
      case 'I': ITERATIONS = atoi(optarg); break;
      case 'd': favor_diagonal = 1; break;
      case 'p': prob_align_null = atof(optarg); break;
      case 'T': diagonal_tension = atof(optarg); break;
      case 'o': optimize_tension = 1; break;
      case 'v': variational_bayes = 1; break;
      case 'a': alpha = atof(optarg); break;
      case 'N': no_null_word = 1; break;
      case 'c': conditional_probability_filename = optarg; break;
      default: return false;
    }
  }
  if (input.size() == 0) return false;
  return true;
}

int main(int argc, char** argv) {
  if (!InitCommandLine(argc, argv)) {
    cerr << "Usage: " << argv[0] << " -i file.fr-en\n"
         << " Standard options ([USE] = strongly recommended):\n"
         << "  -i: [REQ] Input parallel corpus\n"
         << "  -v: [USE] Use Dirichlet prior on lexical translation distributions\n"
         << "  -d: [USE] Favor alignment points close to the monotonic diagonoal\n"
         << "  -o: [USE] Optimize how close to the diagonal alignment points should be\n"
         << "  -r: Run alignment in reverse (condition on target and predict source)\n"
         << "  -c: Output conditional probability table\n"
         << " Advanced options:\n"
         << "  -I: number of iterations in EM training (default = 5)\n"
         << "  -p: p_null parameter (default = 0.08)\n"
         << "  -N: No null word\n"
         << "  -a: alpha parameter for optional Dirichlet prior (default = 0.01)\n"
         << "  -T: starting lambda for diagonal distance parameter (default = 4)\n";
    return 1;
  }
  bool use_null = !no_null_word;
  if (variational_bayes && alpha <= 0.0) {
    cerr << "--alpha must be > 0\n";
    return 1;
  }
  double prob_align_not_null = 1.0 - prob_align_null;
  const unsigned kNULL = d.Convert("<eps>");
  TTable s2t, t2s;
  unordered_map<pair<short, short>, unsigned, PairHash> size_counts;
  double tot_len_ratio = 0;
  double mean_srclen_multiplier = 0;
  vector<double> probs;
  for (int iter = 0; iter < ITERATIONS; ++iter) {
    const bool final_iteration = (iter == (ITERATIONS - 1));
    cerr << "ITERATION " << (iter + 1) << (final_iteration ? " (FINAL)" : "") << endl;
    ifstream in(input.c_str());
    if (!in) {
      cerr << "Can't read " << input << endl;
      return 1;
    }
    double likelihood = 0;
    double denom = 0.0;
    int lc = 0;
    bool flag = false;
    string line;
    string ssrc, strg;
    vector<unsigned> src, trg;
    double c0 = 0;
    double emp_feat = 0;
    double toks = 0;
    while(true) {
      getline(in, line);
      if (!in) break;
      ++lc;
      if (lc % 1000 == 0) { cerr << '.'; flag = true; }
      if (lc %50000 == 0) { cerr << " [" << lc << "]\n" << flush; flag = false; }
      src.clear(); trg.clear();
      ParseLine(line, &src, &trg);
      if (is_reverse) swap(src, trg);
      if (src.size() == 0 || trg.size() == 0) {
        cerr << "Error in line " << lc << "\n" << line << endl;
        return 1;
      }
      if (iter == 0)
        tot_len_ratio += static_cast<double>(trg.size()) / static_cast<double>(src.size());
      denom += trg.size();
      probs.resize(src.size() + 1);
      if (iter == 0)
        ++size_counts[make_pair<short,short>(trg.size(), src.size())];
      bool first_al = true;  // used when printing alignments
      toks += trg.size();
      for (unsigned j = 0; j < trg.size(); ++j) {
        const unsigned& f_j = trg[j];
        double sum = 0;
        double prob_a_i = 1.0 / (src.size() + use_null);  // uniform (model 1)
        if (use_null) {
          if (favor_diagonal) prob_a_i = prob_align_null;
          probs[0] = s2t.prob(kNULL, f_j) * prob_a_i;
          sum += probs[0];
        }
        double az = 0;
        if (favor_diagonal)
          az = DiagonalAlignment::ComputeZ(j+1, trg.size(), src.size(), diagonal_tension) / prob_align_not_null;
        for (unsigned i = 1; i <= src.size(); ++i) {
          if (favor_diagonal)
            prob_a_i = DiagonalAlignment::UnnormalizedProb(j + 1, i, trg.size(), src.size(), diagonal_tension) / az;
          probs[i] = s2t.prob(src[i-1], f_j) * prob_a_i;
          sum += probs[i];
        }
        if (final_iteration) {
          double max_p = -1;
          int max_index = -1;
          if (use_null) {
            max_index = 0;
            max_p = probs[0];
          }
          for (unsigned i = 1; i <= src.size(); ++i) {
            if (probs[i] > max_p) {
              max_index = i;
              max_p = probs[i];
            }
          }
          if (max_index > 0) {
            if (first_al) first_al = false; else cout << ' ';
            if (is_reverse)
              cout << j << '-' << (max_index - 1);
            else
              cout << (max_index - 1) << '-' << j;
          }
        } else {
          if (use_null) {
            double count = probs[0] / sum;
            c0 += count;
            s2t.Increment(kNULL, f_j, count);
          }
          for (unsigned i = 1; i <= src.size(); ++i) {
            const double p = probs[i] / sum;
            s2t.Increment(src[i-1], f_j, p);
            emp_feat += DiagonalAlignment::Feature(j, i, trg.size(), src.size()) * p;
          }
        }
        likelihood += log(sum);
      }
      if (final_iteration) cout << endl;
    }

    // log(e) = 1.0
    double base2_likelihood = likelihood / log(2);

    if (flag) { cerr << endl; }
    if (iter == 0) {
      mean_srclen_multiplier = tot_len_ratio / lc;
      cerr << "expected target length = source length * " << mean_srclen_multiplier << endl;
    }
    emp_feat /= toks;
    cerr << "  log_e likelihood: " << likelihood << endl;
    cerr << "  log_2 likelihood: " << base2_likelihood << endl;
    cerr << "     cross entropy: " << (-base2_likelihood / denom) << endl;
    cerr << "        perplexity: " << pow(2.0, -base2_likelihood / denom) << endl;
    cerr << "      posterior p0: " << c0 / toks << endl;
    cerr << " posterior al-feat: " << emp_feat << endl;
    //cerr << "     model tension: " << mod_feat / toks << endl;
    cerr << "       size counts: " << size_counts.size() << endl;
    if (!final_iteration) {
      if (favor_diagonal && optimize_tension && iter > 0) {
        for (int ii = 0; ii < 8; ++ii) {
          double mod_feat = 0;
          unordered_map<pair<short,short>,unsigned,PairHash>::iterator it = size_counts.begin();
          for(; it != size_counts.end(); ++it) {
            const pair<short,short>& p = it->first;
            for (short j = 1; j <= p.first; ++j)
              mod_feat += it->second * DiagonalAlignment::ComputeDLogZ(j, p.first, p.second, diagonal_tension);
          }
          mod_feat /= toks;
          cerr << "  " << ii + 1 << "  model al-feat: " << mod_feat << " (tension=" << diagonal_tension << ")\n";
          diagonal_tension += (emp_feat - mod_feat) * 20.0;
          if (diagonal_tension <= 0.1) diagonal_tension = 0.1;
          if (diagonal_tension > 14) diagonal_tension = 14;
        }
        cerr << "     final tension: " << diagonal_tension << endl;
      }
      if (variational_bayes)
        s2t.NormalizeVB(alpha);
      else
        s2t.Normalize();
      //prob_align_null *= 0.8; // XXX
      //prob_align_null += (c0 / toks) * 0.2;
      prob_align_not_null = 1.0 - prob_align_null;
    }
  }
  if (!conditional_probability_filename.empty()) {
    cerr << "conditional probabilities: " << conditional_probability_filename << endl;
    s2t.ExportToFile(conditional_probability_filename.c_str(), d);
  }
  return 0;
}

#ifndef CPYPDICT_H_
#define CPYPDICT_H_

#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <set>
#include "src/hashtables.h"
#include "src/port.h"

class Dict {
 public:
  Dict() : b0_("<bad0>") {
    words_.reserve(1000);
  }

  inline unsigned max() const { return words_.size(); }

  static bool is_ws(char x) {
    return (x == ' ' || x == '\t');
  }

  inline void ConvertWhitespaceDelimitedLine(
      const std::string& line,
      const unsigned separator_id,
      std::vector<unsigned>* out) {
    size_t cur = 0;
    size_t last = 0;
    int state = 0;
    out->clear();
    while(cur < line.size()) {
      const char cur_char = line[cur++];
      if (is_ws(cur_char)) {
        if (state == 1) {
          out->push_back(Convert(line.substr(last, cur - last - 1)));
          state = 0;
        }
        if (cur_char == '\t') out->push_back(separator_id);
      } else {
        if (state == 1) continue;
        last = cur - 1;
        state = 1;
      }
    }
    if (state == 1)
      out->push_back(Convert(line.substr(last, cur - last)));
  }

  inline unsigned Convert(const std::string& word, bool frozen = false) {
    MAP_TYPE::iterator i = d_.find(word);
    if (i == d_.end()) {
      if (frozen)
        return 0;
      words_.push_back(word);
      d_[word] = words_.size();
      return words_.size();
    } else {
      return i->second;
    }
  }

  inline const std::string& Convert(const unsigned id) const {
    if (id == 0) return b0_;
    return words_[id-1];
  }
 private:
  std::string b0_;
  std::vector<std::string> words_;
  MAP_TYPE d_;
};

#endif

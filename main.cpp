// Initial approach (unordered_map<string, float>):
//    ~39,700 attempts/s
// Represent ngram as int (WXYZ -> W*26^3 + X*26^2 + Y*26 + Z):
//    ~69,000 attempts/s
// Represent ngram in fixed array (of size 26^4):
//    ~735,000 attempts/s
// Re-use plaintext string:
//    ~950,000 attempts/s

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
#include <random>

using char_t = std::uint8_t;
using Key = std::array<char_t, 26>;
using ngram_t = std::uint32_t;


char_t char_repr(const char c) {
  return static_cast<char_t>(c - 'A');
}

ngram_t ngram_repr(const std::string_view ngram) {
  return char_repr(ngram[0]) * 26 * 26 * 26 +
         char_repr(ngram[1]) * 26 * 26 +
         char_repr(ngram[2]) * 26 +
         char_repr(ngram[3]);
}

Key alphabet_to_key(const std::string& key) {
  assert(key.size() == 26);
  Key out;
  for (char_t src = 0; src < 26; ++src) {
    const char_t dst = char_repr(key[src]);
    out[src] = dst;
  }
  return out;
}

std::string key_to_alphabet(const Key& key) {
  std::string out;
  out.reserve(26);
  for (char_t src = 0; src < 26; ++src) {
    const char c = 'A' + key[src];
    out.push_back(c);
  }
  return out;
}

Key get_decrypt_key(const Key& key) {
  Key decrypt_key;
  for (char_t dst = 0; dst < 26; ++dst) {
    const char_t src = key[dst];
    decrypt_key[src] = dst;
  }
  return decrypt_key;
}

void decrypt(const std::string& ciphertext, const Key& key,
             std::string* plaintext) {
  const Key decrypt_key = get_decrypt_key(key);
  for (std::string::size_type i = 0; i < ciphertext.size(); ++i) {
    const char_t in = char_repr(ciphertext[i]);
    (*plaintext)[i] = 'A' + decrypt_key[in];
  }
}

class Fitness {
 private:
  using ngrams_t = std::array<float, 26*26*26*26>;

 public:
  Fitness(std::istream& in, const float floor_percentage=0.01,
          const int ref_top_ngrams=1000)
    : ngrams_{std::log10(floor_percentage)} {
    std::uint64_t total = 0;

    std::string ngram;
    std::uint64_t count;
    int num_ngrams = 0;
    std::vector<ngram_t> top_ngrams;
    top_ngrams.reserve(ref_top_ngrams);
    while (in >> ngram >> count) {
      const ngram_t ngram_int = ngram_repr(ngram);
      ngrams_[ngram_int] = std::log10(count);
      total += count;
      if (num_ngrams < ref_top_ngrams) {
        top_ngrams.push_back(ngram_int);
      }
      ++num_ngrams;
    }
    n_ = ngram.size();
    const float norm = std::log10(total);
    for (ngrams_t::size_type i = 0; i < ngrams_.size(); ++i) {
      ngrams_[i] -= norm;
    }
    // Based on https://planetcalc.com/8045/
    ref_normalized_ = 0.f;
    for (const auto ngram : top_ngrams) {
      ref_normalized_ += ngrams_[ngram];
    }
    ref_normalized_ /= ref_top_ngrams;
  }

  float score(const std::string_view text) const {
    float fitness = 0.f;
    std::string_view::size_type i = 0;
    for (; i + n_ - 1 < text.size(); ++i) {
      const std::string_view ngram = text.substr(i, n_);
      fitness += ngrams_[ngram_repr(ngram)];
    }
    fitness /= i;
    return std::abs(fitness - ref_normalized_);
  }

 private:
  ngrams_t ngrams_;
  std::uint8_t n_;
  float ref_normalized_;
};

int main() {
  std::ifstream ifs{"english_quadgrams.txt"};
  const Fitness fit{ifs};

  // Used in benchmarks:
  const std::string ciphertext =
    "SOWFBRKAWFCZFSBSCSBQITBKOWLBFXTBKOWLSOXSOXFZWWIBICFWUQLRXINOCIJLWJFQUNWXLF"
    "BSZXFBTXAANTQIFBFSFQUFCZFSBSCSBIMWHWLNKAXBISWGSTOXLXTSWLUQLXJBUUWLWISTBKOW"
    "LSWGSTOXLXTSWLBSJBUUWLFULQRTXWFXLTBKOWLBISOXSSOWTBKOWLXAKOXZWSBFIQSFBRKANS"
    "OWXAKOXZWSFOBUSWJBSBFTQRKAWSWANECRZAWJ";

  std::default_random_engine rand_engine;
  std::uniform_int_distribution<char_t> dist(0, 25);

  std::string plaintext = ciphertext;

  Key best_key = alphabet_to_key("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  float best_score = std::numeric_limits<float>::infinity();

  Key parent_key = best_key;
  float parent_score = best_score;

  const auto start_time = std::chrono::high_resolution_clock::now();
  std::uint64_t total_count = 0;
  while (true) {
    std::shuffle(parent_key.begin(), parent_key.end(), rand_engine);
    decrypt(ciphertext, parent_key, &plaintext);
    parent_score = fit.score(plaintext);

    for (std::uint64_t count = 0; count < 1000; ++count) {
      if (total_count > 0 && total_count % 5000000 == 0) {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto seconds = std::chrono::duration<float>(now - start_time);
        const float speed = total_count / seconds.count();
        std::cout << "[" << speed << " attempts/s]" << std::endl;
      }
      Key child = parent_key;
      std::swap(child[dist(rand_engine)], child[dist(rand_engine)]);
      decrypt(ciphertext, child, &plaintext);
      const float score = fit.score(plaintext);
      if (score < parent_score) {
        parent_score = score;
        parent_key = child;
        count = 0;
      }
      ++total_count;
    }

    if (parent_score < best_score) {
      std::cout << "New best score!! " << parent_score << std::endl;
      decrypt(ciphertext, parent_key, &plaintext);
      std::cout << "Plaintext: " << plaintext << std::endl;
      std::cout << "Going from " << key_to_alphabet(best_key) << " to "
                << key_to_alphabet(parent_key) << "\n" << std::endl;
      best_score = parent_score;
      best_key = parent_key;
    }
  }

  return 0;
}

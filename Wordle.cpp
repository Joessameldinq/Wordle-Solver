#include "Wordle.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <omp.h>

//  Constants 

static constexpr int NUM_PATTERNS   = 243; // 3^5
static constexpr int SOLVED_PATTERN = 242; // "ggggg": 2*(1+3+9+27+81)

//  Constructor 

Wordle::Wordle(std::vector<std::string> dictionary,
               std::vector<std::string> targets)
    : dict_words(dictionary),
      target_words(targets),
      dictionaryLen((int)dictionary.size()),
      targetsLen((int)targets.size()),
      activeTargetsCount((int)targets.size())
{
    dict_map.reserve(dictionaryLen);
    target_map.reserve(targetsLen);

    for (int i = 0; i < dictionaryLen; i++)
        dict_map[dictionary[i]] = i;
    for (int i = 0; i < targetsLen; i++)
        target_map[targets[i]] = i;

    targets_deactivator.assign(targetsLen, 1);
    precomputed_patterns.assign(dictionaryLen, std::vector<int>(targetsLen, 0));
}


int Wordle::compute_pattern(const std::string& guess, const std::string& secret) {
    int guess_used[5]  = {0};
    int secret_used[5] = {0};
    int index = 0;

    // First pass: greens
    for (int l = 0; l < 5; l++) {
        if (guess[l] == secret[l]) {
            index       += (int)std::pow(3, l) * 2;
            guess_used[l]  = 1;
            secret_used[l] = 1;
        }
    }

    // Second pass: yellows
    for (int l = 0; l < 5; l++) {
        if (guess_used[l] == 1) continue;
        for (int ll = 0; ll < 5; ll++) {
            if (secret_used[ll] == 1 || guess[l] != secret[ll]) continue;
            index += (int)std::pow(3, l);
            secret_used[ll] = 1;
            break;
        }
    }
    return index;
}

//  Precompute Matrix 

void Wordle::precompute_matrix() {
    std::cout << "Computing " << (long)dictionaryLen * targetsLen
              << " patterns (once)...\n";

    for (int gi = 0; gi < dictionaryLen; gi++) {
        for (int ti = 0; ti < targetsLen; ti++) {
            precomputed_patterns[gi][ti] =
                compute_pattern(dict_words[gi], target_words[ti]);
        }
    }

    std::cout << "Done!\n";
}

//  calculate_entropies 
//   - only count active targets (deactivator == 1)
//   - add tiny nudge +0.00001 if pattern 242 (ggggg) is reachable
//     so that valid-target guesses are preferred on ties

std::vector<float> Wordle::calculate_entropies() const {
    std::vector<float> entropies(dictionaryLen, 0.0f);
    float remained = (float)activeTargetsCount;
    if (remained == 0) return entropies;

    for (int gi = 0; gi < dictionaryLen; gi++) {
        int buckets[NUM_PATTERNS] = {0};

        for (int ti = 0; ti < targetsLen; ti++) {
            if (targets_deactivator[ti] == 1)
                buckets[precomputed_patterns[gi][ti]]++;
        }

        float entropy = 0.0f;
        for (int p = 0; p < NUM_PATTERNS; p++) {
            if (buckets[p] == 0) continue;
            float prob = (float)buckets[p] / remained;
            entropy -= prob * std::log2f(prob);
        }
        if (buckets[SOLVED_PATTERN] > 0)
            entropy += 0.00001f;

        entropies[gi] = entropy;
    }

    return entropies;
}

//  print_top_n 

void Wordle::print_top_n(int n) const {
    auto entropies = calculate_entropies();

    // Build index array and partial-sort to get top-n
    std::vector<int> indices(dictionaryLen);

    // Fill v with values 0, 1, 2, ..., dictionaryLen-1
    std::iota(indices.begin(), indices.end(), 0);
    int top = std::min(n, dictionaryLen);
    std::partial_sort(indices.begin(), indices.begin() + top, indices.end(),
                      [&](int a, int b){ return entropies[a] > entropies[b]; });

    for (int i = 0; i < top; i++) {
        int   idx = indices[i];
        float e   = entropies[idx];
        if (e == 0.0f) break; // nothing left to show

        float display_e = (std::fabs(e) == 0.00001f) ? 0.0f : std::fabs(e);
        std::printf("%2d. %s | entropy = %.6f\n",
                    i + 1, dict_words[idx].c_str(), display_e);
    }
    std::cout << "BEST=<" << dict_words[indices[0]] << ">\n"; 
}

//  pattern_str_to_code 
//   g -> 2 * 3^i,  Y -> 1 * 3^i,  B -> 0

int Wordle::pattern_str_to_code(const std::string& feedback) {
    int code = 0;
    for (int i = 0; i < 5; i++) {
        char c = feedback[i];
        if      (c == 'g') code += 2 * (int)std::pow(3, i);
        else if (c == 'y') code += 1 * (int)std::pow(3, i);
        // 'r' contributes 0
    }
    return code;
}

std::string Wordle::pattern_code_to_str(int code) {
    static const char mp[] = {'r', 'y', 'g'};
    std::string res(5, ' ');
    for (int i = 0; i < 5; i++) {
        res[i] = mp[code % 3];
        code  /= 3;
    }
    return res;
}

//  filter 
//   targets_deactivator[computed_data[word_idx] != feedback_value] = 0

void Wordle::filter(int guess_idx, int feedback_code) {
    activeTargetsCount = 0;
    for (int ti = 0; ti < targetsLen; ti++) {
        if (targets_deactivator[ti] == 0) continue;
        if (precomputed_patterns[guess_idx][ti] != feedback_code)
            targets_deactivator[ti] = 0;
        else
            activeTargetsCount++;
    }
}

//  solve ─

void Wordle::solve() {
    static constexpr int TOP_N     = 20;
    static constexpr int MAX_TURNS = 6;

    int current_round = 1;

    while (true) {
        int remained = activeTargetsCount;

        if (remained == 0) {
            std::cout << "Some input was wrong, please try again.\n";
            break;
        }

        std::cout << "\n==== Round " << current_round << " ====\n";
        std::cout << "Remaining possible answers: " << remained << "\n";
        std::cout << "Current answer-set entropy: "
                  << std::log2f((float)remained) << "\n";
        std::cout << "\nTop suggestions:\n";
        print_top_n(TOP_N);


        //  Read guess word 
        std::string word;
        std::cout << "\nEnter your played guess word (or 'quit'): ";
        std::cin >> word;
        // lowercase
        for (char& c : word) c = (char)std::tolower((unsigned char)c);

        while (true) {
            if (word == "quit") return;
            if (dict_map.count(word)) break;
            std::cout << "Wrong input! Enter your played guess word again (or 'quit'): ";
            std::cin >> word;
            for (char& c : word) c = (char)std::tolower((unsigned char)c);
        }

        //  Read feedback ─
        std::string feedback;
        std::cout << "Enter your played guess output: ";
        std::cin >> feedback;
        for (char& c : feedback) c = (char)std::tolower((unsigned char)c);

        while (true) {
            bool valid = (feedback.size() == 5);
            if (valid)
                for (char c : feedback)
                    if (c != 'r' && c != 'y' && c != 'g') { valid = false; break; }
            if (valid) break;
            std::cout << "Wrong input! Enter your played guess word output again: ";
            std::cin >> feedback;
            for (char& c : feedback) c = (char)std::toupper((unsigned char)c);
        }

        //  Apply filter 
        int guess_idx    = dict_map.at(word);
        int feedback_code = pattern_str_to_code(feedback);
        filter(guess_idx, feedback_code);
        current_round++;

        //  Win check 
        if (feedback == "ggggg") {
            std::cout << "You Won!\n";
            break;
        }

        if (current_round > MAX_TURNS) {
            std::cout << "Could not solve within " << MAX_TURNS << " turns.\n";
            break;
        }
    }
}
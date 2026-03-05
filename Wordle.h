#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

class Wordle {
public:
    //  Construction 
    Wordle(std::vector<std::string> dictionary,
           std::vector<std::string> targets);

    //  Setup 
    void precompute_matrix();

    //  Solving 
    void solve();
    void filter(int guess_idx, int feedback_code);
    void print_top_n(int n) const;

    //  Utilities 
    static int compute_pattern(const std::string& guess, const std::string& secret);
    static int pattern_str_to_code(const std::string& feedback); // "GBYYB" -> int
    static std::string pattern_code_to_str(int code);                    // int -> "GBYYB"

    int getActiveTargetsCount() const { return activeTargetsCount; }

private:
    //  Word storage 
    std::vector<std::string>            dict_words;   // index → word
    std::vector<std::string>            target_words; // index → word
    std::unordered_map<std::string,int> dict_map;     // word  → index
    std::unordered_map<std::string,int> target_map;   // word  → index

    int dictionaryLen      = 0;
    int targetsLen         = 0;
    int activeTargetsCount = 0;

    // targets_deactivator[ti] = 1 active, 0 eliminated  
    std::vector<int> targets_deactivator;

    // precomputed_patterns[dict_idx][target_idx]
    std::vector<std::vector<int>> precomputed_patterns;

    //  Helpers 
    std::vector<float> calculate_entropies() const;
};
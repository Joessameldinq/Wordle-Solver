#include "Wordle.h"
#include <fstream>
#include <iostream>
#include <set>
std::vector<std::string> load_words(const std::string& file_path){
    std::ifstream in(file_path);
    std::string line;
    std::vector<std::string> words;
    while(std::getline(in,line)){
        words.push_back(line);
    }
    in.close();
    return words;
}
int main(){
    std::vector<std::string> dictionary , targets;
    dictionary = load_words("dictionary_5_letter.txt");
    targets = load_words("targets_5_letter.txt");


    Wordle bot(dictionary,targets);
    bot.precompute_matrix();
    bot.solve();
}
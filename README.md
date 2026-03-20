# Wordle Solver

An entropy-based Wordle solver built in C++. The idea is simple: at every turn, pick the guess that gives you the most information about the answer.

---

## Background

I built this as a way to apply information theory to a concrete problem. Wordle gives you colour-coded feedback after each guess, and the goal of the solver is to use that feedback as efficiently as possible — eliminating as many candidates as quickly as it can.

The core metric is Shannon entropy. For each candidate guess, the solver asks: if I played this word, how evenly would the remaining answers be spread across all possible feedback patterns? A guess that splits the remaining words into many roughly equal groups is better than one that leaves most of them in the same bucket.

$$H(g) = -\sum_{p} P(p \mid g) \log_2 P(p \mid g)$$

Ties are broken in favour of words that are themselves valid answers, since guessing one of those gives you a free win if it happens to be correct.

---

## Feedback Encoding

Feedback is encoded as a base-3 integer, reading left to right:

| Colour | Symbol | Contribution |
|--------|--------|--------------|
| Green  | G      | 2 * 3^i      |
| Yellow | Y      | 1 * 3^i      |
| Black  | R      | 0            |

So "GGGGG" encodes to 2*(1+3+9+27+81) = 242. All dictionary-by-target pattern values are precomputed once into a matrix, so filtering and scoring during the game are both just a single pass over an array.

---

## Features

- Full pattern matrix precomputed at startup
- Top 20 suggestions printed each turn with their entropy values
- Input validation for both the guess word and the feedback string
- Play-again loop between games

---

## Project Structure

```
.
├── Wordle.h                     # Class interface
├── Wordle.cpp                   # Solver logic
├── main.cpp                     # Entry point, JSON , game loop
├── targets_5_letter.txt        # Valid answer words
└── dictionary_5_letter.txt     # Full guess word list
```

---

## Building and Running

Requirements:
- C++17 (GCC or Clang)

Build:
```bash
g++ -std=c++17 main.cpp Wordle.cpp -o wordle
```

Run:
```bash
./wordle
```

The pattern matrix is computed on startup, then the game loop begins:

```
Computing 14000000 patterns (once)...
Done!

==== Round 1 ====
Remaining possible answers: 2315
Current answer-set entropy: 11.177

Top suggestions:
 1. salet | entropy = 5.888765
 2. reast | entropy = 5.867392
 3. crate | entropy = 5.849103
 ...

Enter your played guess word (or 'quit'): salet
Enter your played guess output: BYBGB
```

---


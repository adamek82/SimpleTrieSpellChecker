# SimpleTrieSpellChecker

A portable C spell checker using a compact trie for fast exact lookup and **Damerau-1** (one-edit) suggestions.

> **Attribution & context:**  
> This codebase is **heavily based on** the trie implementation from *Data Structures in C* by **Adam Drozdek** and **Donald L. Simon** (Duquesne University).  
> Original reference sources (code bundle): https://duq.app.box.com/s/rqc4hb1jntoehj1d55hvqlbms6ulx4ce  
> I made **substantial fixes and refactors**: corrected prefix-split logic, removed spurious levels, ensured minimal structure, added suggestions (Damerau-1), and hardened memory handling (no UB on temporary empty non-leaves).

---

## Project Structure

```
SimpleTrieSpellChecker/
├── include/
│   └── SimpleTrieSpellChecker.h   # public API (no private helpers exported)
├── src/
│   ├── SimpleTrieSpellChecker.c   # trie engine + search + suggestions
│   └── main.c                     # demo: build dictionary, check text, print suggestions
├── dictionary                     # sample dictionary (one token per line)
├── text                           # sample input text to spell-check (optional)
├── CMakeLists.txt
└── bin/ build/ .vscode/           # build outputs / build tree / VS Code configs (tracked)
```

> **Note:** The `.vscode/` folder is intentionally checked in (launch/tasks/settings for VS Code).

---

## Data Structures

### Compact Trie (Non-Leaf + Leaf with path compression)
- **NonLeafNode**
  - `letters`: sorted array of distinct outgoing edge labels.
  - `ptrs`: parallel array of child pointers.
  - `EndOfWord`: marks that a word terminates exactly at this node.
- **LeafNode**
  - `word`: **entire remaining suffix** of the dictionary entry (possibly empty).  
  - This is a **path-compression** trick: traversal consumes one character per level until a leaf, then compares the whole tail.

**Why this layout?**  
It minimizes node count and memory traffic. Inserts only split when necessary, and many words share upper prefixes.

---

## Algorithms

### Exact Lookup
- Walk the trie consuming one character per level.
- If a leaf is reached, compare the remaining input with the leaf’s `word`.
- If the input ends on a non-leaf, return success iff `EndOfWord == yes`.

### Insert (with leaf splitting)
When descending into a leaf, split it against the new word’s tail:
1. **Shared-prefix loop** builds a short chain of non-leaves for equal letters.
2. **Prefix cases (fixed vs. original):**
   - **New is a prefix of old** (e.g., `ARE` vs `AREA…`): replace the leaf with an **empty** non-leaf, set `EndOfWord` **on that new node**, attach **one** branch for the old remainder, free the old leaf.
   - **Old is a prefix of new** (e.g., `ARE` vs `AREA`): same idea, but attach the new remainder.
3. **First mismatch**: attach **two** branches (one for each differing continuation) and free the old leaf.

> **Invariant note:** Prefix cases briefly create an **empty** non-leaf (no `letters/ptrs` yet). Helpers are **null-safe** and treat this as length 0. A subsequent `CreateLeaf(...)` immediately restores the invariant before `TrieInsert` returns.

### Suggestions (Damerau-Levenshtein ≤ 1)
- Depth-first traversal with at most **one edit** among:
  - substitution, insertion, deletion, adjacent transposition (swap `i` and `i+1`).
- For leaves, a tail matcher validates the remainder in O(len) with ≤1 edit used.
- Results are deduplicated and limited to a configurable cap.

---

## Building & Running

```bash
# configure out-of-source
cmake -S . -B build
# build
cmake --build build
# run (MSVC places exe under bin/Debug or bin/Release)
./bin/Debug/SimpleTrieSpellChecker   # Windows (example)
# or
./build/SimpleTrieSpellChecker       # Unix-like
```

**MSVC note:** The project defines `_CRT_SECURE_NO_WARNINGS` to keep portable `fopen/fscanf` without vendor “secure CRT” warnings.

---

## Usage

1. Prepare a `dictionary` file with one word per token (whitespace-separated).  
   The demo program uppercases all tokens before inserting.
2. Optionally prepare a `text` file; non-letters are skipped, words are uppercased and checked.
3. Run the executable; it prints a side view of the trie and reports misspellings with suggestions.

**Quick test text (no punctuation):**

```
computer cmputer coomputer combuter copmuter compuuter are ar aer area peer peeer per pear pier ire ipe eire rear rera
```

Expected behavior:
- All single-edit variants of `COMPUTER` suggest `COMPUTER`.
- `AR` suggests `A/ARA/ARE` etc.
- `AER` suggests `ARE` (transposition) and possibly `PER` (substitution).

---

## Key Fixes vs. Original Source

- **Prefix split correctness:** No extra “dummy” levels; `EndOfWord` set exactly on the split node.  
- **Memory safety:** Old leaves are freed after being disconnected; helpers handle temporary empty non-leaves (`letters == NULL`).  
- **Off-by-one in split loop:** Added the missing `offset--` before branching at the first mismatch.  
- **Private API:** Internal helpers (`CreateNonLeaf`, `CreateLeaf`, `CreateEmptyNonLeaf`) are `static`; header exposes only what clients need.  
- **Suggestions:** Added Damerau-1 suggestion engine with tight tail matching.

---

## Complexity

- **Lookup:** O(L) average (L = word length), plus O(tail) only at leaves.  
- **Insert:** O(L) to walk/split; memory proportional to number of distinct branching letters and leaf tails.  
- **Suggest (≤1 edit):** Prunes aggressively by edits_used; worst-case exponential in branching factor, but bounded well for natural dictionaries and small `maxSuggestions`.

---

## Roadmap

- Optional ranking of suggestions (by common prefix length, edit type, frequency).  
- Dictionary serialization (save/load).  
- Unicode support (currently ASCII uppercasing) and locale-aware case mapping.

---

## Acknowledgments

- Based on the trie design from *Data Structures in C* by **Adam Drozdek** & **Donald L. Simon** (Duquesne).  
  Original code archive: https://duq.app.box.com/s/rqc4hb1jntoehj1d55hvqlbms6ulx4ce  
- This repository includes **significant corrections and extensions** to the original examples.


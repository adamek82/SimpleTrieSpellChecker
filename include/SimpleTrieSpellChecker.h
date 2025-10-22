#pragma once
#include <stddef.h>

/* ================================================================ *
 * Compact trie with path-compressed leaves + Damerau-1 suggest     *
 *                                                                  *
 * Data model:                                                      *
 * - Non-leaf node keeps a sorted array of outgoing edge labels     *
 *   (`letters`) and a parallel array of child pointers (`ptrs`).   *
 * - Leaf node stores the ENTIRE remaining suffix of a dictionary   *
 *   word (possibly ""), i.e., path compression at the last hop.    *
 *                                                                  *
 * Consequences:                                                    *
 * - Traversal compares one input char per trie level until a leaf  *
 *   is reached; then the whole remaining tail is compared.         *
 * - Insert may need to "split" a leaf into a small chain of        *
 *   non-leaves plus two leaves (one for old tail, one for new).    *
 * ================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* Node tags and constants (kept compatible with the original code) */
#define leaf       1u
#define yes        1u
#define success    1
#define notFound  -1

/* Forward decls and typedefs */
struct NonLeafNode;
typedef struct NonLeafNode *NonLeafPtr;

struct LeafNode;
typedef struct LeafNode *LeafPtr;

/* Non-leaf node: compact edge set + children */
struct NonLeafNode {
    unsigned int kind      : 1;   /* 0 => non-leaf, 1 => leaf */
    unsigned int EndOfWord : 1;   /* marks end of a word exactly at this node */
    char *letters;                /* sorted distinct edge labels */
    NonLeafPtr *ptrs;             /* parallel children array */
};

/* Leaf node: whole remaining suffix (path compression) */
struct LeafNode {
    unsigned int kind : 1;        /* == leaf */
    char *word;                   /* remaining suffix; "" is allowed */
};

/* -------------------------
 * Public helper structures
 * ------------------------- */

/* Small suggestion container (owned strings) */
typedef struct {
    char **items;
    int   count;
    int   cap;
} SuggestBox;

/* -------------------------
 * Public API
 * ------------------------- */

/* Uppercase in-place (ASCII), returns s; mirrored original strupr behavior. */
char *strupr_local(char *s);

/* Diagnostics helper; aborts with a message. */
void Error(char *s);

/* Create a fresh trie and seed it with the FIRST already UPPERCASED word.
 * Returns the root (non-leaf). */
NonLeafPtr TrieCreateWithFirstWord(const char *upper_word);

/* Insert an UPPERCASED word into the trie rooted at `root`.
 * Handles splitting of leaves; keeps structure minimal:
 * - in prefix cases, marks EndOfWord on the CURRENT node,
 *   adds exactly one new branch for the longer word's remainder,
 *   and frees the disconnected old leaf. */
void TrieInsert(char *word, NonLeafPtr root);

/* Exact search: returns success (1) if in dictionary; 0 otherwise. */
int SearchTrie(NonLeafPtr root, char *word);

/* Side-view printer for debugging. */
void TrieSideView(int depth, NonLeafPtr p, char *prefix);

/* Suggestions (Damerau-Levenshtein distance <= 1).
 * Produces up to `maxSuggestions` dictionary words close to `upper_word`.
 * Owns memory inside SuggestBox; free with FreeSuggestBox. */
void SuggestCorrections(NonLeafPtr root, const char *upper_word,
                        int maxSuggestions, SuggestBox *outBox);

/* Free all strings inside SuggestBox and the array itself. */
void FreeSuggestBox(SuggestBox *box);

#ifdef __cplusplus
}
#endif

#include "SimpleTrieSpellChecker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================= *
 * Utilities / basic helpers *
 * ========================= */

void Error(char *s) {
    puts(s);
    exit(1);
}

char *strupr_local(char *s) {
    char *ss = s;
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
    return ss;
}

/* Find position of `ch` in p->letters (sorted). Returns index or notFound. */
static int Position(NonLeafPtr p, char ch) {
    int len = (int)strlen(p->letters);
    for (int i = 0; i < len; ++i)
        if (p->letters[i] == ch) return i;
    return notFound;
}

/* ========================= *
 * Memory / node factories   *
 * ========================= */

/* Expand letters/ptrs by inserting edge `ch` at index `stop`.
 * Copies both lower (<stop) and upper (>=stop) portions from old buffers. */
static void AddCell(char ch, NonLeafPtr p, int stop) {
    int len = (int)strlen(p->letters);
    char *old_letters = p->letters;
    NonLeafPtr *old_ptrs = p->ptrs;

    /* allocate new, bigger buffers; calloc() zeros new ptrs slots */
    p->letters = (char*)malloc((size_t)len + 2);
    p->ptrs    = (NonLeafPtr*)calloc((size_t)(len + 1), sizeof(NonLeafPtr));
    if (!p->letters || !p->ptrs) Error("out of memory: AddCell");

    /* shift the upper half (> stop-1) by +1 */
    for (int i = len; i >= stop + 1; --i) {
        p->ptrs[i]    = old_ptrs[i - 1];
        p->letters[i] = old_letters[i - 1];
    }

    /* insert the new edge */
    p->letters[stop] = ch;

    /* copy the lower half [0..stop-1] 1:1 */
    for (int i = stop - 1; i >= 0; --i) {
        p->ptrs[i]    = old_ptrs[i];
        p->letters[i] = old_letters[i];
    }

    p->letters[len + 1] = '\0';
    free(old_letters);
    free(old_ptrs);
}

NonLeafPtr CreateNonLeaf(char ch) {
    NonLeafPtr p = (NonLeafPtr)calloc(1, sizeof(*p));
    if (!p) Error("out of memory: CreateNonLeaf");

    p->kind = !leaf;         /* 0 => non-leaf */
    p->EndOfWord = !yes;     /* 0 => false */
    p->ptrs = (NonLeafPtr*)calloc(1, sizeof(NonLeafPtr));
    p->letters = (char*)malloc(2);
    if (!p->ptrs || !p->letters) Error("out of memory: CreateNonLeaf buffers");

    p->letters[0] = ch;
    p->letters[1] = '\0';
    return p;
}

void CreateLeaf(char ch, char *suffix, NonLeafPtr p) {
    int pos = Position(p, ch);
    int len = (int)strlen(p->letters);

    LeafPtr lf = (LeafPtr)malloc(sizeof(*lf));
    if (!lf) Error("out of memory: CreateLeaf node");

	size_t n = strlen(suffix);
    lf->word = (char*)malloc(n + 1);
    if (!lf->word) Error("out of memory: CreateLeaf word");
    memcpy(lf->word, suffix, n + 1);
    lf->kind = leaf;

    if (pos == notFound) {
        /* choose sorted position for `ch` */
        for (pos = 0; pos < len && p->letters[pos] < ch; ++pos);
        AddCell(ch, p, pos);
    }
    p->ptrs[pos] = (NonLeafPtr)lf;
}

/* ========================= *
 * Search & debug display    *
 * ========================= */

int SearchTrie(NonLeafPtr root, char *word) {
    NonLeafPtr p = root;
    LeafPtr lf;
    int pos;

    while (1) {
        if (p->kind == leaf) {
            /* leaf holds the entire remaining suffix */
            lf = (LeafPtr)p;
            return strcmp(word, lf->word) == 0 ? success : !success;
        } else if (*word == '\0') {
            /* end of input must coincide with EndOfWord flag */
            return (p->EndOfWord == yes) ? success : !success;
        } else if ((pos = Position(p, *word)) != notFound && p->ptrs[pos]) {
            p = p->ptrs[pos];
            ++word;
        } else {
            return !success;
        }
    }
}

void TrieSideView(int depth, NonLeafPtr p, char *prefix) {
    if (p->kind == leaf) {
        LeafPtr lf = (LeafPtr)p;
        for (int i = 0; i < depth; ++i) printf("   ");
        printf(" >>%s|%s\n", prefix, lf->word);
    } else {
        int n = (int)strlen(p->letters);
        for (int i = n - 1; i >= 0; --i) {
            if (p->ptrs[i]) {
                prefix[depth] = p->letters[i];
                prefix[depth + 1] = '\0';
                TrieSideView(depth + 1, p->ptrs[i], prefix);
            }
        }
        if (p->EndOfWord == yes) {
            prefix[depth] = '\0';
            for (int i = 0; i < depth + 1; ++i) printf("   ");
            printf(">>>%s|\n", prefix);
        }
    }
}

/* ===================================================================================== *
 * Insert (with fixed prefix cases)                                                      *
 * ===================================================================================== *
 *                                                                                       *
 * When descending edge `*word` hits a LEAF (holding `lf->word`), we split:              *
 * - Build a chain of non-leaves for the SHARED prefix between (word[1..] and lf->word)  *
 *   by reassigning the parent's child pointer (this disconnects the old leaf).          *
 * - Then there are 3 outcomes:                                                          *
 *   (A) new word ends at this node (new is prefix of old) => mark EndOfWord here,       *
 *       and attach only OLD remainder as one branch; free the old leaf.                 *
 *   (B) old leaf ends at this node (old is prefix of new) => mark EndOfWord here,       *
 *       and attach only NEW remainder as one branch; free the old leaf.                 *
 *   (C) both continue with different next letters => attach TWO branches                *
 *       (one for NEW remainder, one for OLD remainder); free the old leaf.              *
 *                                                                                       *
 * Importantly, in (A) and (B) we DO NOT create an extra non-leaf level.                 *
 * EndOfWord must be set on the CURRENT node `p`, keeping the trie minimal.              *
 * ===================================================================================== */
void TrieInsert(char *word, NonLeafPtr root) {
    NonLeafPtr p = root;
    int pos;

    while (1) {
        if (*word == '\0') {
            /* end of input: toggle/confirm EndOfWord at current non-leaf */
            if (p->EndOfWord == yes) {
                /* duplicate: harmless */
            } else {
                p->EndOfWord = yes;
            }
            return;
        }

        pos = Position(p, *word);
        if (pos == notFound) {
            /* missing edge: add as leaf carrying the whole remaining suffix */
            CreateLeaf(*word, word + 1, p);
            return;
        }

        if (p->ptrs[pos]->kind == leaf) {
            /* Split this leaf. */
            LeafPtr lf = (LeafPtr)p->ptrs[pos];

            /* Fast duplicate: word equals existing leaf's tail. */
            if (strcmp(lf->word, word + 1) == 0) {
                /* duplicate entry; nothing to do */
                return;
            }

            int offset = 0;

            /* Build non-leaves along the shared prefix.
             * We repeatedly REPLACE the parent's child pointer with a new non-leaf,
             * disconnecting `lf` (which we'll free later). */
            do {
                pos = Position(p, word[offset]); /* edge label at this parent */

				/* CASE A: new word finishes at this split point (new is a prefix of old)
				Example (after descending first letter): word="ARE", lf->word="REA..." */
				if ((int)strlen(word) == offset + 1) {
					/* Replace the leaf with a non-leaf for the LAST shared letter (word[offset]).
					EndOfWord must be set on THIS new node to mark the end of the NEW word. */
					p->ptrs[pos] = CreateNonLeaf(word[offset]);
					p->ptrs[pos]->EndOfWord = yes;

					/* Attach the remaining OLD tail as a single branch:
					edge lf->word[offset] with leaf suffix lf->word + offset + 1 */
					CreateLeaf(lf->word[offset], lf->word + offset + 1, p->ptrs[pos]);

					/* The old leaf is now disconnected: free it. */
					free(lf->word);
					free(lf);
					return;
				}

				/* CASE B: old leaf finishes at this split point (old is a prefix of new)
				Example (after descending first letter): word="AREA", lf->word="RE" */
				else if ((int)strlen(lf->word) == offset) {
					/* Replace the leaf with a non-leaf for the NEXT letter of the NEW word (word[offset+1]).
					EndOfWord must be set on THIS new node to preserve the end of the OLD word. */
					p->ptrs[pos] = CreateNonLeaf(word[offset + 1]);
					p->ptrs[pos]->EndOfWord = yes;

					/* Attach the remaining NEW tail as a single branch:
					edge word[offset+1] with leaf suffix word + offset + 2 */
					CreateLeaf(word[offset + 1], word + offset + 2, p->ptrs[pos]);

					/* The old leaf is now disconnected: free it. */
					free(lf->word);
					free(lf);
					return;
				}

                /* Otherwise, still in shared-prefix territory:
                 * replace parent's child with a non-leaf for the NEXT new letter. */
                p->ptrs[pos] = CreateNonLeaf(word[offset + 1]);
                p = p->ptrs[pos];
                ++offset;

            } while (word[offset] == lf->word[offset - 1]);

			/* step back to the last shared position */
			offset--;

            /* We stopped at the first differing next letter:
             * word[offset+1] vs lf->word[offset].
             * Attach TWO branches (NEW and OLD remainders). */
            /* NEW branch */
            CreateLeaf(word[offset + 1], word + offset + 2, p);
            /* OLD branch */
            CreateLeaf(lf->word[offset], lf->word + offset + 1, p);

            /* old leaf was disconnected earlier; safe to free now */
            free(lf->word);
            free(lf);
            return;
        }

        /* Child is a non-leaf: descend normally. */
        p = p->ptrs[pos];
        ++word;
    }
}

/* ===============================================
 * Suggestions: Damerau-Levenshtein distance <= 1 
 * ===============================================
 *
 * We allow exactly one edit among:
 *  - substitution,
 *  - insertion (extra char in input),
 *  - deletion (missing char in input),
 *  - adjacent transposition (swap of i and i+1).
 *
 * Implementation strategy:
 *  - DFS over the trie, carrying:
 *      - input index (idx),
 *      - current dictionary prefix (in a char buffer),
 *      - edits_used (0 or 1),
 *  - When encountering a leaf, validate the remaining tails using
 *    a tight tail-matcher that accepts <=1 edit (considering current edits_used).
 */

static void add_suggestion(SuggestBox *box, const char *w) {
    for (int i = 0; i < box->count; ++i)
        if (strcmp(box->items[i], w) == 0) return; /* de-dup */

    if (box->count >= box->cap) return;

	size_t n = strlen(w);
    char *copy = (char*)malloc(n + 1);
    if (!copy) return;
    memcpy(copy, w, n + 1);
    box->items[box->count++] = copy;
}

/* Build full word from prefix + leaf suffix and add to box. */
static void emit_word_from_prefix_and_leaf(SuggestBox *box,
                                           const char *prefix,
                                           const char *leaf_suffix) {
    size_t lp = strlen(prefix), ls = strlen(leaf_suffix);
    char *w = (char*)malloc(lp + ls + 1);
    if (!w) return;
    memcpy(w, prefix, lp);
    memcpy(w + lp, leaf_suffix, ls);
    w[lp + ls] = '\0';
    add_suggestion(box, w);
    free(w);
}

/* Tail matcher for <=1 edit between tailA (dict leaf suffix) and tailB (input suffix). */
static int tailWithinOneEdit(const char *tailA, const char *tailB, int edits_used) {
    if (edits_used > 1) return 0;
    if (edits_used == 1) return strcmp(tailA, tailB) == 0;

    if (strcmp(tailA, tailB) == 0) return 1;

    int la = (int)strlen(tailA), lb = (int)strlen(tailB);

    /* substitution or transposition (equal lengths) */
    if (la == lb) {
        int diff = 0;
        for (int i = 0; i < la; ++i) {
            if (tailA[i] != tailB[i]) { diff++; if (diff > 1) break; }
        }
        if (diff == 1) return 1;

        /* adjacent transposition */
        for (int i = 0; i + 1 < la; ++i) {
            if (tailA[i] != tailB[i]) {
                if (tailA[i] == tailB[i + 1] && tailA[i + 1] == tailB[i]) {
                    if (strncmp(tailA + i + 2, tailB + i + 2, la - (i + 2)) == 0)
                        return 1;
                }
                break;
            }
        }
        return 0;
    }

    /* insertion (extra char in input): skip one in tailB */
    if (lb == la + 1) {
        int i = 0, j = 0, skipped = 0;
        while (i < la && j < lb) {
            if (tailA[i] == tailB[j]) { ++i; ++j; }
            else if (!skipped) { skipped = 1; ++j; }
            else return 0;
        }
        return 1;
    }

    /* deletion (missing char in input): skip one in tailA */
    if (la == lb + 1) {
        int i = 0, j = 0, skipped = 0;
        while (i < la && j < lb) {
            if (tailA[i] == tailB[j]) { ++i; ++j; }
            else if (!skipped) { skipped = 1; ++i; }
            else return 0;
        }
        return 1;
    }

    return 0;
}

/* DFS with at most one edit. See header for move semantics. */
static void dfsSuggest(NonLeafPtr p, const char *in, int idx,
                       char *prefix, int plen, int edits_used,
                       SuggestBox *box, int MAX_SUGG) {
    if (box->count >= MAX_SUGG) return;

    if (in[idx] == '\0' && p->EndOfWord == yes) {
        prefix[plen] = '\0';
        add_suggestion(box, prefix);
        if (box->count >= MAX_SUGG) return;
    }

    int n = (int)strlen(p->letters);

    /* Insertion (extra input char): consume in[idx] and stay on this node. */
    if (edits_used == 0 && in[idx] != '\0') {
        dfsSuggest(p, in, idx + 1, prefix, plen, 1, box, MAX_SUGG);
        if (box->count >= MAX_SUGG) return;
    }

    for (int i = 0; i < n && box->count < MAX_SUGG; ++i) {
        char edge = p->letters[i];
        NonLeafPtr child = p->ptrs[i];

        /* exact match */
        if (in[idx] != '\0' && in[idx] == edge) {
            prefix[plen] = edge; prefix[plen + 1] = '\0';
            if (child->kind == leaf) {
                LeafPtr lf = (LeafPtr)child;
                if (tailWithinOneEdit(lf->word, in + idx + 1, edits_used))
                    emit_word_from_prefix_and_leaf(box, prefix, lf->word);
            } else {
                dfsSuggest(child, in, idx + 1, prefix, plen + 1, edits_used, box, MAX_SUGG);
            }
            if (box->count >= MAX_SUGG) return;
        }

        /* substitution */
        if (edits_used == 0 && in[idx] != '\0' && in[idx] != edge) {
            prefix[plen] = edge; prefix[plen + 1] = '\0';
            if (child->kind == leaf) {
                LeafPtr lf = (LeafPtr)child;
                if (tailWithinOneEdit(lf->word, in + idx + 1, 1))
                    emit_word_from_prefix_and_leaf(box, prefix, lf->word);
            } else {
                dfsSuggest(child, in, idx + 1, prefix, plen + 1, 1, box, MAX_SUGG);
            }
            if (box->count >= MAX_SUGG) return;
        }

        /* deletion: go down without consuming input */
        if (edits_used == 0) {
            prefix[plen] = edge; prefix[plen + 1] = '\0';
            if (child->kind == leaf) {
                LeafPtr lf = (LeafPtr)child;
                if (tailWithinOneEdit(lf->word, in + idx, 1))
                    emit_word_from_prefix_and_leaf(box, prefix, lf->word);
            } else {
                dfsSuggest(child, in, idx, prefix, plen + 1, 1, box, MAX_SUGG);
            }
            if (box->count >= MAX_SUGG) return;
        }
    }

    /* adjacent transposition: consume in[idx+1] first, then in[idx] */
    if (edits_used == 0 && in[idx] != '\0' && in[idx + 1] != '\0') {
        int pos1 = Position(p, in[idx + 1]);
        if (pos1 != notFound && p->ptrs[pos1]) {
            NonLeafPtr c1 = p->ptrs[pos1];

            prefix[plen] = in[idx + 1]; prefix[plen + 1] = '\0';

            if (c1->kind == leaf) {
                LeafPtr lf1 = (LeafPtr)c1;
                if (lf1->word[0] == in[idx]) {
                    if (tailWithinOneEdit(lf1->word + 1, in + idx + 2, 1))
                        emit_word_from_prefix_and_leaf(box, prefix, lf1->word);
                }
            } else {
                int pos2 = Position(c1, in[idx]);
                if (pos2 != notFound && c1->ptrs[pos2]) {
                    NonLeafPtr c2 = c1->ptrs[pos2];

                    prefix[plen + 1] = in[idx]; prefix[plen + 2] = '\0';

                    if (c2->kind == leaf) {
                        LeafPtr lf2 = (LeafPtr)c2;
                        if (tailWithinOneEdit(lf2->word, in + idx + 2, 1))
                            emit_word_from_prefix_and_leaf(box, prefix, lf2->word);
                    } else {
                        dfsSuggest(c2, in, idx + 2, prefix, plen + 2, 1, box, MAX_SUGG);
                    }
                }
            }
        }
    }
}

void SuggestCorrections(NonLeafPtr root, const char *upper_word,
                        int maxSuggestions, SuggestBox *outBox) {
    outBox->count = 0;
    outBox->cap   = maxSuggestions;
    outBox->items = (char**)calloc((size_t)maxSuggestions, sizeof(char*));
    if (!outBox->items) { outBox->cap = 0; return; }

    char prefix[256];
    prefix[0] = '\0';

    dfsSuggest(root, upper_word, 0, prefix, 0, 0, outBox, maxSuggestions);
}

void FreeSuggestBox(SuggestBox *box) {
    if (!box || !box->items) return;
    for (int i = 0; i < box->count; ++i) free(box->items[i]);
    free(box->items);
    box->items = NULL;
    box->count = box->cap = 0;
}

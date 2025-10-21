#include "SimpleTrieSpellChecker.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Demo program:
 * - builds a trie from "dictionary" (one word per token),
 * - prints a side-view,
 * - scans "text" and reports misspelled words with suggestions.
 *
 * Files are read as ASCII; input tokens are uppercased. */

int main(void) {
    FILE *FIn = NULL, *words = NULL;
    char word[256], prefix[256] = "";
    NonLeafPtr root;
    int i, lineNum = 1, ch;

    /* Build dictionary */
    words = fopen("dictionary", "r");
    if (!words) Error("can't open `dictionary`");
    if (fscanf(words, "%255s", word) != 1) Error("empty `dictionary`");

    strupr_local(word);
    root = CreateNonLeaf(*word);           /* seed: root is non-leaf */
    CreateLeaf(*word, word + 1, root);     /* attach first word as leaf */

    while (fscanf(words, "%255s", word) == 1)
        TrieInsert(strupr_local(word), root);

    puts("SIDE VIEW");
    TrieSideView(0, root, prefix);

    /* Spell-check */
    FIn = fopen("text", "rb");
    if (!FIn) Error("can't open `text`");

    puts("Misspelled words (with suggestions):");
    ch = fgetc(FIn);
    while (1) {
        /* skip non-letters and track lines */
        while (ch != EOF && !isalpha((unsigned char)ch)) {
            if ((char)ch == '\n') lineNum++;
            ch = fgetc(FIn);
        }
        if (ch == EOF) break;

        /* read a word token */
        i = 0;
        while (ch != EOF && isalpha((unsigned char)ch)) {
            if (i < (int)sizeof(word) - 1) word[i++] = (char)ch;
            ch = fgetc(FIn);
        }
        word[i] = '\0';
        strupr_local(word);

        /* exact lookup; if missing, print suggestions */
        if (SearchTrie(root, word) != success) {
            printf("%s on line %d\n", word, lineNum);

            SuggestBox box;
            SuggestCorrections(root, word, 10, &box);
            if (box.count > 0) {
                printf("  Did you mean:");
                for (int k = 0; k < box.count; ++k) {
                    printf(" %s%s", box.items[k], (k + 1 < box.count ? "," : ""));
                }
                printf("\n");
            } else {
                printf("  (no close suggestions)\n");
            }
            FreeSuggestBox(&box);
        }
    }

    fclose(words);
    fclose(FIn);
    return 0;
}

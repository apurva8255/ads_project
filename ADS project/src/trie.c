/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  trie.c — Location Name Search (implementation)
 * ============================================================================
 */

#include "trie.h"
#include <ctype.h>

/* ══════════════════════════════════════════════════════════════════════════
 *   Internal helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── Character mapping (0..31) ────────────────────────────────────────── */

/* Maps A-Z, a-z, space, underscore to a 0-31 index */
static int char_to_index(char c) {
    if (isalpha((unsigned char)c)) {
        return tolower((unsigned char)c) - 'a';   /* 0..25 */
    } else if (c == ' ') {
        return 26;
    } else if (c == '_') {
        return 27;
    }
    return 31; /* fallback for unknown chars (dumping ground) */
}

/* ── Create a new Trie node ───────────────────────────────────────────── */

static TrieNode *create_node(void) {
    TrieNode *n = (TrieNode *)malloc(sizeof(TrieNode));
    if (!n) {
        fprintf(stderr, "[Trie] Error: Memory allocation failed.\n");
        return NULL;
    }
    for (int i = 0; i < TRIE_CHAR_RANGE; i++) {
        n->children[i] = NULL;
    }
    n->isEndOfWord    = 0;
    n->locationNodeID = -1;
    n->fullName[0]    = '\0';
    return n;
}

/* ── Recursive free ───────────────────────────────────────────────────── */

static void free_node(TrieNode *node) {
    if (!node) return;
    for (int i = 0; i < TRIE_CHAR_RANGE; i++) {
        free_node(node->children[i]);
    }
    free(node);
}

/* ── DFS helper for prefix search (autocomplete) ──────────────────────── */

static void collect_matches(TrieNode *node, TrieMatch results[],
                            int maxResults, int *count) {
    if (!node || *count >= maxResults) return;

    if (node->isEndOfWord) {
        results[*count].nodeID = node->locationNodeID;
        strncpy(results[*count].name, node->fullName, MAX_NAME_LEN - 1);
        results[*count].name[MAX_NAME_LEN - 1] = '\0';
        (*count)++;
    }

    for (int i = 0; i < TRIE_CHAR_RANGE; i++) {
        if (node->children[i]) {
            collect_matches(node->children[i], results, maxResults, count);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *   Public API
 * ══════════════════════════════════════════════════════════════════════════ */

void trie_init(Trie *t) {
    t->root      = create_node();
    t->wordCount = 0;
}

int trie_insert(Trie *t, const char *name, int locationNodeID) {
    if (!t || !t->root || !name) return -1;

    TrieNode *cur = t->root;
    for (int i = 0; name[i] != '\0'; i++) {
        int idx = char_to_index(name[i]);
        if (!cur->children[idx]) {
            cur->children[idx] = create_node();
            if (!cur->children[idx]) return -1; /* memory error */
        }
        cur = cur->children[idx];
    }

    /* Mark end of word and store payload */
    if (!cur->isEndOfWord) {
        cur->isEndOfWord = 1;
        t->wordCount++;
    }
    cur->locationNodeID = locationNodeID;
    strncpy(cur->fullName, name, MAX_NAME_LEN - 1);
    cur->fullName[MAX_NAME_LEN - 1] = '\0';

    return 0;
}

int trie_search_exact(const Trie *t, const char *name) {
    if (!t || !t->root || !name) return -1;

    TrieNode *cur = t->root;
    for (int i = 0; name[i] != '\0'; i++) {
        int idx = char_to_index(name[i]);
        if (!cur->children[idx]) return -1;
        cur = cur->children[idx];
    }

    if (cur && cur->isEndOfWord)
        return cur->locationNodeID;
    return -1;
}

int trie_search_prefix(const Trie *t, const char *prefix,
                       TrieMatch results[], int maxResults) {
    int count = 0;
    if (!t || !t->root || !prefix || !results || maxResults <= 0) return 0;

    TrieNode *cur = t->root;
    for (int i = 0; prefix[i] != '\0'; i++) {
        int idx = char_to_index(prefix[i]);
        if (!cur->children[idx]) return 0;  /* prefix not found at all */
        cur = cur->children[idx];
    }

    /* Found the prefix node, now collect all words below it */
    collect_matches(cur, results, maxResults, &count);
    return count;
}

int trie_load_from_graph(Trie *t, const Graph *g) {
    if (!t || !g) return -1;

    int loaded = 0;
    for (int i = 0; i < g->nodeCount; i++) {
        if (g->nodes[i].name[0] != '\0') {
            if (trie_insert(t, g->nodes[i].name, g->nodes[i].id) == 0) {
                loaded++;
            }
        }
    }
    printf("[Trie] Loaded %d locations from Graph.\n", loaded);
    return loaded;
}

void trie_free(Trie *t) {
    if (t) {
        free_node(t->root);
        t->root      = NULL;
        t->wordCount = 0;
    }
}

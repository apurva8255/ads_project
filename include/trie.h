/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  trie.h — Trie for Location Name Search (Prefix matching / Autocomplete)
 * ============================================================================
 */

#ifndef TRIE_H
#define TRIE_H

#include "common.h"
#include "graph.h"

/* ── Trie Node ────────────────────────────────────────────────────────── */

/* A-Z, a-z, space, and underscore (mapped into 0..31 array) */
#define TRIE_CHAR_RANGE 32

typedef struct TrieNode {
    struct TrieNode *children[TRIE_CHAR_RANGE];
    int              isEndOfWord;
    int              locationNodeID;   /* links to city graph node        */
    char             fullName[MAX_NAME_LEN]; /* store original format     */
} TrieNode;

/* ── Trie Root ────────────────────────────────────────────────────────── */

typedef struct {
    TrieNode *root;
    int       wordCount;
} Trie;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Initialise an empty Trie */
void trie_init(Trie *t);

/* Insert a location name mapping locally to its graph node ID */
int trie_insert(Trie *t, const char *name, int locationNodeID);

/* Search for an exact location name. Returns node ID, or -1 if not found. */
int trie_search_exact(const Trie *t, const char *name);

/*
 * Prefix search (Autocomplete).
 * Fills `results` with up to `maxResults` matching location names and
 * their node IDs. Returns the number of matches found.
 */
typedef struct {
    int  nodeID;
    char name[MAX_NAME_LEN];
} TrieMatch;

int trie_search_prefix(const Trie *t, const char *prefix,
                       TrieMatch results[], int maxResults);

/* Load all location names directly from a populated Graph */
int trie_load_from_graph(Trie *t, const Graph *g);

/* Free all nodes in the Trie */
void trie_free(Trie *t);

#endif /* TRIE_H */

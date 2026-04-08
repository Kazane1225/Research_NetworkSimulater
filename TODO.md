# TODO

## High priority

### 1. Node hash for `MergeHistoryTrees` (hashing)
**File:** `src/history_tree.h`, `src/history_tree.c`

Add a `hash` field to `HistoryTree`. Compute it bottom-up after each merge so that every node carries a hash of its entire subtree structure (input, outdegree, children hashes, red-edge hashes sorted by target hash + multiplicity).

Use the hash in the child-lookup inner loop of `MergeHistoryTrees` and `HistoryTreeContains`: instead of calling `EquivalentNodes` (O(obs)), compare hash values (O(1)). Build a small hash-keyed lookup table over `b->children` so the whole search drops from O(C × obs) to O(C) or O(1).

Expected gain: up to **~obs×** faster per merge call, where obs can be as large as n−1.

---

### 2. Incremental history-tree update on round add/delete
**File:** `src/network.c`, `src/entity.c`

Currently `ExecuteNetwork()` rebuilds every entity's history tree from scratch whenever any round changes. Instead:
- On **round append**: replay only the new round on top of the existing trees.
- On **round delete** (when it is the last round): roll back one level by keeping a snapshot of the tree state before that round was applied.

This avoids O(rounds × n²) work on every user interaction.

---

## Low priority

### 3. Arena allocator for `HistoryTree` nodes
**File:** `src/history_tree.c`

Replace per-node `malloc`/`free` with a slab/arena allocator scoped to the lifetime of a network execution. Reduces allocator overhead when round counts are large.

### 4. `HistoryTreeEquals` short-circuit via hash
Once node hashes are available (item 1), `HistoryTreeEquals` can compare root hashes first and return immediately when they differ, avoiding two full BFS traversals.

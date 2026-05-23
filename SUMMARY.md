# Summary

## Benchmark Snapshot

### End-of-sequence round append

Measured by reducing the tutorial network to 1 round, then appending from `1 -> 250` while waiting for the round count to increase and one frame to render after each append.

| Metric | current | `main` |
|---|---:|---:|
| Total time | 1.296 s | 16.162 s |
| Average | 4.9 ms/click | 64.6 ms/click |
| First 50 clicks | 4.2 ms/click | 4.9 ms/click |
| After 200 clicks | 6.5 ms/click | 155.9 ms/click |
| Max | 10.9 ms | 195.6 ms |
| p95 | 7.5 ms | 173.4 ms |

### Middle insert near round 1

Measured by starting near round 1 in the tutorial network and repeatedly inserting rounds until the total reached 250.

| Metric | current | `main` |
|---|---:|---:|
| Total time | 2.532 s | 16.926 s |
| Average | 10.34 ms/click | 69.09 ms/click |
| p95 | 21.60 ms | 181.95 ms |
| Max | 23.67 ms | 203.17 ms |

### Current limit check (`> 1 s / click`)

Measured on the current branch only. The limit is defined as the first round where a single append/insert takes more than `1000 ms` (JS-side wall-clock including one rAF frame ≈ 16 ms overhead).

| Scenario | Limit definition | Result |
|---|---|---|
| End-of-sequence append (`AppendLastRound`) | Single append `> 1000 ms` | Not reached by 5000 rounds (~224 ms at round 5000) |
| Middle insert from round 1 (constant ~5-round suffix, bottleneck is `RebuildFinalHistory` + `ComputeAuxData`) | Single insert `> 1000 ms` | **~3252 rounds** (1003 ms, JS-side timing; was ~2980 before Merkle hash) |
| Middle insert at round 100, 1300 total rounds (suffix 1199 rounds) | Crash test | No crash; ~12.5 s per insert (suffix of 1199 rounds fully re-executed) |

## Key Points

- The current branch is about 12.5x faster than `main` for repeated end-of-sequence round appends.
- In the 200+ round region, current stays responsive while `main` slows down sharply.
- Middle insert and middle delete no longer replay the whole network from round 0; they now reuse checkpoints and replay only the affected suffix.
- The dominant last-round append bottleneck was removed by extending `finalHistory` incrementally instead of rebuilding it from scratch.

## Main Optimization Steps

### 1. Incremental last-round execution

- Added snapshot-based fast paths for last-round re-execution, rollback, and append.
- Result: common last-round interactions dropped from `O(R x n^2)` to `O(n^2)`.

### 2. Incremental `finalHistory` extension

- Replaced full `RebuildFinalHistory()` during append with a one-level incremental extension.
- Result: the append-specific history step dropped from `O(R x n^3)` to `O(n^2)`.

### 3. Replay from round checkpoints

- Added periodic checkpoints and `ExecuteNetworkFromRound(firstRound)`.
- Result: middle insert and delete now replay only the affected suffix instead of rebuilding from round 0.

### 4. Auxiliary-data and tree-lookup reductions

- Added hash-based lookup in `ComputeAuxDataRedEdges`.
- Reduced unnecessary work in history-tree equality and child lookup.
- Sorted observations so red-edge lookup uses binary search.

### 5. Hash map-based child lookup in `MergeHistoryTrees`

`MergeHistoryTrees` walks h2 in BFS order and maps each h2 node into h1. The core inner step is: given a node `b` in h1 and a child `x` of the corresponding h2 node, decide whether `b` already has a child equivalent to `x` (and map `x` there) or whether a new child must be inserted.

With the old linear scan, this took O(k) per candidate child, where k is the number of existing children of `b`. In large history trees, k can be large and the total cost over all BFS levels is quadratic in the tree width.

The new approach builds a small open-addressing hash map over `b`'s children before processing any of `x`'s siblings:

1. **Key function.** Each child is keyed by `(input, outdegree, observations_count)` — the three fields that `EquivalentNodes` checks first and that are cheapest to compute. A key mismatch is a guaranteed structural mismatch, so the full `EquivalentNodes` call is skipped entirely.
2. **Capacity.** The map is sized at `(existing + incoming) × 2 + 3` so the load factor stays below 50% even after all of `a`'s children are inserted, avoiding rehashing.
3. **Lookup.** `cmap_get` hashes `x`'s key, probes linearly until an empty slot or a key match, and only calls `EquivalentNodes` on key-matching entries. Collisions (different structure, same key) fall through to the full structural check as before.
4. **Insertion.** When a new child `y` is created and added to `b`, it is also inserted into the map immediately so that later siblings of `x` (which may be identical to `y`) benefit from the O(1) lookup too.

This makes the inner loop O(1) expected per child instead of O(k), turning the per-BFS-level cost from O(k²) to O(k).

## History-Tree Hash

Two separate hash mechanisms exist in `history_tree.c`:

### 1. Merkle structural hash (per-node `h->hash`) — **active**

- Each `HistoryTree` node has a 64-bit `hash` field, initialized to 0 in `NewHistoryTree`.
- `ComputeNodeHash` computes the hash from `input`, `outdegree`, child count, and the sorted array of child hashes. `ComputeHashBottomUp` runs this post-order over an entire subtree.
- **`ComputeHashBottomUp` is now called in `RebuildFinalHistory`** at the start of every merge pass (once per entity tree, before the merge loop).
- **Entity-deduplication optimization**: after hashing all entity trees, the merge loop checks whether any prior entity shares the same root hash (same hash ↔ isomorphic subtree). If so, the second entity's `MergeHistoryTrees` call is skipped entirely and it reuses the first entity's `finalLeaf`. This is correct because isomorphic entity trees map to the same finalHistory node.
- **The `EquivalentNodes` short-circuit** (`if(h1->hash && h2->hash && h1->hash!=h2->hash) return false`) still never fires: finalHistory nodes are always created fresh with `hash=0`, so both sides are 0 when merging an entity tree into finalHistory. The deduplication works at the entity level (before any merge call), not inside `EquivalentNodes`.
- **Effectiveness**: benefit depends on network symmetry. For the tutorial network (all 6 entities uniquely identifiable, all trees distinct), no merges are skipped — overhead is one `ComputeHashBottomUp` call per entity and an O(n²) uniqueness check (negligible for typical n ≤ 50). For symmetric/regular networks where multiple entities share identical history trees, the optimization skips redundant merge calls entirely.

### 2. Child-lookup key (`local_key`, used inside `MergeHistoryTrees`) — **active**

- An open-addressing hash map is built over `b->children` before processing each BFS node's children in `MergeHistoryTrees`.
- **Key fields**: `(input + 0x8000, outdegree + 2, observations->tot)` — offset biases keep values away from 0, which is the empty-slot sentinel. Result is normalised to non-zero: `return k ? k : 1ULL`.
- The same `hash_mix` function is used: `(h ^ v) * 0x9e3779b97f4a7c15 ^ (result >> 30)`.
- This key covers exactly the three fields that `EquivalentNodes` checks first. A key mismatch guarantees `EquivalentNodes` returns false; on a key match, `EquivalentNodes` is called for the full structural check.
- Capacity: `(existing + incoming) * 2 + 3` keeps load factor below 50%.

### Why not build the hash from the parent's value and incoming red-edge predecessor hashes?

(Note: this argument applies to the `h->hash` Merkle field, which is currently dead code. It explains the design rationale if the feature were activated.)

A node at level L receives red edges from deeper nodes (level L+1) that point back toward it. One could imagine hashing N as `hash(parent_value, input, multiset_of_hashes_of_nodes_with_red_edges_to_N)`. Even if `parent_value` is used instead of `parent_hash`, this still mixes context into the node hash — the core problem is the same regardless of which form the parent's information takes. There are two structural reasons this does not work:

1. **The hash needs to represent the subtree, not the context.** History-tree isomorphism is defined subtree-wise: two nodes are isomorphic iff their subtrees (everything below and including them) are structurally identical. The parent's hash and any incoming red edges are *outside* the subtree rooted at N — they describe the surrounding context, not the subtree itself. Including them would make two structurally identical subtrees hash differently whenever they appear in different positions in the tree, breaking the isomorphism check.

2. **Incoming red edges are not stable at hash-computation time.** Red edges from level L+1 toward level L are created during the same round that level-L+1 nodes are built, which happens *after* level-L nodes already exist. In the current bottom-up pass, level-L nodes must be hashed before their deeper descendants exist, so the set of incoming red edges to N is not yet known. Computing the hash top-down (parent first) would fix the ordering, but would then require knowing parent hashes before children — the opposite of Merkle — and would still not capture subtree structure.

The current design (children only, sorted multiset) is the standard Merkle construction: the hash of N depends only on N's own fields and the hashes of its children's subtrees, computed in a single bottom-up DFS pass. This makes the hash purely structural, context-independent, and incrementally recomputable.

## Crash Status

**Fixed.**

- Middle insert no longer crashes.
- Root cause confirmed: `ComputeAuxDataWidth` was recursive with depth equal to the round count. At ~1993 rounds this overflowed the default WebAssembly shadow-stack (64 KB), causing a silent stack overflow → crash.
- Fix: rewrote `ComputeAuxDataWidth` as a two-phase iterative DFS (Phase 1: pre-order via explicit stack to allocate and attach `AuxData`; Phase 2: bottom-up reverse iteration to compute widths). No recursion remains in the aux-data path.
- Verified: middle insert at round 100 in a 1301-round simulation (suffix 1199 rounds) completed with no crash. Previously any deep middle insert crashed before ~1993 rounds. Note: the insert itself took ~12.5 s because `ExecuteNetworkFromRound` re-executes the full 1199-round suffix; this is expected compute cost, not a regression.

### Performance limit after fix

With the crash fixed, the practical limit shifts to raw compute time:

| Scenario | Limit definition | Result |
|---|---|---|
| End-of-sequence append (`AppendLastRound`) | Single append `> 1000 ms` | Not reached by 5000 rounds (~224 ms at round 5000) |
| Middle insert from round 1 (~5-round suffix, bottleneck is `RebuildFinalHistory` + `ComputeAuxData`) | Single insert `> 1000 ms` | **~3252 rounds** (Merkle hash: +272 rounds vs ~2980 before) |
| Middle insert at round 100, 1300 total rounds (suffix 1199 rounds) | Crash test + timing | No crash; ~12.5 s for one insert (1199-round suffix fully re-executed) |

### Remaining hypotheses (no longer blocking, kept for reference)

The earlier crash analysis identified two secondary candidates that have not been ruled out at even higher round counts:

- **Memory exhaustion**: `TakeSnapshotsBeforeRound()` copies all entity history trees; at very large round counts the combined peak (live trees + snapshot copies + `finalHistory` + aux data) could approach the 256 MB WASM linear-memory limit. A `malloc` failure that is not checked would cause a null-pointer dereference.
- **Dangling checkpoint pointer**: `ExecuteNetworkFromRound` restores a raw `current` pointer stored in the checkpoint. If `TrimHistoryTreeToRound` frees a node whose `bornRound == prefixRound` (boundary condition), the restored pointer is stale.

## Measurement Notes

- Benchmark values depend on the machine, browser, viewport, and foreground/background tab state, so absolute times should be read as environment-specific.
- The most reliable reading is the relative comparison between `current` and `main` under the same machine and browser conditions.
- Environment: Windows, VS Code integrated browser.
- Branches: `feature/improve_algorithm` vs `main` in a separate worktree.
- Test data: `Module._TutorialLoadNetwork()`.
- End-of-sequence append was measured in the same foreground tab for both builds to avoid background-tab throttling.

For implementation details and chronological notes, see `DONE.md`.
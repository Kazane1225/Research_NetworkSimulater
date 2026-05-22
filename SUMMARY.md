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

Measured on the current branch only. The limit is defined as the first round where a single append takes more than `1000 ms`.

| Scenario | Branch | Limit definition | Result |
|---|---|---|---|
| End-of-sequence append | current | Single append `> 1000 ms` | Not reached by 5000 rounds |
| Middle insert near round 1 | current | Single insert `> 1000 ms` | Not reached before crash at 1993 rounds |

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

- Each `HistoryTree` node stores a Merkle-style structural hash.
- The hash is built from `input`, `outdegree`, and the sorted multiset of child hashes.
- Red edges are intentionally excluded because they point upward (from level L toward level L−1) and would create a computation-order dependency: in a bottom-up (post-order) traversal, level-L nodes are processed before their shallower level-(L−1) ancestors, so the ancestor hashes are not yet available when level-L hashes are being computed.
- The hash is used as a fast inequality check before the full tree-isomorphism logic runs; it is a pruning step, not the final correctness check by itself.

### Why not build the hash from the parent's value and incoming red-edge predecessor hashes?

A node at level L receives red edges from deeper nodes (level L+1) that point back toward it. One could imagine hashing N as `hash(parent_value, input, multiset_of_hashes_of_nodes_with_red_edges_to_N)`. Even if `parent_value` is used instead of `parent_hash`, this still mixes context into the node hash — the core problem is the same regardless of which form the parent's information takes. There are two structural reasons this does not work:

1. **The hash needs to represent the subtree, not the context.** History-tree isomorphism is defined subtree-wise: two nodes are isomorphic iff their subtrees (everything below and including them) are structurally identical. The parent's hash and any incoming red edges are *outside* the subtree rooted at N — they describe the surrounding context, not the subtree itself. Including them would make two structurally identical subtrees hash differently whenever they appear in different positions in the tree, breaking the isomorphism check.

2. **Incoming red edges are not stable at hash-computation time.** Red edges from level L+1 toward level L are created during the same round that level-L+1 nodes are built, which happens *after* level-L nodes already exist. In the current bottom-up pass, level-L nodes must be hashed before their deeper descendants exist, so the set of incoming red edges to N is not yet known. Computing the hash top-down (parent first) would fix the ordering, but would then require knowing parent hashes before children — the opposite of Merkle — and would still not capture subtree structure.

The current design (children only, sorted multiset) is the standard Merkle construction: the hash of N depends only on N's own fields and the hashes of its children's subtrees, computed in a single bottom-up DFS pass. This makes the hash purely structural, context-independent, and incrementally recomputable.

## Crash Status

- Middle insert no longer fails around the old `~1000`-round region after the checkpoint redesign, but it still crashes later, currently before `1993` rounds.
- The current direction is to keep improving the replay/restore path rather than weakening the feature for large round counts.
- The reason is that the remaining instability is in the deep middle-insert checkpoint/replay path, while end-of-sequence append is still stable and fast.

### Why it likely crashes (hypothesis — needs further investigation)

The checkpoint store keeps at most `ROUND_CHECKPOINT_MAX_STORED = 32` entries at a spacing of `ROUND_CHECKPOINT_INTERVAL = 8` rounds. Round 1 is specially protected from eviction. At ~1993 total rounds, the 31 non-protected slots cover roughly the most recent 248 rounds (rounds 1744–1992). For a middle insert near round 1, the nearest available checkpoint is round 1 itself; all others are too far ahead. `ExecuteNetworkFromRound` therefore restores to round 1 and replays all ~1992 remaining rounds.

During that deep replay, two operations have large peak memory cost:

- `TakeSnapshotsBeforeRound()` (called at the final replay round) copies every entity's full history tree. At ~1993 rounds, each tree can have on the order of thousands of nodes, so copying all entities' trees roughly doubles the live node count at that moment.
- `RebuildFinalHistory()` immediately after merges all entity trees into `finalHistory` and runs `ComputeAuxData`, which allocates auxiliary structures over the merged tree.

The combined peak — live entity trees + snapshot copies + finalHistory + auxiliary data — can approach or exceed the WebAssembly linear-memory limit (typically 256 MB). A failed `malloc` that is not checked would then produce a null-pointer dereference (crash) somewhere inside the tree manipulation code.

A secondary candidate is a dangling pointer in the restored checkpoint: the checkpoint stores a raw `current` pointer into the history tree. If `TrimHistoryTreeToRound` has an edge case that frees a node whose `bornRound` equals `prefixRound` (the boundary condition), the restored pointer is stale. This is harder to trigger and would produce a more erratic crash pattern, but it cannot be ruled out without a sanitizer run.

Next steps: run with `AddressSanitizer` or check `malloc` return values to distinguish memory exhaustion from a use-after-free.

## Measurement Notes

- Benchmark values depend on the machine, browser, viewport, and foreground/background tab state, so absolute times should be read as environment-specific.
- The most reliable reading is the relative comparison between `current` and `main` under the same machine and browser conditions.
- Environment: Windows, VS Code integrated browser.
- Branches: `feature/improve_algorithm` vs `main` in a separate worktree.
- Test data: `Module._TutorialLoadNetwork()`.
- End-of-sequence append was measured in the same foreground tab for both builds to avoid background-tab throttling.

For implementation details and chronological notes, see `DONE.md`.
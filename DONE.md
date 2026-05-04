# Optimization Log

## 2026-05-04 — `AppendLastRound` O(n²) incremental finalHistory extension

**File:** `src/network.c`, `src/entity.c`, `src/history_tree.h`, `src/history_tree.c`

**Problem:**  
`AppendLastRound` called `RebuildFinalHistory()`, which merged every entity's full history tree (depth R) into `finalHistory` from scratch — O(R × n³) per append. As R grew, each additional round took longer than the last.

**Fix:**  
Added `HistoryTree *finalLeafAtSend` field to `Observation`. `SendHistory` stores `e1->finalLeaf` (the sender's current position in `finalHistory`) into each mailbox observation at send time. Before `EndRound` destroys the mailbox, `AppendLastRound` collects `(senderFinalLeaf, multiplicity)` pairs per entity. After `EndRound`, `ExtendFinalHistoryOneLevel` adds exactly one new level to `finalHistory` under each entity's `prevFinalLeaf`:

1. Sort + merge collected pairs by `senderFinalLeaf` pointer (same sender twice → combine multiplicity).
2. For each entity, search existing children of `prevFinalLeaf[i]` for an equivalent node using zip-comparison on sorted `observations` lists — O(children × obs).
3. If not found, create a new child and add red edges pointing to `senderFinalLeaf` targets (already nodes in `finalHistory`; no pointer chasing required).
4. Set `e->finalLeaf` to the found/created node.

**Gain:**  
`AppendLastRound` cost drops from O(R × n³) to O(n²) for the history-extension step. The residual cost is `ComputeAuxData` O(R × n × obs) + `EndRound`'s merge O(n × R × n²). For interactive use (each press of `=` appends one round), the history-extension bottleneck is eliminated.

---

## 2026-05-04 — `ComputeAuxDataRedEdges` hash-map lookup

**File:** `src/auxdata.c`

**Problem:**  
`ComputeAuxDataRedEdges` searched for each red-edge target by linear scan over all level-i−1 nodes — O(n) per red edge per node, O(R × n² × obs) overall. This grew with both R and n.

**Fix:**  
Before processing each level i, build a small open-addressing hash map (`HistoryTree * → AuxData index`) for level i−1. Each red-edge lookup becomes O(1) expected instead of O(n). The map is allocated and freed per level.

**Gain:**  
`ComputeAuxDataRedEdges` drops from O(R × n² × obs) to O(R × n × obs). For n=20, this is a 20× reduction in that step.

---

## 2026-05-04 — Merkle hash + `HistoryTreeEquals` short-circuit

**File:** `src/history_tree.h`, `src/history_tree.c`, `src/network.c`

**Problem:**  
`HistoryTreeEquals` always ran two full BFS traversals (`HistoryTreeContains` × 2), even when the trees were obviously different.

**Fix:**  
Added an `unsigned long long hash` field to `HistoryTree`. The hash is a pure Merkle fingerprint: `input`, `outdegree`, and the sorted multiset of children's hashes (red edges intentionally excluded — including them would create a circular dependency because red edges point upward from level R to level R−1, which are not yet processed when computing level-R hashes in a post-order DFS).

- `ComputeNodeHash` computes the hash for a single node (children already hashed).
- `ComputeHashBottomUp` does a post-order DFS to recompute all hashes in a subtree. Called in `RebuildFinalHistory` after each entity's history is finalized.
- `HistoryTreeEquals` has a one-line guard at the top: if both root hashes are non-zero and differ, return `false` immediately — O(1) for the common case of unequal trees. Equal or uncertain cases fall through to the full `HistoryTreeContains` × 2 BFS as before.

**Note:** An earlier attempt tried using the hash field to replace `EquivalentNodes` with an O(1) open-addressing hash map inside `MergeHistoryTrees` and `HistoryTreeContains`. This was reverted: red-edge hashes could not be included in the Merkle hash without a circular dependency, so the hash alone could not distinguish structurally different nodes at the same level. `EquivalentNodes` (which checks observations directly) remains the authoritative equivalence check in those two functions. A correct hash-map child lookup was later implemented using `local_key` (see next entry).

---

## 2026-05-04 — Incremental history-tree update on round add/delete

**File:** `src/network.c`, `src/network.h`, `src/entity.h`, `src/entity.c`

**Problem:**  
`ExecuteNetwork()` rebuilt all entity history trees from scratch on every user interaction — O(R × n²) where R is the round count.

**Fix:**  
Added per-entity `EntitySnapshot *snap` (in `entity.h`) that stores a copy of `history`, `current`, and `outdegree` before the last round is applied.  
`ExecuteNetwork()` takes a snapshot (`TakeSnapshotsBeforeRound()`) just before executing the last round.  
Three new functions handle the three common cases without a full rebuild:

- `ReExecuteLastRound()` — restores from snapshot (copy), replays last round, rebuilds `finalHistory`. Snapshot stays valid. Used when a link is added/removed in the last round, or all links in the last round are cleared.
- `RollBackLastRound()` — restores from snapshot (ownership transfer), rebuilds `finalHistory`. Snapshot is consumed/invalidated. Used when the last round is deleted.
- `AppendLastRound()` — takes a fresh snapshot of the current entity states, applies the newly appended last round, rebuilds `finalHistory`. Used when a new round is inserted at the end.

Call-site changes in `events.c`:
- BACKSPACE (clear round links): `ReExecuteLastRound()` if in last round
- MINUS (delete round): `RollBackLastRound()` if last round was deleted
- EQUALS (insert round): `AppendLastRound()` if appended at end
- Mouse link add/remove: `ReExecuteLastRound()` if in last round (single-round modification only; all-rounds modifier still calls `ExecuteNetwork()`)

All four paths fall back to `ExecuteNetwork()` if snapshots are unavailable (e.g., after a structural change).

**Gain:**  
Interactions with the last round drop from O(R × n²) to O(n²). For R rounds the savings grow linearly with R.

---



## 2026-05-04 — `MergeHistoryTrees` / `HistoryTreeContains` child-lookup hash map

**File:** `src/history_tree.c`

**Problem:**  
The child-lookup inner loop in `MergeHistoryTrees` and `HistoryTreeContains` called `EquivalentNodes` for every candidate child of node `b` — O(C) calls per BFS node. `EquivalentNodes` starts with O(1) field checks (input, outdegree, level, obs_count), but when those match it scans all observations via `FindRedEdge` — O(obs). Total per BFS node: O(C × obs).

**Fix:**  
Added a small open-addressing hash map over `b->children`, indexed by `local_key = hash(input, outdegree, observations_count)`. These are exactly the fields `EquivalentNodes` checks in O(1) at the start; a key mismatch guarantees `EquivalentNodes` would return false, so the call is skipped entirely. Only candidates with a matching key proceed to `EquivalentNodes`.

- `ChildEntry` struct (key + node pointer), `local_key`, `cmap_put`, `cmap_get` added as file-local helpers.
- Map capacity pre-allocated at `(existing + incoming) × 2 + 3` to keep load ≤ 50% even after new children are inserted during the merge.
- When a new child `y` is created in `MergeHistoryTrees`, it is inserted into the map immediately so later children of the same `a` can find it.

**Note:** The key is `local_key` (input + outdegree + obs_count), NOT the Merkle hash. Using the Merkle hash (subtree fingerprint) as a pre-filter for `EquivalentNodes` (local-node check) is incorrect: `EquivalentNodes` can return true for two nodes whose subtrees differ (the BFS handles children level-by-level; the local check does not require subtree equality). `local_key` is a valid necessary condition for `EquivalentNodes`.

**Gain:**  
In the typical case where children have distinct (input, outdegree, obs_count), zero `EquivalentNodes` calls are needed per non-matching child. Child lookup drops from O(C × obs) to O(C) map-build + O(obs) for the one matching candidate. For C ≈ n−1, this gives up to **~n×** speedup per BFS node.

---

## 2026-05-04 — `observations` sorted + `FindRedEdge` binary search

**File:** `src/history_tree.c`

**Problem:**  
`FindRedEdge(h, target)` did a linear scan over `h->observations` — O(obs). It is called once per observation in `EquivalentNodes`, making `EquivalentNodes` O(obs²) in the worst case.

**Fix:**  
Keep `observations` sorted by `o->history` pointer value at all times.
- `FindRedEdge` now uses binary search: O(log obs).
- `AddNewRedEdge` uses binary search to find the insertion position and calls `InsertVector` to shift: O(obs) worst case for the shift, but obs is bounded by n−1 and insertions are rare compared to lookups.
- `AddRedEdge` is unchanged (still calls `FindRedEdge` then `AddNewRedEdge`).

`EquivalentNodes` drops from O(obs²) to O(obs log obs).

**Gain:**  
For the research networks in this project obs ≤ n−1 which is typically small (≤20). The asymptotic improvement becomes visible at larger obs, but correctness and code clarity are improved regardless.

---

## 2026-04-08 — `HistoryTreeContains`: eliminate unnecessary tree copy

**File:** `src/history_tree.c`

**Problem:**  
`HistoryTreeContains(h1, h2)` was implemented by copying the entire `h1` tree (O(n) time and memory), merging `h2` into the copy, and then discarding it. Since `HistoryTreeEquals` calls `HistoryTreeContains` twice, every equality check required two full copies.

**Fix:**  
Replaced with a direct BFS isomorphism check that reads `h1` without modifying it. The same `reference` field convention used by `MergeHistoryTrees` is used to track the mapping during the traversal, and `ResetReferences` cleans up afterward.  
This turns two O(n) heap allocations + copies into zero allocations per call.

**Notes on other ideas considered:**
- `observations` hash map (`FindRedEdge` O(obs→1)): confined to `history_tree.c`, but obs is bounded by n (typically ≤20 for research networks), so impact is negligible.
- Child-lookup hash in `MergeHistoryTrees` inner loop: would reduce O(C × obs²) to O(obs), but requires more invasive changes. Deferred.
- Merkle hash on subtrees: would make `HistoryTreeEquals` O(1) but high implementation cost.
- Arena allocator for `malloc`-per-node: low implementation cost, high gain for large round counts.

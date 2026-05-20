# Optimization Log

## 2026-05-20 — Round-Append Benchmark (`current` vs `main`)

**Goal:**  
Measure how interactive round-appends behave at large round counts, and verify whether the reported “UI starts stuttering around 200+ rounds” effect is visible in actual timing data. Compare the current branch against `main` under the same conditions.

**Environment:**
- OS: Windows
- Runtime: VS Code integrated browser
- User-Agent: `Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Code/1.120.0 Chrome/142.0.7444.265 Electron/39.8.8 Safari/537.36`
- Logical cores: 10
- Viewport: `1812 x 942`

**Branches measured:**
- current: `feature/improve_algorithm`
- baseline: `main` (measured in a separate worktree at `C:\myproject\Research_NetworkSimulater_main`)

**Test setup:**
- Use `Module._TutorialLoadNetwork()`
- Agent count: 6
- Initial round count after tutorial load: 5
- Before each benchmark, move to the last round and delete rounds until only 1 round remains
- Then append rounds from `1 -> 250`

**How the benchmark was driven:**
- DOM buttons `btn-add-round`, `btn-del-round`, `btn-next-round`, `btn-prev-round` were triggered from Playwright
- For each `+` click, wait until `Module._GetNumRounds()` increments, then wait one `requestAnimationFrame`
- Aggregate timings per 25-round segment
- To avoid background-tab throttling, both current and `main` were measured sequentially in the **same foreground browser tab**, switching between `http://localhost:8000/` and `http://localhost:8001/`

**Meaning of “done” for each append:**
- This benchmark targets rapid append responsiveness, not full visual settling time
- Each append is considered complete when:
	1. the round count has increased, and
	2. one subsequent frame has been rendered

**Summary results:**

| Metric | current | `main` |
|---|---:|---:|
| Total time, 1 -> 250 | 1.296 s | 16.162 s |
| Average over all clicks | 4.9 ms/click | 64.6 ms/click |
| Average over first 50 clicks | 4.2 ms/click | 4.9 ms/click |
| Average over clicks after 200 | 6.5 ms/click | 155.9 ms/click |
| Max click time | 10.9 ms | 195.6 ms |
| 95th percentile | 7.5 ms | 173.4 ms |

**Time per 25-round segment:**

| Segment | current | `main` |
|---|---:|---:|
| 1 -> 25 | 100.3 ms | 100.3 ms |
| 25 -> 50 | 104.4 ms | 134.6 ms |
| 50 -> 75 | 104.3 ms | 326.9 ms |
| 75 -> 100 | 104.4 ms | 603.3 ms |
| 100 -> 125 | 104.4 ms | 974.1 ms |
| 125 -> 150 | 109.0 ms | 1445.8 ms |
| 150 -> 175 | 121.2 ms | 2027.9 ms |
| 175 -> 200 | 141.2 ms | 2677.3 ms |
| 200 -> 225 | 162.0 ms | 3473.9 ms |
| 225 -> 250 | 164.2 ms | 4319.3 ms |

**Observations:**
- The current branch still gets somewhat slower after 200 rounds, but the increase is mild: segment time rises from roughly 100–120 ms up to about 141–164 ms.
- In `main`, slowdown starts much earlier and grows aggressively. Beyond 200 rounds, each 25-round block takes 2.7–4.3 seconds.
- The reported “UI starts catching/stuttering around 200+ rounds” is consistent with `main` timings. The current branch reduces that effect substantially.

**Conclusion:**
- For repeated end-of-sequence round appends, the current branch is about **12.5x faster** overall than `main`.
- In the 200+ round region specifically, current averages 6.5 ms/click while `main` averages 155.9 ms/click, a gap of roughly **24x**.
- The implemented incremental update path removes the dominant source of high-round-count append slowdown seen in `main`.

---

## 2026-05-20 — Middle-Insert Replay From Round Checkpoints

**File:** `src/network.c`, `src/network.h`, `src/events.c`

**Problem:**  
The existing optimization only improved the case where `+` appends a new round at the end (`AppendLastRound`). If the user is positioned in the middle of the timeline, `+` inserts a round at `currentRound + 1`, which invalidates all later rounds. That path still fell back to `ExecuteNetwork()`, rebuilding the entire network from round 0.

This distinction mattered in benchmarking: a slow measurement taken while the cursor was near round 1 was not exercising the append-at-end optimization at all. It was measuring repeated middle inserts.

**Fix:**  
Added periodic round checkpoints and a suffix-replay path:

1. During `ExecuteNetwork()`, capture a checkpoint after round 1 and every 8 rounds thereafter.
2. Each checkpoint stores every entity's `history`, `current`, and `outdegree` at that prefix length.
3. Added `ExecuteNetworkFromRound(firstRound)`, which restores the nearest checkpoint at or before `firstRound`, trims invalid future checkpoints, and replays only the affected suffix.
4. In `events.c`, middle `+` insert and middle `-` delete now call `ExecuteNetworkFromRound(currentRound)` instead of full `ExecuteNetwork()`.

End-of-sequence append/delete behavior is unchanged:
- last-round append still uses `AppendLastRound()`
- last-round delete still uses `RollBackLastRound()`

**Measured effect:**

- Tutorial network, starting near round 1, repeated `+` until total rounds reached 250:
- before this change: about `22.1 s`
- after this change: `2.532 s`
- average: `10.34 ms/click`
- p95: `21.60 ms`

- End-of-sequence append benchmark was rechecked after the checkpoint work:
- `245` appends from round `5 -> 250` at the end of the timeline
- total: `1.115 s`
- average: `4.55 ms/click`
- p95: `6.23 ms`

**Gain:**  
Middle insert/delete no longer always pay the full cost of replaying from round 0. They now reuse the unchanged prefix and replay only the suffix from the nearest checkpoint. This does not make middle insert as cheap as append-at-end, but it removes the worst-case behavior that previously dominated interactive use away from the last round.

---

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

---

## 2026-05-07 — `AppendAuxDataOneLevel`: incremental AuxData update

**File:** `src/auxdata.c`, `src/auxdata.h`, `src/network.c`

**Problem:**  
`AppendLastRound` called `ComputeAuxData(finalHistory)`, which rebuilt all AuxData levels from scratch — O(R × n × obs) for red edges alone. Each `=` keypress was O(R), so appending rounds repeatedly grew progressively slower.

**Fix:**  
Added `AppendAuxDataOneLevel()`. After `ExtendFinalHistoryOneLevel` adds exactly one new level to `finalHistory`, this function:

1. Creates AuxData nodes for only the new deepest level (O(n)).
2. Recomputes widths bottom-up from the old deepest level to root (O(R × n)).
3. Recomputes coordinates for all levels — unavoidable since widths changed (O(R × n)).
4. Computes red edges **for the new level only**, using the same hash-map lookup as `ComputeAuxDataRedEdges` — O(n × obs) instead of O(R × n × obs).
5. Recomputes anonymities for all levels (O(R × n)) and resets algorithm state.

`AppendLastRound` now calls `AppendAuxDataOneLevel()` instead of `ComputeAuxData(finalHistory)`.

**Gain:**  
Red-edge computation drops from O(R × n × obs) to O(n × obs) per append. The O(R × n) passes (width, coordinate, anonymity) remain but are cheap compared to the red-edge rebuild. Interactive round-append no longer slows down as R grows.

---

## 2026-05-07 — Remove dead `ComputeHashBottomUp` calls

**File:** `src/network.c`

**Problem:**  
`RebuildFinalHistory` and `AppendLastRound` each called `ComputeHashBottomUp(finalHistory)` after building `finalHistory`. `ComputeHashBottomUp` is O(R × n²) (post-order DFS, sorts children at each node). The only consumer of these hashes was `HistoryTreeEquals`, which is never called anywhere in the codebase.

**Fix:**  
Removed both `ComputeHashBottomUp(finalHistory)` calls. `ComputeHashBottomUp` itself is kept (it may be useful for debugging), but is no longer invoked on hot paths.

**Gain:**  
Eliminates one O(R × n²) pass per `RebuildFinalHistory` / `AppendLastRound` call — a significant saving at high R.

---

## 2026-05-07 — `CopyHistoryTree`: direct BFS copy

**File:** `src/history_tree.c`

**Problem:**  
`CopyHistoryTree` was implemented via `MergeHistoryTrees(dest, src)` where `dest` was always a freshly allocated empty tree. `MergeHistoryTrees` allocated a `ChildEntry` hash map for each BFS node of `dest` to look up existing children — but since `dest` was empty, every lookup was a guaranteed miss and every map was immediately freed after zero useful work.

**Fix:**  
Replaced with a direct BFS copy that uses the `reference` field (same convention as `MergeHistoryTrees`) for src→dst mapping. Red edges point from level L to level L−1; BFS processes level L−1 before level L, so all red-edge targets already have their `reference` set when we need them — a single pass suffices.

**Gain:**  
Eliminates O(n) `calloc`/`free` calls (one hash-map allocation per node in the source tree). No algorithmic complexity change, but removes all per-node heap overhead for tree copies.

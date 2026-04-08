# Optimization Log

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

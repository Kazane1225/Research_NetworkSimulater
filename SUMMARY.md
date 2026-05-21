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

## History-Tree Hash

- Each `HistoryTree` node stores a Merkle-style structural hash.
- The hash is built from `input`, `outdegree`, and the sorted multiset of child hashes.
- Red edges are intentionally excluded because they point upward and would create a circular dependency during bottom-up hash construction.
- The hash is used as a fast inequality check before the full tree-isomorphism logic runs; it is a pruning step, not the final correctness check by itself.

## Crash Status

- Middle insert no longer fails around the old `~1000`-round region after the checkpoint redesign, but it still crashes later, currently before `1993` rounds.
- The current direction is to keep improving the replay/restore path rather than weakening the feature for large round counts.
- The reason is that the remaining instability is in the deep middle-insert checkpoint/replay path, while end-of-sequence append is still stable and fast.

## Measurement Notes

- Benchmark values depend on the machine, browser, viewport, and foreground/background tab state, so absolute times should be read as environment-specific.
- The most reliable reading is the relative comparison between `current` and `main` under the same machine and browser conditions.
- Environment: Windows, VS Code integrated browser.
- Branches: `feature/improve_algorithm` vs `main` in a separate worktree.
- Test data: `Module._TutorialLoadNetwork()`.
- End-of-sequence append was measured in the same foreground tab for both builds to avoid background-tab throttling.

For implementation details and chronological notes, see `DONE.md`.
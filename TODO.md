# TODO

## Low priority

### 1. Arena allocator for `HistoryTree` nodes
**File:** `src/history_tree.c`

Replace per-node `malloc`/`free` with a slab/arena allocator scoped to the lifetime of a network execution. Reduces allocator overhead when round counts are large.

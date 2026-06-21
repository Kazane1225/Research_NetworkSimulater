# RebuildFinalHistory Benchmark — Console Test Instructions

## Setup: Start 4 Servers

```powershell
# Terminal 1 (main branch)
node c:\myproject\Research_NetworkSimulater_main_wt\scripts\serve.js 9001

# Terminal 2 (merkle-only)
node c:\myproject\Research_NetworkSimulater_merkle_wt\scripts\serve.js 9002

# Terminal 3 (only-vista)
node c:\myproject\Research_NetworkSimulater\scripts\serve.js 9003

# Terminal 4 (merkle+vista)
node c:\myproject\Research_NetworkSimulater_merklevista_wt\scripts\serve.js 9004
```

## Test Networks (from the UI "Load" button)

| Network file        | Why test it |
|---------------------|-------------|
| `RegularStatic.txt` | All entities have the same input/outdegree → maximum hash-collision pressure on cmap / EquivalentNodes |
| `LowerBound2n.txt`  | Many distinct isomorphism classes → tests correctness across branches |
| `TerminatingWorstCase.txt` | Deep trees (many rounds) → stress-tests `MergeHistoryTrees` per-node matching |

## Test Procedure

1. Open each URL in a separate tab (port 9001–9004).
2. Load a network file.
3. Click **Execute** (run all rounds at once — NOT step-by-step to avoid the slow 1click/1s path).
4. Open DevTools console and run:

```js
// Reset counters first (if re-testing)
Module._ResetRebuildPerfMetrics();

// After executing:
let n = Module._GetRebuildSamples();
let sum = Module._GetSumRebuildMs();
let last = Module._GetLastRebuildMs();
console.log(`Rebuild calls: ${n}, total: ${sum.toFixed(3)} ms, last: ${last.toFixed(3)} ms, avg: ${(sum/n).toFixed(3)} ms`);
```

> **Note**: `RebuildFinalHistory` is called once per "Execute" and once per click in step-by-step mode. The sum includes all calls since page load (or last reset).

## What to Compare

| Metric | Expected trend |
|--------|----------------|
| `GetSumRebuildMs` on RegularStatic | main > merkle-only ≥ only-vista > merkle+vista |
| `GetRebuildSamples` | Same across all branches (same number of Execute calls) |

## Quick One-Liner (paste in console after Execute)

```js
[Module._GetRebuildSamples(), Module._GetLastRebuildMs().toFixed(3), Module._GetSumRebuildMs().toFixed(3)].join(" | samples | last ms | sum ms")
```

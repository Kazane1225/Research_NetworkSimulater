// Cooperative (single-thread) scheduler that spreads the cost of a full ExecuteNetwork()
// replay across multiple animation frames, so the renderer keeps producing frames (and a
// "computing" indicator can be shown) instead of blocking the whole tab for one long
// synchronous call.
//
// This never runs on a background thread: on each call to ComputeJob_Step(), a bounded
// slice of work is executed on the main thread, then control returns to the caller so a
// frame can be rendered. On native (non-Emscripten) builds this degrades to running the
// whole job to completion inside a single ComputeJob_Step() call (see compute_job.c),
// which reproduces the exact synchronous behaviour the app had before this feature existed.
//
// Scope: only the full replay path (ExecuteNetwork's round-replay + entity-merge loops) is
// chunked here. The snapshot-based incremental paths (ReExecuteLastRound/AppendLastRound/
// RollBackLastRound) are already O(1)-per-edit-ish and stay fully synchronous; they are not
// wrapped by this scheduler, but interactive call sites must still avoid invoking them while
// a chunked job is active (see invariant 1 below), since they mutate shared state
// (currentMutationRound, finalHistory, aux, entity->history/snap) that a paused job also owns.
//
// IMPORTANT invariants:
//   1) Interactive edits (add/delete entity, add/delete round, add/delete interaction,
//      reorder entities, toggle outAware) -- whether they end up calling ExecuteNetwork,
//      ReExecuteLastRound, AppendLastRound, or RollBackLastRound -- must check
//      ComputeJob_IsActive() and skip the whole edit (mutation + recompute) while a job is
//      still running, instead of applying it immediately. This deliberately reproduces the
//      pacing the previous fully-synchronous code had -- e.g. holding down "+" fires native
//      key-repeat events far faster than any single recompute can complete, and letting every
//      one of those mutate `network` immediately lets the round count grow far beyond what
//      was ever practically reachable before, which can hit unrelated pre-existing
//      recursion-depth limits in the history-tree code. Skipping the edit while busy keeps
//      the effective edit rate bounded by how fast a recompute can actually finish, exactly
//      like before.
//   2) Code that unconditionally replaces or frees `network` (DoneNetwork, LoadNetworkHelper,
//      and defensively ExecuteNetwork itself) must call ComputeJob_CancelActive() first, so a
//      job that is paused mid-flight never resumes against a `network` it no longer matches.

typedef void (*ComputeJobCallback)(void *userdata);

void ComputeJob_Init(void);

// Call once per frame (from the main loop), after Events() and before rendering.
void ComputeJob_Step(void);

bool ComputeJob_IsActive(void);
double ComputeJob_ElapsedMs(void); // elapsed time of the currently active job; 0 if none

// Discards any in-flight (not yet committed) recompute job. Safe to call even if no job is
// active. Must be called before mutating/replacing `network`'s shape (see invariant 2 above).
void ComputeJob_CancelActive(void);

// Starts a fresh recompute job equivalent to calling ExecuteNetwork() synchronously, chunked
// across frames. Once the job commits (finalHistory/aux/entity->finalLeaf all updated together):
//   1) onComplete(userdata) runs, if non-NULL (e.g. to re-select a node via SelectNodeFromEntity,
//      which needs the just-rebuilt aux data);
//   2) CountingAlgorithm() runs, if runCountingAlgorithm is true (caller must set `numSteps`
//      beforehand, exactly like the previous synchronous call sites did);
//   3) win1->invalid is set so the final state is rendered.
// Must be called only after the corresponding network mutation has already been applied, and
// only when ComputeJob_IsActive() was false (see invariant 1 above).
void ComputeJob_RequestRecompute(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm);

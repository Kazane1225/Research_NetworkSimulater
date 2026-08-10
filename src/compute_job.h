// Cooperative (single-thread) scheduler that spreads the cost of ExecuteNetwork() across
// multiple animation frames, so the renderer keeps producing frames (and a "computing"
// indicator can be shown) instead of blocking the whole tab for one long synchronous call.
//
// This never runs on a background thread: on each call to ComputeJob_Step(), a bounded
// slice of work is executed on the main thread, then control returns to the caller so a
// frame can be rendered. On native (non-Emscripten) builds this degrades to running the
// whole job to completion inside a single ComputeJob_Step() call (see compute_job.c),
// which reproduces the exact synchronous behaviour the app had before this feature existed.
//
// IMPORTANT invariants:
//   1) Interactive edits (add/delete entity, add/delete round, add/delete interaction,
//      reorder entities, toggle outAware) must check ComputeJob_IsActive() and skip the
//      whole edit (mutation + recompute request) while a job is still running, instead of
//      applying it immediately. This deliberately reproduces the pacing the fully-synchronous
//      code used to have -- e.g. holding down "+" fires native key-repeat events far faster
//      than any single recompute can complete, and letting every one of those mutate
//      `network` immediately (even just to queue/cancel jobs) lets the round count grow far
//      beyond what was ever practically reachable before, which can hit unrelated existing
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
// active. Must be called before mutating `network`'s shape (see invariant above).
void ComputeJob_CancelActive(void);

// Starts a fresh recompute job equivalent to calling ExecuteNetwork() synchronously, chunked
// across frames. Once the job commits (finalHistory/aux/entity->finalLeaf all updated together):
//   1) onComplete(userdata) runs, if non-NULL (e.g. to re-select a node via SelectNodeFromEntity,
//      which needs the just-rebuilt aux data);
//   2) CountingAlgorithm() runs, if runCountingAlgorithm is true (caller must set `numSteps`
//      beforehand, exactly like the previous synchronous call sites did);
//   3) win1->invalid is set so the final state is rendered.
// Must be called only after the corresponding network mutation has already been applied.
void ComputeJob_RequestRecompute(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm);

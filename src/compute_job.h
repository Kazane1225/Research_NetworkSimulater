// Real multi-threaded compute worker: a single persistent pthread runs ExecuteNetwork() (the
// full-replay path) so the main thread (input handling + rendering) is never blocked by it, no
// matter how large the network is.
//
// This is a genuine OS/browser-level thread (Emscripten pthreads backed by a Web Worker and
// SharedArrayBuffer), not a cooperative single-thread scheduler. On native (non-Emscripten)
// builds this degrades to running ExecuteNetwork() synchronously on the calling thread, so
// native behaviour is unchanged.
//
// Scope: only ExecuteNetwork() is dispatched to the worker. The already-fast, snapshot-based
// incremental paths (ReExecuteLastRound/AppendLastRound/RollBackLastRound) intentionally mutate
// finalHistory/aux *in place* rather than rebuild them -- that in-place mutation is exactly what
// makes them fast, but it also means they cannot safely run concurrently with rendering (which
// reads that same tree every frame) without either a full copy first (undermining the whole
// point of being incremental) or fine-grained locking that would just reintroduce blocking by a
// different name. They therefore remain fully synchronous on the main thread, unchanged from
// before; see events.c's RunRecompute() for how the two are dispatched side by side.
//
// Ownership / synchronization model (see compute_job.c for the implementation):
//   - `aux` and `finalHistory` are _Thread_local (see auxdata.h/network.h). The worker builds a
//     brand new tree into its own thread-local slots on every job (never reusing or freeing
//     whatever it built last time, since the main thread may still be that data's only owner);
//     the main thread's slots are only ever assigned a finished pointer value at commit time,
//     handed over through a mutex-protected result slot, and the main thread is responsible for
//     freeing whatever it previously held once it stops being needed.
//   - Entity->finalLeaf is genuinely shared (a field on a shared struct), so the worker never
//     writes it directly. ExecuteNetwork() writes into a caller-provided outFinalLeaf array
//     instead; ComputeJob_Step() copies that array into each Entity->finalLeaf only once the job
//     has fully committed.
//   - Entity->history/current/mailbox/outdegree/snap and Entity->input, plus network->entities/
//     network->rounds, are exclusively owned by the worker for the duration of a dispatched job.
//     The main thread must not mutate them (nor call the synchronous incremental functions,
//     which touch the same fields) while ComputeJob_IsActive() is true -- every call site that
//     would do so must check ComputeJob_IsActive() first and skip the edit (see events.c), so a
//     job in flight always operates on a `network` that nothing else concurrently mutates.
//   - Code that unconditionally replaces or frees `network` (DoneNetwork, LoadNetworkHelper)
//     must call ComputeJob_WaitForIdle() first, since a job cannot simply be "cancelled" once a
//     real thread is executing it.

typedef void (*ComputeJobCallback)(void *userdata);

void ComputeJob_Init(void);

// Call once per frame (from the main loop), after Events() and before rendering. Cheap when no
// job is outstanding; when the worker has finished, applies its result (commits finalLeaf/aux)
// and runs the completion callback / CountingAlgorithm on the main thread.
void ComputeJob_Step(void);

// True from the moment a job is dispatched until its result has been fully committed (i.e. until
// ComputeJob_Step() has applied it). Interactive edits must check this before mutating `network`
// or calling any of the synchronous incremental recompute functions.
bool ComputeJob_IsActive(void);
double ComputeJob_ElapsedMs(void); // wall-clock time since the active job was dispatched; 0 if none

// Blocks the calling (main) thread until any in-flight job has fully committed. Required before
// code that unconditionally frees/replaces `network` (see DoneNetwork).
void ComputeJob_WaitForIdle(void);

// Dispatches ExecuteNetwork() to the worker thread; must be called only when
// ComputeJob_IsActive() is false, and only after the corresponding network mutation has already
// been applied on the main thread. Once the worker finishes and ComputeJob_Step() commits the
// result:
//   1) onComplete(userdata) runs, if non-NULL (e.g. to re-select a node via SelectNodeFromEntity,
//      which needs the just-committed aux data);
//   2) CountingAlgorithm() runs, if runCountingAlgorithm is true (caller must set `numSteps`
//      beforehand, exactly like the previous synchronous call sites did);
//   3) win1->invalid is set so the final state is rendered.
// All of this (steps 1-3) runs on the main thread, matching what used to happen synchronously
// right after the old blocking call.
void ComputeJob_DispatchExecuteNetwork(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm);

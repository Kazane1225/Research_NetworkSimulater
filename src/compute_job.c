#include "main.h"

#ifdef __EMSCRIPTEN__
#include <pthread.h>
#include <SDL3/SDL.h>

typedef enum{ CJ_IDLE, CJ_DISPATCHED, CJ_RUNNING, CJ_DONE }ComputeJobState;

static pthread_t workerThread;
static bool workerCreated=false;

// Guards every field below this line up to (not including) the "main-thread only" block.
static pthread_mutex_t jobMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t jobCond=PTHREAD_COND_INITIALIZER; // signaled on every state transition

static ComputeJobState state=CJ_IDLE;
static int jobEntityCount=0;
static HistoryTree **jobOutFinalLeaf=NULL;      // array contents written by the worker, read by the main thread once state==CJ_DONE has been observed
static Vector *jobResultAux=NULL;               // set by the worker just before it publishes CJ_DONE
static HistoryTree *jobResultFinalHistory=NULL; // same; only used by the main thread for bookkeeping/freeing, never dereferenced there

// Main-thread only: set at dispatch, consumed at commit. Never touched by the worker.
static ComputeJobCallback pendingOnComplete=NULL;
static void *pendingOnCompleteData=NULL;
static bool pendingRunCounting=false;
static double jobStartMs=0.0;

static double PerfNowMs(void){
    static double freq=0.0;
    if(freq==0.0)freq=(double)SDL_GetPerformanceFrequency();
    return 1000.0*(double)SDL_GetPerformanceCounter()/freq;
}

static void *WorkerMain(void *arg){
    (void)arg;
    for(;;){
        pthread_mutex_lock(&jobMutex);
        while(state!=CJ_DISPATCHED)pthread_cond_wait(&jobCond,&jobMutex);
        state=CJ_RUNNING;
        HistoryTree **outFinalLeaf=jobOutFinalLeaf;
        pthread_mutex_unlock(&jobMutex);

        // Exclusive compute phase. Every interactive call site checks ComputeJob_IsActive()
        // before mutating `network` or calling one of the synchronous incremental recompute
        // functions (see events.c), so nothing else touches `network`, its entities'
        // compute-scratch fields, or this thread's own aux/finalHistory slots while this runs.
        //
        // Discard (do not free) this thread's previous aux/finalHistory: the main thread now
        // owns whatever they last pointed to (it was handed the same pointer values at the
        // previous commit) and is responsible for freeing it once nothing references it
        // anymore. Freeing it here would race with the main thread, which may still be
        // rendering from it right up until this job's own commit.
        finalHistory=NULL;
        aux=NULL;
        ExecuteNetwork(outFinalLeaf);

        pthread_mutex_lock(&jobMutex);
        jobResultAux=aux;
        jobResultFinalHistory=finalHistory;
        state=CJ_DONE;
        pthread_cond_broadcast(&jobCond);
        pthread_mutex_unlock(&jobMutex);
    }
    return NULL;
}

void ComputeJob_Init(void){
    if(workerCreated)return;
    if(pthread_create(&workerThread,NULL,WorkerMain,NULL)!=0)
        Exit("Unable to create compute worker thread.");
    workerCreated=true;
}

bool ComputeJob_IsActive(void){
    pthread_mutex_lock(&jobMutex);
    bool active=(state!=CJ_IDLE);
    pthread_mutex_unlock(&jobMutex);
    return active;
}

double ComputeJob_ElapsedMs(void){
    if(!ComputeJob_IsActive())return 0.0;
    return PerfNowMs()-jobStartMs;
}

void ComputeJob_WaitForIdle(void){
    pthread_mutex_lock(&jobMutex);
    while(state==CJ_DISPATCHED||state==CJ_RUNNING)pthread_cond_wait(&jobCond,&jobMutex);
    bool needsCommit=(state==CJ_DONE);
    pthread_mutex_unlock(&jobMutex);
    if(needsCommit)ComputeJob_Step(); // commit synchronously on the calling (main) thread
}

void ComputeJob_DispatchExecuteNetwork(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm){
    int n=network->entities->tot;
    pendingOnComplete=onComplete;
    pendingOnCompleteData=userdata;
    pendingRunCounting=runCountingAlgorithm;
    jobStartMs=PerfNowMs();

    pthread_mutex_lock(&jobMutex);
    jobEntityCount=n;
    jobOutFinalLeaf=malloc((size_t)(n>0?n:1)*sizeof(HistoryTree*));
    state=CJ_DISPATCHED;
    pthread_cond_broadcast(&jobCond);
    pthread_mutex_unlock(&jobMutex);

    win1->invalid=true;
}

void ComputeJob_Step(void){
    pthread_mutex_lock(&jobMutex);
    if(state!=CJ_DONE){pthread_mutex_unlock(&jobMutex);return;}
    int n=jobEntityCount;
    HistoryTree **outFinalLeaf=jobOutFinalLeaf;
    Vector *newAux=jobResultAux;
    HistoryTree *newFinalHistory=jobResultFinalHistory;
    jobOutFinalLeaf=NULL;
    jobResultAux=NULL;
    jobResultFinalHistory=NULL;
    state=CJ_IDLE;
    pthread_cond_broadcast(&jobCond);
    pthread_mutex_unlock(&jobMutex);

    // Commit: everything below runs on the main thread only, and only after nothing else
    // (including the worker, which just discarded its own reference above) still points at the
    // previous aux/finalHistory, so it is now safe to free.
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=outFinalLeaf[i];
    free(outFinalLeaf);
    FreeAuxData(); // frees the main thread's previous aux (no-op if it was already NULL)
    aux=newAux;
    FreeHistoryTree(finalHistory); // frees the main thread's previous finalHistory (no-op if NULL)
    finalHistory=newFinalHistory;

    ComputeJobCallback cb=pendingOnComplete;
    void *data=pendingOnCompleteData;
    bool runCounting=pendingRunCounting;
    pendingOnComplete=NULL;
    pendingOnCompleteData=NULL;
    pendingRunCounting=false;

    if(cb)cb(data);
    if(runCounting)CountingAlgorithm();
    win1->invalid=true;
}

#else // native (non-Emscripten): no threading, run synchronously, behaviour unchanged from before

void ComputeJob_Init(void){ }
void ComputeJob_Step(void){ }
bool ComputeJob_IsActive(void){ return false; }
double ComputeJob_ElapsedMs(void){ return 0.0; }
void ComputeJob_WaitForIdle(void){ }

void ComputeJob_DispatchExecuteNetwork(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm){
    int n=network->entities->tot;
    HistoryTree **outFinalLeaf=malloc((size_t)(n>0?n:1)*sizeof(HistoryTree*));
    ExecuteNetwork(outFinalLeaf);
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=outFinalLeaf[i];
    free(outFinalLeaf);
    if(onComplete)onComplete(userdata);
    if(runCountingAlgorithm)CountingAlgorithm();
    win1->invalid=true;
}

#endif

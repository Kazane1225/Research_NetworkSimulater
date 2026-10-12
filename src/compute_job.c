#include "main.h"

// Maximum time spent per frame advancing an in-flight job. Chosen to leave the rest of a
// 16.6ms (60fps) frame free for input handling and rendering. On native builds PerfNowMs()
// always returns 0, so this budget is never exceeded and a job always runs to completion
// inside a single ComputeJob_Step() call -- i.e. native behaves exactly as before.
#define COMPUTE_JOB_FRAME_BUDGET_MS 8.0

typedef enum{
    CJ_IDLE,
    CJ_ROUNDS
}ComputeJobPhase;

static ComputeJobPhase phase=CJ_IDLE;
static double jobStartMs=0.0;
static double accumulatedComputeMs=0.0;

static ComputeJobCallback pendingOnComplete=NULL;
static void *pendingOnCompleteData=NULL;
static bool pendingRunCounting=false;

static double PerfNowMs(void){
    static double freq=0.0;
#ifdef __EMSCRIPTEN__
    if(freq==0.0)freq=(double)SDL_GetPerformanceFrequency();
    return 1000.0*(double)SDL_GetPerformanceCounter()/freq;
#else
    return 0.0;
#endif
}

static void DiscardActiveJob(void){
    phase=CJ_IDLE;
}

void ComputeJob_Init(void){
    phase=CJ_IDLE;
    pendingOnComplete=NULL;
    pendingOnCompleteData=NULL;
    pendingRunCounting=false;
    accumulatedComputeMs=0.0;
}

bool ComputeJob_IsActive(void){
    return phase!=CJ_IDLE;
}

double ComputeJob_ElapsedMs(void){
    return phase==CJ_IDLE?0.0:PerfNowMs()-jobStartMs;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE int IsComputeJobBusy(void){
    return ComputeJob_IsActive()?1:0;
}

EMSCRIPTEN_KEEPALIVE double GetComputeJobElapsedMs(void){
    return ComputeJob_ElapsedMs();
}
#endif

void ComputeJob_CancelActive(void){
    DiscardActiveJob();
    pendingOnComplete=NULL;
    pendingOnCompleteData=NULL;
    pendingRunCounting=false;
}

static void FinishJob(void){
    ExecuteNetworkReplayFinish();
    phase=CJ_IDLE;
    RecordRebuildPerf(accumulatedComputeMs);
    accumulatedComputeMs=0.0;

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

void ComputeJob_RequestRecompute(ComputeJobCallback onComplete,void *userdata,bool runCountingAlgorithm){
    DiscardActiveJob();
    pendingOnComplete=onComplete;
    pendingOnCompleteData=userdata;
    pendingRunCounting=runCountingAlgorithm;
    jobStartMs=PerfNowMs();
    accumulatedComputeMs=0.0;
    ExecuteNetworkReplayBegin(-1);
    phase=CJ_ROUNDS;
    win1->invalid=true;
}

void ComputeJob_Step(void){
    if(phase==CJ_IDLE)return;
    double stepStart=PerfNowMs();
    double budgetEnd=stepStart+COMPUTE_JOB_FRAME_BUDGET_MS;
    for(;;){
        if(ExecuteNetworkReplayStep()){
            accumulatedComputeMs+=PerfNowMs()-stepStart;
            FinishJob();
            return;
        }
        if(PerfNowMs()>=budgetEnd)break;
    }
    accumulatedComputeMs+=PerfNowMs()-stepStart;
    win1->invalid=true;
}

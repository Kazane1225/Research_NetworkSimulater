#include "main.h"

// Maximum time spent per frame advancing an in-flight job. Chosen to leave the rest of a
// 16.6ms (60fps) frame free for input handling and rendering. On native builds PerfNowMs()
// always returns 0, so this budget is never exceeded and a job always runs to completion
// inside a single ComputeJob_Step() call -- i.e. native behaves exactly as before.
#define COMPUTE_JOB_FRAME_BUDGET_MS 8.0

typedef enum{
    CJ_IDLE,
    CJ_ROUNDS, // replaying rounds [nextRound, network->rounds->tot)
    CJ_MERGE   // merging entities [nextEntity, network->entities->tot) into newFinalHistory
}ComputeJobPhase;

static ComputeJobPhase phase=CJ_IDLE;
static int nextRound=0;
static int nextEntity=0;
static HistoryTree *newFinalHistory=NULL;
static HistoryTree **newFinalLeaf=NULL;
static double jobStartMs=0.0;
static double accumulatedComputeMs=0.0; // actual busy time spent on this job, excluding idle time between frames

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
    if(phase==CJ_MERGE){
        if(newFinalHistory)FreeHistoryTree(newFinalHistory);
        if(newFinalLeaf)free(newFinalLeaf);
    }
    newFinalHistory=NULL;
    newFinalLeaf=NULL;
    phase=CJ_IDLE;
}

void ComputeJob_Init(void){
    phase=CJ_IDLE;
    newFinalHistory=NULL;
    newFinalLeaf=NULL;
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

void ComputeJob_CancelActive(void){
    DiscardActiveJob();
    pendingOnComplete=NULL;
    pendingOnCompleteData=NULL;
    pendingRunCounting=false;
}

static void StartRoundsPhase(void){
    ExecuteNetworkResetEntities();
    nextRound=0;
    phase=CJ_ROUNDS;
}

static void StartMergePhase(void){
    int n=network->entities->tot;
    newFinalHistory=NewHistoryTree();
    newFinalLeaf=malloc((size_t)(n>0?n:1)*sizeof(HistoryTree*));
    nextEntity=0;
    phase=CJ_MERGE;
}

// Commits the freshly computed finalHistory/finalLeaf/aux all at once, so nothing ever
// observes a state where only some entities' finalLeaf point into the new tree.
static void CommitAndFinish(void){
    int n=network->entities->tot;
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=newFinalHistory;
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=newFinalLeaf[i];
    free(newFinalLeaf);
    newFinalHistory=NULL;
    newFinalLeaf=NULL;
    phase=CJ_IDLE;

    ComputeAuxData(finalHistory);
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
    StartRoundsPhase();
    win1->invalid=true;
}

void ComputeJob_Step(void){
    if(phase==CJ_IDLE)return;
    double stepStart=PerfNowMs();
    double budgetEnd=stepStart+COMPUTE_JOB_FRAME_BUDGET_MS;
    for(;;){
        if(phase==CJ_ROUNDS){
            if(nextRound>=network->rounds->tot){
                StartMergePhase();
            }
            else{
                ExecuteNetworkRunRound(nextRound);
                nextRound++;
            }
        }
        else if(phase==CJ_MERGE){
            int n=network->entities->tot;
            if(nextEntity>=n){
                accumulatedComputeMs+=PerfNowMs()-stepStart;
                CommitAndFinish();
                return;
            }
            else{
                Entity *e=GetEntity(nextEntity);
                newFinalLeaf[nextEntity]=ExecuteNetworkMergeEntity(newFinalHistory,e);
                nextEntity++;
            }
        }
        else break;
        if(phase==CJ_IDLE)break;
        if(PerfNowMs()>=budgetEnd)break;
    }
    accumulatedComputeMs+=PerfNowMs()-stepStart;
    win1->invalid=true;
}

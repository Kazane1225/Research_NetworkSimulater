#include "main.h"

Network *network=NULL;
int currentRound=-1;
HistoryTree *finalHistory=NULL;
int selectedEntity=-1;
int selectedNodeI=-1,selectedNodeJ=-1;
bool drawingEdge=false;
bool draggingEntity=false;
int algorithm=0;
int numSteps=-1;

typedef struct {
    HistoryTree *current;
    int outdegree;
} RoundCheckpointState;

typedef struct {
    int prefixRounds;
    int numEntities;
    RoundCheckpointState *states;
} RoundCheckpoint;

typedef struct {
    double lastSnapshotMs;
    double lastExecuteMs;
    double lastFinalHistoryMs;
    double lastAuxMs;
    double lastTotalMs;
    double sumSnapshotMs;
    double sumExecuteMs;
    double sumFinalHistoryMs;
    double sumAuxMs;
    double sumTotalMs;
    int samples;
} AppendPerfStats;

static AppendPerfStats appendPerfStats={0};
static double lastRebuildMs=0.0;
static double sumRebuildMs=0.0;
static int rebuildSamples=0;
static Vector *roundCheckpoints=NULL;

/* -1 = clean (simulation is up to date).
   >=0 = dirty: simulation needs to be re-run from this round before the next
   display.  Set by MarkNetworkDirtyFromRound(); cleared by EnsureNetworkComputed()
   or by any direct call to ExecuteNetwork / ExecuteNetworkFromRound. */
static int dirtyFromRound=-1;
static Entity *pendingSelectRestore=NULL;

void MarkNetworkDirtyFromRound(int firstRound){
    if(dirtyFromRound<0 || firstRound<dirtyFromRound) dirtyFromRound=firstRound;
}

void QueueSelectRestore(Entity *e){
    pendingSelectRestore=e;
}

bool IsNetworkDirty(void){
    return dirtyFromRound>=0;
}

/* Run any deferred recomputation.  Returns true iff a recompute was performed.
   Call once per frame (e.g. in MainLoop) before rendering. */
bool EnsureNetworkComputed(void){
    if(dirtyFromRound<0) return false;
    int dirty=dirtyFromRound;
    dirtyFromRound=-1;
    if(dirty<=0) ExecuteNetwork();
    else ExecuteNetworkFromRound(dirty);
    if(pendingSelectRestore){
        SelectNodeFromEntity(pendingSelectRestore);
        pendingSelectRestore=NULL;
    }
    return true;
}

/* Keep enough lightweight checkpoints for middle edits to restart close to the
   edited round.  Each checkpoint stores only per-entity pointers/outdegree, not
   copied history trees, so the memory cost stays small compared with HistoryTree
   storage. */
enum { ROUND_CHECKPOINT_INTERVAL = 4 };
enum { ROUND_CHECKPOINT_MAX_STORED = 64 };

static double PerfNowMs(void){
    static double freq=0.0;
    if(freq==0.0)freq=(double)SDL_GetPerformanceFrequency();
    return 1000.0*(double)SDL_GetPerformanceCounter()/freq;
}

static void ResetAppendPerfStats(void){
    appendPerfStats=(AppendPerfStats){0};
}

static void RecordAppendPerf(double snapshotMs,double executeMs,double finalHistoryMs,double auxMs,double totalMs){
    appendPerfStats.lastSnapshotMs=snapshotMs;
    appendPerfStats.lastExecuteMs=executeMs;
    appendPerfStats.lastFinalHistoryMs=finalHistoryMs;
    appendPerfStats.lastAuxMs=auxMs;
    appendPerfStats.lastTotalMs=totalMs;
    appendPerfStats.sumSnapshotMs+=snapshotMs;
    appendPerfStats.sumExecuteMs+=executeMs;
    appendPerfStats.sumFinalHistoryMs+=finalHistoryMs;
    appendPerfStats.sumAuxMs+=auxMs;
    appendPerfStats.sumTotalMs+=totalMs;
    appendPerfStats.samples++;
}

Entity *GetEntity(int i){
    return network->entities->items[i];
}

int GetEntityIndex(Entity *e){
    for(int i=0;i<network->entities->tot;i++)
        if(e==GetEntity(i))return i;
    return -1;
}

static Interaction *NewInteraction(int i,int j,int multiplicity){
    Interaction *interaction=malloc(sizeof(Interaction));
    interaction->e1=GetEntity(i);
    interaction->e2=GetEntity(j);
    interaction->multiplicity=multiplicity;
    return interaction;
}

static Interaction *CopyInteraction(Interaction *interaction){
    Interaction *interaction2=malloc(sizeof(Interaction));
    interaction2->e1=interaction->e1;
    interaction2->e2=interaction->e2;
    interaction2->multiplicity=interaction->multiplicity;
    return interaction2;
}

static void ExecuteInteraction(Interaction *interaction){
    SendHistory(interaction->e1,interaction->e2,interaction->multiplicity);
}

void SortLeaders(void){ // put all leaders at the beginning of the list of entities
    int l=0;
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        if(e->input==0){
            if(i>l){
                network->entities->items[i]=GetEntity(l);
                network->entities->items[l]=e;
                if(selectedEntity==i)selectedEntity=l;
                else if(selectedEntity==l)selectedEntity=i;
            }
            l++;
        }
    }
}

void InitNetwork(int type,int n){
    drawingEdge=draggingEntity=false;
    selectedEntity=selectedNodeI=selectedNodeJ=-1;
    selectedNode=NULL;
    currentRound=-1;
    DoneNetwork();
    network=malloc(sizeof(Network));
    network->entities=NewVector(8);
    network->rounds=NewVector(8);
    switch(type){
        case 1: ExampleNetwork1(n); break;
        case 2: ExampleNetwork2(n); break;
        case 3: ExampleNetwork3(n); break;
        case 4: ExampleNetwork4(n); break;
        case 5: ExampleNetwork5(); break;
        case 6: ExampleNetwork6(); break;
        default: break;
    }
    if(network->rounds->tot)currentRound=0;
    ExecuteNetwork();
    win1->invalid=true;
}

/* ── incremental execution helpers ────────────────────────────────────────── */

/* Returns the node in copy_root's tree that corresponds to orig_current in orig_root's tree.
   Works by recording the child-index path from orig_current back to orig_root, then following
   it in copy_root.  The two trees must be structurally identical up to orig_current. */
static HistoryTree *FindCurrentInCopy(HistoryTree *orig_root,HistoryTree *orig_current,HistoryTree *copy_root){
    if(orig_current==orig_root)return copy_root;
    int depth=0;
    HistoryTree *n=orig_current;
    while(n->parent){n=n->parent;depth++;}
    int *path=malloc((size_t)depth*sizeof(int));
    n=orig_current;
    for(int i=depth-1;i>=0;i--){
        HistoryTree *p=n->parent;
        for(int j=0;j<p->children->tot;j++){
            if(p->children->items[j]==n){path[i]=j;break;}
        }
        n=p;
    }
    n=copy_root;
    for(int i=0;i<depth;i++)n=n->children->items[path[i]];
    free(path);
    return n;
}

static void ClearEntityMailbox(Entity *e){
    for(int j=0;j<e->mailbox->tot;j++){
        Observation *m=e->mailbox->items[j];
        FreeHistoryTree(m->history);
        free(m);
    }
    FreeVector(e->mailbox);
    e->mailbox=NewVector(4);
}

static void FreeEntitySnap(Entity *e){
    if(e->snap){
        FreeHistoryTree(e->snap->history);
        free(e->snap);
        e->snap=NULL;
    }
}

static void FreeRoundCheckpoint(RoundCheckpoint *cp){
    if(!cp)return;
    free(cp->states);
    free(cp);
}

static void FreeRoundCheckpoints(void){
    if(!roundCheckpoints)return;
    for(int i=0;i<roundCheckpoints->tot;i++)
        FreeRoundCheckpoint(roundCheckpoints->items[i]);
    FreeVector(roundCheckpoints);
    roundCheckpoints=NULL;
}

static bool HasRoundCheckpoint(int prefixRounds){
    if(!roundCheckpoints)return false;
    for(int i=0;i<roundCheckpoints->tot;i++){
        RoundCheckpoint *cp=roundCheckpoints->items[i];
        if(cp->prefixRounds==prefixRounds)return true;
    }
    return false;
}

static RoundCheckpoint *CaptureRoundCheckpoint(int prefixRounds){
    int n=network->entities->tot;
    RoundCheckpoint *cp=malloc(sizeof(RoundCheckpoint));
    cp->prefixRounds=prefixRounds;
    cp->numEntities=n;
    cp->states=calloc((size_t)n,sizeof(RoundCheckpointState));
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        cp->states[i].current=e->current;
        cp->states[i].outdegree=e->outdegree;
    }
    return cp;
}

static void MaybeCaptureRoundCheckpoint(int prefixRounds){
    /* No upper bound against network->rounds->tot: during pure tail-append growth
       (AppendLastRound -> ExecuteRoundIncremental), prefixRounds always equals the
       current tot at the moment of the call, so requiring prefixRounds<tot would
       silently skip every capture attempt and leave the checkpoint cache stuck at
       whatever was captured at load time. A checkpoint "at the current tail" is
       harmless (FindRoundCheckpoint never selects it for the edit that just
       produced it, since that edit's firstRound is always < tot-1) and becomes a
       normal, useful interior checkpoint as soon as more rounds are appended after it. */
    if(prefixRounds<=0)return;
    if(prefixRounds!=1 && prefixRounds%ROUND_CHECKPOINT_INTERVAL!=0)return;
    if(!roundCheckpoints)roundCheckpoints=NewVector(8);
    if(HasRoundCheckpoint(prefixRounds))return;
    AddVector(roundCheckpoints,CaptureRoundCheckpoint(prefixRounds));
    while(roundCheckpoints->tot>ROUND_CHECKPOINT_MAX_STORED){
        int dropIndex=0;
        RoundCheckpoint *oldest=roundCheckpoints->items[0];
        if(oldest->prefixRounds==1 && roundCheckpoints->tot>1)dropIndex=1;
        FreeRoundCheckpoint(DeinsertVector(roundCheckpoints,dropIndex));
    }
}

static RoundCheckpoint *FindRoundCheckpoint(int firstRound){
    RoundCheckpoint *best=NULL;
    if(!roundCheckpoints)return NULL;
    for(int i=0;i<roundCheckpoints->tot;i++){
        RoundCheckpoint *cp=roundCheckpoints->items[i];
        if(cp->prefixRounds>firstRound)continue;
        if(!best || cp->prefixRounds>best->prefixRounds)best=cp;
    }
    return best;
}

static void TrimRoundCheckpointsAfter(int prefixRounds){
    if(!roundCheckpoints)return;
    Vector *kept=NewVector(8);
    for(int i=0;i<roundCheckpoints->tot;i++){
        RoundCheckpoint *cp=roundCheckpoints->items[i];
        if(cp->prefixRounds<=prefixRounds)AddVector(kept,cp);
        else FreeRoundCheckpoint(cp);
    }
    FreeVector(roundCheckpoints);
    roundCheckpoints=kept;
}

static bool RestoreRoundCheckpoint(RoundCheckpoint *cp){
    if(!cp || cp->numEntities!=network->entities->tot)return false;
    for(int i=0;i<cp->numEntities;i++){
        Entity *e=GetEntity(i);
        HistoryTree *target=cp->states[i].current;
        int depth=0;
        for(HistoryTree *n=target;n&&n->parent;n=n->parent)depth++;
        int *path=depth?malloc((size_t)depth*sizeof(int)):NULL;
        if(depth){
            HistoryTree *n=target;
            for(int d=depth-1;d>=0;d--){
                HistoryTree *p=n->parent;
                for(int j=0;j<p->children->tot;j++){
                    if(p->children->items[j]==n){path[d]=j;break;}
                }
                n=p;
            }
        }
        ClearEntityMailbox(e);
        FreeEntitySnap(e);
        TrimHistoryTreeToRound(e->history,cp->prefixRounds);
        HistoryTree *n=e->history;
        for(int d=0;d<depth&&n;d++){
            if(path[d]>=n->children->tot){n=NULL;break;}
            n=n->children->items[path[d]];
        }
        if(!n){
            /* Should be unreachable: maxBornRound-based pruning in TrimHistoryTreeToRound
               must never remove a node on the path down to a checkpointed entity's own
               `current` node. Falling back to the tree root avoids a crash, but the
               displayed round for this entity would be wrong, so surface it loudly. */
            fprintf(stderr,"RestoreRoundCheckpoint: lost path to entity %d's checkpointed node (prefixRounds=%d); falling back to history root\n",i,cp->prefixRounds);
            n=e->history;
        }
        e->current=n;
        free(path);
        e->outdegree=cp->states[i].outdegree;
        e->finalLeaf=NULL;
    }
    return true;
}

static void RebuildFinalHistory(void);
static void TakeSnapshotsBeforeRound(void);
static void InitFinalHistoryFromEntities(void);
static void ExecuteRoundIncremental(int r);

typedef struct{HistoryTree *target;int mult;}RedInfo;
static int cmp_redinfo(const void *a,const void *b){
    uintptr_t pa=(uintptr_t)((const RedInfo *)a)->target;
    uintptr_t pb=(uintptr_t)((const RedInfo *)b)->target;
    return (pa>pb)-(pa<pb);
}
static void ExtendFinalHistoryOneLevel(HistoryTree **prevFL,RedInfo **infos,int *counts){
    int n=network->entities->tot;
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        HistoryTree *par=prevFL[i];
        int nc=counts[i];
        HistoryTree *node=NULL;
        for(int j=0;j<par->children->tot&&!node;j++){
            HistoryTree *z=par->children->items[j];
            if(z->input!=e->current->input||z->outdegree!=e->current->outdegree||z->observations->tot!=nc)continue;
            bool ok=true;
            for(int k=0;k<nc&&ok;k++){
                Observation *o=z->observations->items[k];
                if(o->history!=infos[i][k].target||o->multiplicity!=infos[i][k].mult)ok=false;
            }
            if(ok)node=z;
        }
        if(!node){
            node=AddHistoryTreeChild(par,e->current->input);
            node->outdegree=e->current->outdegree;
            for(int k=0;k<nc;k++)
                AddRedEdge(node,infos[i][k].target,infos[i][k].mult);
        }
        e->finalLeaf=node;
    }
}

static void InitFinalHistoryFromEntities(void){
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=NewHistoryTree();
    int n=network->entities->tot;
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        e->finalLeaf=MergeHistoryTrees(finalHistory,e->history,NULL);
    }
    ComputeAuxData(finalHistory);
}

static void CollectMailboxRedInfo(int n,RedInfo ***infos,int **counts){
    *infos=malloc((size_t)n*sizeof(RedInfo*));
    *counts=malloc((size_t)n*sizeof(int));
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        int m=e->mailbox->tot;
        (*infos)[i]=malloc((size_t)(m>0?m:1)*sizeof(RedInfo));
        for(int j=0;j<m;j++){
            Observation *obs=e->mailbox->items[j];
            (*infos)[i][j].target=obs->finalLeafAtSend;
            (*infos)[i][j].mult=obs->multiplicity;
        }
        qsort((*infos)[i],(size_t)m,sizeof(RedInfo),cmp_redinfo);
        int u=0;
        for(int j=0;j<m;j++){
            if(u>0&&(*infos)[i][u-1].target==(*infos)[i][j].target)
                (*infos)[i][u-1].mult+=(*infos)[i][j].mult;
            else (*infos)[i][u++]=(*infos)[i][j];
        }
        (*counts)[i]=u;
    }
}

static void FreeMailboxRedInfo(RedInfo **infos,int *counts,int n){
    if(!infos)return;
    for(int i=0;i<n;i++)free(infos[i]);
    free(infos);
    free(counts);
}

static void ExecuteRoundIncremental(int r){
    int n=network->entities->tot;
    int R=network->rounds->tot;
    if(r==R-1)TakeSnapshotsBeforeRound();
    SetHistoryTreeMutationRound(r+1);
    HistoryTree **prevFL=malloc((size_t)n*sizeof(HistoryTree*));
    for(int i=0;i<n;i++)prevFL[i]=GetEntity(i)->finalLeaf;
    Vector *v=network->rounds->items[r];
    for(int i=0;i<v->tot;i++)ExecuteInteraction(v->items[i]);
    RedInfo **infos;
    int *counts;
    CollectMailboxRedInfo(n,&infos,&counts);
    for(int i=0;i<n;i++)EndRound(GetEntity(i));
    ExtendFinalHistoryOneLevel(prevFL,infos,counts);
    AppendAuxDataOneLevel();
    MaybeCaptureRoundCheckpoint(r+1);
    FreeMailboxRedInfo(infos,counts,n);
    free(prevFL);
}

static HistoryTree *FindMatchingFinalLeaf(HistoryTree *entityHist){
    if(!finalHistory||!entityHist)return NULL;
    HistoryTree *best=NULL;
    Vector *stack=NewVector(16);
    AddVector(stack,finalHistory);
    while(stack->tot){
        HistoryTree *node=DeleteVector(stack,stack->tot-1);
        if(HistoryTreeEquals(entityHist,node)){
            if(!best||node->level>best->level)best=node;
        }
        for(int i=0;i<node->children->tot;i++)
            AddVector(stack,node->children->items[i]);
    }
    FreeVector(stack);
    return best;
}

static void ReassignEntityFinalLeaves(void){
    int n=network->entities->tot;
    for(int i=0;i<n;i++)ComputeHashBottomUp(GetEntity(i)->history);
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        HistoryTree *leaf=NULL;
        unsigned long long h=e->history->hash;
        for(int j=0;j<i;j++){
            if(GetEntity(j)->history->hash==h&&HistoryTreeEquals(GetEntity(j)->history,e->history)){
                leaf=GetEntity(j)->finalLeaf;break;
            }
        }
        if(!leaf)leaf=FindMatchingFinalLeaf(e->history);
        e->finalLeaf=leaf;
    }
}

static void TrimSimulationViewsToPrefix(int prefixRounds){
    if(!finalHistory)return;
    TrimHistoryTreeToRound(finalHistory,prefixRounds);
    int target=prefixRounds+1;
    while(aux&&aux->tot>target)TrimAuxDataOneLevel();
}

static bool TryRestorePrefixViews(int prefixRounds){
    if(!finalHistory||!aux||prefixRounds<0)return false;
    TrimSimulationViewsToPrefix(prefixRounds);
    ReassignEntityFinalLeaves();
    for(int i=0;i<network->entities->tot;i++)
        if(!GetEntity(i)->finalLeaf)return false;
    return true;
}

static void ReplayRoundsFrom(int prefixRounds){
    /* Rebuild finalHistory/aux from entity prefix state instead of trimming stale
       deep aux (which diverged from full ExecuteNetwork on symmetric levels). */
    RebuildFinalHistory();
    int R=network->rounds->tot;
    for(int r=prefixRounds;r<R;r++)
        ExecuteRoundIncremental(r);
}

static void RebuildFinalHistory(void){
    double t0=PerfNowMs();
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=NewHistoryTree();
    int n=network->entities->tot;
    /* Compute Merkle hashes so identical trees can be detected in O(1). */
    for(int i=0;i<n;i++) ComputeHashBottomUp(GetEntity(i)->history);
    for(int i=0;i<n;i++){
        Entity *e=GetEntity(i);
        /* The Merkle hash (which excludes red edges) is a fast filter only, not a proof
           of isomorphism.  Two trees with the same black-edge structure but different
           reception histories can share a hash yet represent distinct equivalence classes.
           We therefore always follow a hash match with a full structural isomorphism check
           (HistoryTreeEquals) before reusing a finalLeaf. */
        HistoryTree *leaf=NULL;
        unsigned long long h=e->history->hash;
        for(int j=0;j<i;j++){
            if(GetEntity(j)->history->hash==h && HistoryTreeEquals(GetEntity(j)->history,e->history)){
                leaf=GetEntity(j)->finalLeaf;break;
            }
        }
        e->finalLeaf=leaf?leaf:MergeHistoryTrees(finalHistory,e->history,NULL);
    }
    ComputeAuxData(finalHistory);
    double elapsed=PerfNowMs()-t0;
    lastRebuildMs=elapsed; sumRebuildMs+=elapsed; rebuildSamples++;
}

/* Snapshot all entities' current states as "before the last round". */
static void TakeSnapshotsBeforeRound(void){
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        FreeEntitySnap(e);
        e->snap=malloc(sizeof(EntitySnapshot));
        e->snap->history=CopyHistoryTree(e->history,NULL);
        e->snap->current=FindCurrentInCopy(e->history,e->current,e->snap->history);
        e->snap->outdegree=e->outdegree;
    }
}

/* Restore all entities from their snapshots.  snap fields are consumed and set NULL. */
static void RestoreFromSnapshots(void){
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        FreeHistoryTree(e->history);
        ClearEntityMailbox(e);
        /* Take ownership of snap tree directly (no copy needed for RollBack) */
        e->history=e->snap->history;
        e->current=e->snap->current;
        e->outdegree=e->snap->outdegree;
        free(e->snap);e->snap=NULL;
    }
}

/* Restore all entities from their snapshots, keeping snap valid for future re-executions. */
static void RestoreFromSnapshotsCopy(void){
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        FreeHistoryTree(e->history);
        ClearEntityMailbox(e);
        HistoryTree *h=CopyHistoryTree(e->snap->history,NULL);
        e->current=FindCurrentInCopy(e->snap->history,e->snap->current,h);
        e->history=h;
        e->outdegree=e->snap->outdegree;
    }
}

static bool SnapshotsValid(void){
    if(!network || network->rounds->tot==0)return false;
    for(int i=0;i<network->entities->tot;i++)
        if(!GetEntity(i)->snap)return false;
    return true;
}

/* Re-execute only the last round using saved snapshots.
   Restores pre-last-round state (keeping snap valid), replays last round, rebuilds finalHistory.
   Falls back to full ExecuteNetwork() when snapshots are unavailable. */
void ReExecuteLastRound(void){
    if(!SnapshotsValid()){ExecuteNetwork();return;}
    RestoreFromSnapshotsCopy();
    ExecuteRoundIncremental(network->rounds->tot-1);
}

/* Roll back the last round: entity states revert to pre-last-round snapshots.
   Called after DeleteRound(last) when the last round was removed. */
void RollBackLastRound(void){
    if(!SnapshotsValid()){ExecuteNetwork();return;}
    RestoreFromSnapshots();
    if(!TryRestorePrefixViews(network->rounds->tot))
        RebuildFinalHistory();
}

/* Append the newly added last round on top of the current (already up-to-date) entity states.
   Called after InsertRound() appended a round at the end. */
void AppendLastRound(void){
    double totalStart=PerfNowMs();
    ExecuteRoundIncremental(network->rounds->tot-1);
    double afterAux=PerfNowMs();
    RecordAppendPerf(0.0,afterAux-totalStart,0.0,0.0,afterAux-totalStart);
}

void ExecuteNetwork(void){
    dirtyFromRound=-1;
    FreeRoundCheckpoints();
    roundCheckpoints=NewVector(8);
    SetHistoryTreeMutationRound(0);
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        if(e->history)FreeHistoryTree(e->history);
        FreeEntitySnap(e);
        e->outdegree=outAware?0:-1;
        e->current=e->history=NewHistoryTree();
        ExtendHistory(e);
    }
    int R=network->rounds->tot;
    for(int r=0;r<R;r++){
        if(r==R-1)TakeSnapshotsBeforeRound();
        SetHistoryTreeMutationRound(r+1);
        Vector *v=network->rounds->items[r];
        for(int i=0;i<v->tot;i++)
            ExecuteInteraction(v->items[i]);
        for(int i=0;i<network->entities->tot;i++)
            EndRound(GetEntity(i));
        MaybeCaptureRoundCheckpoint(r+1);
    }
    InitFinalHistoryFromEntities();
}

void ExecuteNetworkFromRound(int firstRound){
    dirtyFromRound=-1;
    RoundCheckpoint *cp;
    if(firstRound<=0){ExecuteNetwork();return;}
    cp=FindRoundCheckpoint(firstRound);
    /* Prefix-1 checkpoints are captured once at tutorial load and go stale after
       many tail appends; replay from them diverges from full ExecuteNetwork(). */
    if(!cp || cp->prefixRounds<=1){ExecuteNetwork();return;}
    if(!RestoreRoundCheckpoint(cp)){ExecuteNetwork();return;}
    TrimRoundCheckpointsAfter(firstRound);
    ReplayRoundsFrom(cp->prefixRounds);
}

void DoneNetwork(void){
    dirtyFromRound=-1;
    if(!network)return;
    FreeRoundCheckpoints();
    FreeAuxData();
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=NULL;
    for(int i=0;i<network->entities->tot;i++)
        FreeEntity(GetEntity(i));
    for(int r=network->rounds->tot-1;r>=0;r--)
        DeleteRound(r);
    FreeVector(network->entities);
    FreeVector(network->rounds);
    free(network);
    network=NULL;
}

void InsertRound(int index,bool copy){ // copy interactions from previous round?
    InsertVector(network->rounds,index,NewVector(16));
    if(!copy || !index)return;
    Vector *v1=network->rounds->items[index-1];
    Vector *v2=network->rounds->items[index];
    for(int i=0;i<v1->tot;i++){
        Interaction *interaction=v1->items[i];
        AddVector(v2,CopyInteraction(interaction));
    }
}

static void FreeInteraction(Interaction *interaction){
    free(interaction);
}

void DeleteInteractions(int index){
    Vector *v=network->rounds->items[index];
    for(int i=0;i<v->tot;i++)FreeInteraction(v->items[i]);
    while(v->tot)DeleteVector(v,0);
}

void DeleteRound(int index){
    Vector *v=DeinsertVector(network->rounds,index);
    for(int i=0;i<v->tot;i++)FreeInteraction(v->items[i]);
    FreeVector(v);
}

void AddEntity(int input,float x,float y){
    AddVector(network->entities,NewEntity(input,x,y));
    if(input==0)SortLeaders();
}

void DeleteEntity(int index){
    Entity *e=DeinsertVector(network->entities,index);
    for(int r=0;r<network->rounds->tot;r++){
        Vector *v=network->rounds->items[r];
        for(int i=0;i<v->tot;i++){
            Interaction *interaction=v->items[i];
            if(interaction->e1==e || interaction->e2==e){
                DeleteVector(v,i);
                FreeInteraction(interaction);
                i--;
            }
        }
    }
    FreeEntity(e);
}

static int GetInteraction(Vector *v,int i,int j){
    Entity *e1=GetEntity(i);
    Entity *e2=GetEntity(j);
    for(int k=0;k<v->tot;k++){
        Interaction *interaction=v->items[k];
        if(e1==interaction->e1 && e2==interaction->e2)return k;
    }
    return -1;
}

void AddInteraction(int round,int i,int j,int multiplicity){ // interaction from entity i to entity j
    Vector *v=network->rounds->items[round];
    int k=GetInteraction(v,i,j);
    if(k!=-1){
        Interaction *interaction=v->items[k];
        interaction->multiplicity+=multiplicity;
        if(interaction->multiplicity<=0){
            DeleteVector(v,k);
            FreeInteraction(interaction);
        }
    }
    else if(multiplicity>0)AddVector(v,NewInteraction(i,j,multiplicity));
}

void AddDoubleInteraction(int round,int i,int j,int multiplicity){ // interaction between entity i and entity j
    AddInteraction(round,i,j,multiplicity);
    AddInteraction(round,j,i,multiplicity);
}

int SelectEntityXY(int x,int y){
    float minDist=-1.0f;
    int s=-1;
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        float cx=ToScreenX1(win1,e->x);
        float cy=ToScreenY(win1,e->y);
        float dist=(cx-x)*(cx-x)+(cy-y)*(cy-y);
        if(s==-1 || dist<minDist){ minDist=dist; s=i; }
    }
    if(s!=-1 && minDist<=NODE_SIZE*NODE_SIZE*0.25f)return s;
    else return -1;
}

void CountingAlgorithm(void){
    if(IsNetworkDirty())return;
    SelectView();
    ResetAuxDataVariables();
    if(selectedNode && selectedNode->h->level>=0)
        switch(algorithm){
            case 1: StabilizingAlgorithm(); break;
            case 2: TerminatingAlgorithm(); break;
            default: break;
        }
}

static bool LoadNetworkHelper(SDL_IOStream *stream){
    #define BUF_SIZE 1024
    char buffer[BUF_SIZE];
    int input,round=-1,r,e1,e2,m,n=0;
    float x,y;
    size_t length=0,readBytes=0;
    char ch;
    Network *backup=network;
    network=malloc(sizeof(Network));
    network->entities=NewVector(8);
    network->rounds=NewVector(8);
    for(;;){
        readBytes=SDL_ReadIO(stream,&ch,1);
        if(!readBytes)break;
        if(length<BUF_SIZE-1)buffer[length++]=ch;
        if(ch=='\n'||length>=BUF_SIZE-1){
            buffer[length]='\0';
            if(sscanf(buffer,"entity (%d, %f, %f)",&input,&x,&y)==3){
                if(x<-1.0f)x=-1.0f;
                if(x>1.0f)x=1.0f;
                if(y<-1.0f)y=-1.0f;
                if(y>1.0f)y=1.0f;
                if(input<0)input=0;
                AddEntity(input,x,y);
                n++;
            }
            else if(sscanf(buffer,"round %d",&r)==1)InsertRound(++round,false);
            else if(sscanf(buffer,"inter (%d, %d, %d)",&e1,&e2,&m)==3){
                if(0<=round && round<network->rounds->tot && 0<=e1 && e1<n && 0<=e2 && e2<n && m>0)
                    AddInteraction(round,e1,e2,m);
            }
            length=0;
        }
    }
    if(length>0){
        buffer[length]='\0';
        if(sscanf(buffer,"entity (%d, %f, %f)",&input,&x,&y)==3){
            if(x<-1.0f)x=-1.0f;
            if(x>1.0f)x=1.0f;
            if(y<-1.0f)y=-1.0f;
            if(y>1.0f)y=1.0f;
            if(input<0)input=0;
            AddEntity(input,x,y);
            n++;
        }
        else if(sscanf(buffer,"round %d",&r)==1)InsertRound(++round,false);
        else if(sscanf(buffer,"inter (%d, %d, %d)",&e1,&e2,&m)==3){
            if(0<=round && round<network->rounds->tot && 0<=e1 && e1<n && 0<=e2 && e2<n && m>0)
                AddInteraction(round,e1,e2,m);
        }
    }
    if(n==0){
        for(int i=0;i<network->entities->tot;i++)FreeEntity(GetEntity(i));
        for(int j=network->rounds->tot-1;j>=0;j--)DeleteRound(j);
        FreeVector(network->entities);
        FreeVector(network->rounds);
        free(network);
        network=backup;
        return false;
    }
    selectedEntity=selectedNodeI=selectedNodeJ=-1;
    selectedNode=NULL;
    currentRound=round==-1?-1:0;
    Network *temp=network;
    network=backup;
    DoneNetwork();
    network=temp;
    ExecuteNetwork();
    win1->invalid=true;
    return true;
}

static bool SaveNetworkHelper(SDL_IOStream *stream){
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        if(!SDL_IOprintf(stream,"entity (%d, %f, %f)\n",e->input,e->x,e->y))return false;
    }
    for(int i=0;i<network->rounds->tot;i++){
        if(!SDL_IOprintf(stream,"\nround %d\n",i+1))return false;
        Vector *v=network->rounds->items[i];
        for(int j=0;j<v->tot;j++){
            Interaction *interaction=v->items[j];
            int e1=GetEntityIndex(interaction->e1);
            int e2=GetEntityIndex(interaction->e2);
            if(e1==-1 || e2==-1)continue;
            if(!SDL_IOprintf(stream,"inter (%d, %d, %d)\n",e1,e2,interaction->multiplicity))return false;
        }
    }
    return true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
void HandleLoadedFile(const char *data,int length){
    SDL_IOStream *stream=NULL;
    if(!(stream=SDL_IOFromMem((void*)data,length))){
        free((void*)data);
        DisplayMessage("Failed to load network");
        return;
    }
    if(LoadNetworkHelper(stream) && SDL_CloseIO(stream))DisplayMessage("Network loaded");
    else DisplayMessage("Failed to load network");
    free((void*)data);
}

EM_JS(void,LoadFileHelper,(void),{
    const input=document.createElement('input');
    input.type='file';
    input.accept='.txt,*/*';
    input.style.position='fixed';
    input.style.width='1px';
    input.style.height='1px';
    input.style.opacity='0';
    input.style.left='0';
    input.style.top='0';
    input.onchange=(event)=>{
        const file=event.target.files && event.target.files[0];
        input.remove();
        if(!file)return;
        const reader=new FileReader();
        reader.onload=(e)=>{
            const bytes=new Uint8Array(e.target.result);
            const ptr=_malloc(bytes.length);
            HEAPU8.set(bytes,ptr);
            _HandleLoadedFile(ptr,bytes.length);
        };
        reader.readAsArrayBuffer(file);
    };
    document.body.appendChild(input);
    input.click();
});

EM_JS(void,SaveFileHelper,(const char *filename,const unsigned char *data,int length),{
    try{
        const name=UTF8ToString(filename);
        const bytes=HEAPU8.slice(data,data+length);
        const blob=new Blob([bytes],{type:'application/octet-stream'});
        const url=URL.createObjectURL(blob);
        const a=document.createElement('a');
        a.href=url;
        a.download=name;
        a.style.position='fixed';
        a.style.width='1px';
        a.style.height='1px';
        a.style.opacity='0';
        a.style.left='0';
        a.style.top='0';
        document.body.appendChild(a);
        a.click();
        setTimeout(()=>a.remove(),0);
        setTimeout(URL.revokeObjectURL,60000,url);
    }
    catch(e){console.error('[SaveFileHelper] failed:',e);}
});

void LoadNetwork(void){
    resizing=drawingEdge=draggingEntity=false;
    DisplayMessage("Loading network");
    LoadFileHelper();
}

void SaveNetwork(void){
    resizing=drawingEdge=draggingEntity=false;
    Uint8 **data;
    SDL_IOStream *stream=NewDynamicBufferStream(&data);
    if(!stream){
        DisplayMessage("Failed to save network");
        return;
    }
    if(!SaveNetworkHelper(stream)){
        SDL_CloseIO(stream);
        DisplayMessage("Failed to save network");
        return;
    }
    SaveFileHelper(FILENAME,*data,SDL_TellIO(stream));
    if(SDL_CloseIO(stream))DisplayMessage("Saving network");
    else DisplayMessage("Failed to save network");
}
#else
static const SDL_DialogFileFilter filters[]={
    {"TXT files","txt"},
    {"All files","*"}
};

static void SDLCALL LoadFileCallback(void *userdata,const char *const *filelist,int filter){
    (void)userdata; (void)filter;
    if(!filelist || !*filelist)return;
    SDL_Event event;
    SDL_zero(event);
    event.type=SDL_EVENT_LOAD_NETWORK;
    event.user.data1=SDL_strdup(*filelist);
    SDL_PushEvent(&event);
}

static char *EnsureSuffix(const char *string,const char *suffix){
    if(!string)return NULL;
    if(!suffix)return SDL_strdup(string);
    size_t stringLen=SDL_strlen(string);
    size_t suffixLen=SDL_strlen(suffix);
    if(stringLen>=suffixLen && !SDL_strcasecmp(string+stringLen-suffixLen,suffix))return SDL_strdup(string);
    size_t newStringLen=stringLen+suffixLen+1;
    char *newString=SDL_malloc(newStringLen);
    if(!newString)return NULL;
    SDL_strlcpy(newString,string,newStringLen);
    SDL_strlcat(newString,suffix,newStringLen);
    return newString;
}

static void SDLCALL SaveFileCallback(void *userdata,const char *const *filelist,int filter){
    (void)userdata; (void)filter;
    if(!filelist || !*filelist)return;
    SDL_Event event;
    SDL_zero(event);
    event.type=SDL_EVENT_SAVE_NETWORK;
    event.user.data1=EnsureSuffix(*filelist,".txt");
    SDL_PushEvent(&event);
}

void LoadNetwork(void){
    SDL_ShowOpenFileDialog(LoadFileCallback,NULL,win1->window,filters,2,SDL_GetBasePath(),false);
}

void SaveNetwork(void){
    SDL_ShowSaveFileDialog(SaveFileCallback,NULL,win1->window,filters,2,SDL_GetBasePath());
}

void LoadNetworkRun(const char *filename){
    resizing=drawingEdge=draggingEntity=false;
    SDL_IOStream *stream=NULL;
    if(!(stream=SDL_IOFromFile(filename,"rb"))){
        DisplayMessage("Failed to load network");
        SDL_free((char*)filename);
        return;
    }
    if(LoadNetworkHelper(stream) && SDL_CloseIO(stream))DisplayMessage("Network loaded");
    else DisplayMessage("Failed to load network");
    SDL_free((char*)filename);
}

void SaveNetworkRun(const char *filename){
    resizing=drawingEdge=draggingEntity=false;
    SDL_IOStream *stream=NULL;
    if(!(stream=SDL_IOFromFile(filename,"wb"))){
        DisplayMessage("Failed to save network");
        SDL_free((char*)filename);
        return;
    }
    if(SaveNetworkHelper(stream) && SDL_CloseIO(stream))DisplayMessage("Network saved");
    else DisplayMessage("Failed to save network");
    SDL_free((char*)filename);
}
#endif

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE int GetNumAgents(void){
    return network ? network->entities->tot : 0;
}

EMSCRIPTEN_KEEPALIVE int GetNumLeaders(void){
    if(!network)return 0;
    int l=0;
    for(int i=0;i<network->entities->tot;i++)
        if(GetEntity(i)->input==0)l++;
    return l;
}

EMSCRIPTEN_KEEPALIVE int GetNumRounds(void){
    return network ? network->rounds->tot : 0;
}

EMSCRIPTEN_KEEPALIVE void ResetAppendPerfMetrics(void){
    ResetAppendPerfStats();
}

EMSCRIPTEN_KEEPALIVE double GetLastAppendSnapshotMs(void){
    return appendPerfStats.lastSnapshotMs;
}

EMSCRIPTEN_KEEPALIVE double GetLastAppendExecuteMs(void){
    return appendPerfStats.lastExecuteMs;
}

EMSCRIPTEN_KEEPALIVE double GetLastAppendFinalHistoryMs(void){
    return appendPerfStats.lastFinalHistoryMs;
}

EMSCRIPTEN_KEEPALIVE double GetLastAppendAuxMs(void){
    return appendPerfStats.lastAuxMs;
}

EMSCRIPTEN_KEEPALIVE double GetLastAppendTotalMs(void){
    return appendPerfStats.lastTotalMs;
}

EMSCRIPTEN_KEEPALIVE double GetSumAppendSnapshotMs(void){
    return appendPerfStats.sumSnapshotMs;
}

EMSCRIPTEN_KEEPALIVE double GetSumAppendExecuteMs(void){
    return appendPerfStats.sumExecuteMs;
}

EMSCRIPTEN_KEEPALIVE double GetSumAppendFinalHistoryMs(void){
    return appendPerfStats.sumFinalHistoryMs;
}

EMSCRIPTEN_KEEPALIVE double GetSumAppendAuxMs(void){
    return appendPerfStats.sumAuxMs;
}

EMSCRIPTEN_KEEPALIVE double GetSumAppendTotalMs(void){
    return appendPerfStats.sumTotalMs;
}

EMSCRIPTEN_KEEPALIVE int GetAppendPerfSamples(void){
    return appendPerfStats.samples;
}

EMSCRIPTEN_KEEPALIVE void ResetRebuildPerfMetrics(void){
    lastRebuildMs=0.0; sumRebuildMs=0.0; rebuildSamples=0;
}
EMSCRIPTEN_KEEPALIVE double GetLastRebuildMs(void){ return lastRebuildMs; }
EMSCRIPTEN_KEEPALIVE double GetSumRebuildMs(void)  { return sumRebuildMs;  }
EMSCRIPTEN_KEEPALIVE int    GetRebuildSamples(void){ return rebuildSamples; }

EMSCRIPTEN_KEEPALIVE int GetCurrentRound(void){
    return currentRound; // 0-based; -1 if no rounds
}

EMSCRIPTEN_KEEPALIVE int GetCurrentRoundLinks(void){
    if(!network || currentRound<0 || currentRound>=network->rounds->tot)return 0;
    return ((Vector*)network->rounds->items[currentRound])->tot;
}

EMSCRIPTEN_KEEPALIVE int GetNumAnonymityClasses(void){
    if(!aux || aux->tot==0)return 0;
    return GetLevel(aux->tot-1)->tot;
}

EMSCRIPTEN_KEEPALIVE int GetNumUniqueAgents(void){
    if(!aux || aux->tot==0)return 0;
    Vector *lastLevel=GetLevel(aux->tot-1);
    int count=0;
    for(int j=0;j<lastLevel->tot;j++){
        AuxData *data=lastLevel->items[j];
        if(data->anonymity==1)count++;
    }
    return count;
}

// Tutorial network: 6 agents (1 leader + 5 anonymous), 5 dynamic rounds
// Arranged as a hexagonal ring; rounds 1-4 break symmetry so the
// stabilizing algorithm converges to n = 6.
EMSCRIPTEN_KEEPALIVE void TutorialLoadNetwork(void){
    drawingEdge=draggingEntity=false;
    selectedEntity=selectedNodeI=selectedNodeJ=-1;
    selectedNode=NULL;
    currentRound=-1;
    numSteps=-1;
    DoneNetwork();
    network=malloc(sizeof(Network));
    network->entities=NewVector(8);
    network->rounds=NewVector(8);
    /* 6 agents arranged in a hexagon.
     * Agent 0 = leader (input 0), agents 1-5 anonymous (input 1).
     *
     * Round 0 is a RING: L-1-2-3-4-5-L.
     * This creates two symmetric classes:
     *   {1,5}   - both see L + one anonymous agent (yellow after step 1)
     *   {2,3,4} - all see two anonymous agents    (yellow after step 1)
     * Rounds 1-4 break this symmetry so the algorithm converges to n=6. */
    AddEntity(0, -0.65f,  0.0f);    /* 0: leader L */
    AddEntity(1, -0.325f, 0.563f);  /* 1 */
    AddEntity(1,  0.325f, 0.563f);  /* 2 */
    AddEntity(1,  0.65f,  0.0f);    /* 3 */
    AddEntity(1,  0.325f,-0.563f);  /* 4 */
    AddEntity(1, -0.325f,-0.563f);  /* 5 */
    /* Round 0: ring L-1-2-3-4-5-L */
    InsertRound(0,false);
    AddDoubleInteraction(0,0,1,1);
    AddDoubleInteraction(0,1,2,1);
    AddDoubleInteraction(0,2,3,1);
    AddDoubleInteraction(0,3,4,1);
    AddDoubleInteraction(0,4,5,1);
    AddDoubleInteraction(0,5,0,1);
    /* Round 1: L connects to 1 and 3 (breaks 1-5 symmetry) */
    InsertRound(1,false);
    AddDoubleInteraction(1,0,1,1);
    AddDoubleInteraction(1,0,3,1);
    AddDoubleInteraction(1,1,5,1);
    AddDoubleInteraction(1,2,5,1);
    AddDoubleInteraction(1,3,4,1);
    /* Round 2: L connects to 2 and 4 */
    InsertRound(2,false);
    AddDoubleInteraction(2,0,2,1);
    AddDoubleInteraction(2,0,4,1);
    AddDoubleInteraction(2,1,3,1);
    AddDoubleInteraction(2,2,5,1);
    AddDoubleInteraction(2,3,5,1);
    /* Round 3: L connects to 3 and 5 */
    InsertRound(3,false);
    AddDoubleInteraction(3,0,3,1);
    AddDoubleInteraction(3,0,5,1);
    AddDoubleInteraction(3,1,2,1);
    AddDoubleInteraction(3,2,4,1);
    AddDoubleInteraction(3,3,4,1);
    /* Round 4: L connects to 1 and 2; additional cross-links */
    InsertRound(4,false);
    AddDoubleInteraction(4,0,1,1);
    AddDoubleInteraction(4,0,2,1);
    AddDoubleInteraction(4,1,4,1);
    AddDoubleInteraction(4,3,5,1);
    AddDoubleInteraction(4,4,5,1);
    currentRound=0;
    ExecuteNetwork();
    win1->invalid=true;
}

EMSCRIPTEN_KEEPALIVE int GetRootGuess(void){
    if(!aux || aux->tot==0)return -1;
    return GetAuxData(0,0)->guess;
}

EMSCRIPTEN_KEEPALIVE int GetNumGuessedNodes(void){
    if(!aux || aux->tot==0)return 0;
    int count=0;
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++)
            if(((AuxData*)v->items[j])->guess!=-1)count++;
    }
    return count;
}

EMSCRIPTEN_KEEPALIVE int GetSelectedEntity(void){
    return selectedEntity;
}

EMSCRIPTEN_KEEPALIVE int GetSelectedNodeI(void){
    return selectedNodeI;
}

EMSCRIPTEN_KEEPALIVE int GetSelectedNodeJ(void){
    return selectedNodeJ;
}

EMSCRIPTEN_KEEPALIVE void FlushDeferred(void){
    EnsureNetworkComputed();
}

static unsigned SpecHashMix(unsigned h,unsigned v){
    h^=v;
    h*=16777619u;
    return h;
}

EMSCRIPTEN_KEEPALIVE unsigned GetNetworkSpecFingerprint(void){
    unsigned h=2166136261u;
    if(!network)return h;
    h=SpecHashMix(h,(unsigned)network->entities->tot);
    h=SpecHashMix(h,(unsigned)network->rounds->tot);
    h=SpecHashMix(h,(unsigned)currentRound);
    for(int r=0;r<network->rounds->tot;r++){
        Vector *v=network->rounds->items[r];
        h=SpecHashMix(h,(unsigned)v->tot);
        for(int j=0;j<v->tot;j++){
            Interaction *in=v->items[j];
            h=SpecHashMix(h,(unsigned)GetEntityIndex(in->e1));
            h=SpecHashMix(h,(unsigned)GetEntityIndex(in->e2));
            h=SpecHashMix(h,(unsigned)in->multiplicity);
        }
    }
    if(aux && aux->tot>0){
        h=SpecHashMix(h,(unsigned)aux->tot);
        for(int i=0;i<aux->tot;i++){
            Vector *lv=GetLevel(i);
            h=SpecHashMix(h,(unsigned)lv->tot);
            for(int j=0;j<lv->tot;j++){
                AuxData *d=lv->items[j];
                h=SpecHashMix(h,(unsigned)d->anonymity);
                h=SpecHashMix(h,(unsigned)d->guess);
            }
        }
    }
    return h;
}

EMSCRIPTEN_KEEPALIVE unsigned GetNetworkSemanticFingerprint(void){
    unsigned h=2166136261u;
    if(!network)return h;
    h=SpecHashMix(h,(unsigned)network->entities->tot);
    h=SpecHashMix(h,(unsigned)network->rounds->tot);
    h=SpecHashMix(h,(unsigned)currentRound);
    for(int r=0;r<network->rounds->tot;r++){
        Vector *v=network->rounds->tot>0?network->rounds->items[r]:NULL;
        if(!v)continue;
        h=SpecHashMix(h,(unsigned)v->tot);
        for(int j=0;j<v->tot;j++){
            Interaction *in=v->items[j];
            h=SpecHashMix(h,(unsigned)GetEntityIndex(in->e1));
            h=SpecHashMix(h,(unsigned)GetEntityIndex(in->e2));
            h=SpecHashMix(h,(unsigned)in->multiplicity);
        }
    }
    if(aux && aux->tot>0){
        h=SpecHashMix(h,(unsigned)aux->tot);
        for(int i=0;i<aux->tot;i++){
            Vector *lv=GetLevel(i);
            h=SpecHashMix(h,(unsigned)lv->tot);
            for(int j=0;j<lv->tot;j++){
                AuxData *d=lv->items[j];
                h=SpecHashMix(h,(unsigned)d->anonymity);
                h=SpecHashMix(h,(unsigned)d->guess);
            }
        }
    }
    return h;
}

EMSCRIPTEN_KEEPALIVE void TestGotoRound(int r){
    if(!network || r<-1 || r>=network->rounds->tot)return;
    currentRound=r;
    numSteps=-1;
    CountingAlgorithm();
}

EMSCRIPTEN_KEEPALIVE int GetAuxLevelCount(void){
    return aux ? aux->tot : 0;
}

EMSCRIPTEN_KEEPALIVE int GetAuxLevelWidth(int level){
    if(!aux || level<0 || level>=aux->tot)return 0;
    return GetLevel(level)->tot;
}

EMSCRIPTEN_KEEPALIVE int GetAuxCell(int level,int j){
    if(!aux || level<0 || level>=aux->tot)return -1;
    Vector *v=GetLevel(level);
    if(j<0 || j>=v->tot)return -1;
    AuxData *d=v->items[j];
    return d->anonymity*100000+(d->guess+1);
}
#endif

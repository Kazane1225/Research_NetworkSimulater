#include "main.h"
#ifdef __EMSCRIPTEN__
#include <SDL3/SDL.h>
#endif

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

static double PerfNowMs(void){
    static double freq=0.0;
#ifdef __EMSCRIPTEN__
    if(freq==0.0)freq=(double)SDL_GetPerformanceFrequency();
    return 1000.0*(double)SDL_GetPerformanceCounter()/freq;
#else
    return 0.0;
#endif
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

/* ── simulation ───────────────────────────────────────────────────────────── */

/* finalHistory is built one level per round without materialising entity views: an entity's
   node at level r+1 is the child of its level-r node identified by its input, its outdegree in
   round r and the multiset of level-r nodes it received messages from. Children are created in
   entity order, so the tree depends only on the network (not on the edits that produced it),
   and the leader's node, entity 0 after SortLeaders, is the first child at every level. */

typedef struct{HistoryTree *target;int mult;}RedInfo;
static int cmp_redinfo(const void *a,const void *b){
    uintptr_t pa=(uintptr_t)((const RedInfo *)a)->target;
    uintptr_t pb=(uintptr_t)((const RedInfo *)b)->target;
    return (pa>pb)-(pa<pb);
}

static void AssignEntityIndices(void){
    for(int i=0;i<network->entities->tot;i++)GetEntity(i)->index=i;
}

/* infos must be sorted by target and free of duplicate targets, like a node's observations. */
static HistoryTree *FindOrAddChild(HistoryTree *par,int input,int outdegree,const RedInfo *infos,int nc){
    for(int j=0;j<par->children->tot;j++){
        HistoryTree *z=par->children->items[j];
        if(z->input!=input||z->outdegree!=outdegree||z->observations->tot!=nc)continue;
        bool ok=true;
        for(int k=0;k<nc&&ok;k++){
            Observation *o=z->observations->items[k];
            if(o->history!=infos[k].target||o->multiplicity!=infos[k].mult)ok=false;
        }
        if(ok)return z;
    }
    HistoryTree *node=AddHistoryTreeChild(par,input);
    node->outdegree=outdegree;
    for(int k=0;k<nc;k++)AddRedEdge(node,infos[k].target,infos[k].mult);
    return node;
}

static void SimulateLevel0(HistoryTree *root,HistoryTree **leaves){
    int outdegree=outAware?0:-1;
    for(int i=0;i<network->entities->tot;i++)
        leaves[i]=FindOrAddChild(root,GetEntity(i)->input,outdegree,NULL,0);
}

/* Advances leaves[i] (the node of entity i) by round r. Entity indices must be current. */
static void SimulateRound(int r,HistoryTree **leaves){
    int n=network->entities->tot;
    Vector *v=network->rounds->items[r];
    int *start=calloc((size_t)n+1,sizeof(int));
    int *fill=calloc((size_t)n+1,sizeof(int));
    int *outdegree=malloc(((size_t)n+1)*sizeof(int));
    RedInfo *infos=malloc((size_t)(v->tot>0?v->tot:1)*sizeof(RedInfo));
    for(int i=0;i<n;i++)outdegree[i]=outAware?0:-1;
    for(int k=0;k<v->tot;k++){
        Interaction *interaction=v->items[k];
        start[interaction->e2->index+1]++;
    }
    for(int i=0;i<n;i++)start[i+1]+=start[i];
    for(int k=0;k<v->tot;k++){
        Interaction *interaction=v->items[k];
        int s=interaction->e1->index,d=interaction->e2->index;
        infos[start[d]+fill[d]++]=(RedInfo){leaves[s],interaction->multiplicity};
        if(outAware)outdegree[s]+=interaction->multiplicity;
    }
    for(int i=0;i<n;i++){
        RedInfo *seg=infos+start[i];
        int m=start[i+1]-start[i];
        qsort(seg,(size_t)m,sizeof(RedInfo),cmp_redinfo);
        int u=0;
        for(int j=0;j<m;j++){
            if(u>0&&seg[u-1].target==seg[j].target)seg[u-1].mult+=seg[j].mult;
            else seg[u++]=seg[j];
        }
        leaves[i]=FindOrAddChild(leaves[i],GetEntity(i)->input,outdegree[i],seg,u);
    }
    free(infos);
    free(outdegree);
    free(fill);
    free(start);
}

static HistoryTree **simLeaves=NULL;
static int simN=0;
static int simNext=0;
static int simEnd=0;

static void FreeSimState(void){
    free(simLeaves);
    simLeaves=NULL;
    simN=0;
    simNext=0;
    simEnd=0;
}

static void CommitSimLeaves(void){
    for(int i=0;i<simN;i++)GetEntity(i)->finalLeaf=simLeaves[i];
}

/* Extends finalHistory and aux by round r, which must be the round right after the deepest level. */
static void ExtendFinalHistoryByRound(int r){
    int n=network->entities->tot;
    HistoryTree **leaves=malloc(((size_t)n+1)*sizeof(HistoryTree*));
    AssignEntityIndices();
    for(int i=0;i<n;i++)leaves[i]=GetEntity(i)->finalLeaf;
    SimulateRound(r,leaves);
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=leaves[i];
    free(leaves);
    AppendAuxDataOneLevel();
}

/* aux has one level for the root plus one per finalHistory level (rounds+1 of them). */
static bool FinalHistoryHasLevels(int auxLevels){
    if(!finalHistory||!aux||aux->tot!=auxLevels)return false;
    for(int i=0;i<network->entities->tot;i++)
        if(!GetEntity(i)->finalLeaf)return false;
    return true;
}

static bool CanTrimToLevel(int keepLevel){
    int keepAux=keepLevel+2;
    if(!finalHistory||!aux||aux->tot<keepAux)return false;
    for(int i=0;i<network->entities->tot;i++){
        HistoryTree *h=GetEntity(i)->finalLeaf;
        if(!h)return false;
        while(h&&h->level>keepLevel)h=h->parent;
        if(!h||h->level!=keepLevel)return false;
    }
    return true;
}

/* Removes the deepest level of finalHistory and of aux; entities move back to their parents. */
static void TrimFinalHistoryLastLevel(void){
    int n=network->entities->tot;
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=GetEntity(i)->finalLeaf->parent;
    Vector *deepest=GetLevel(aux->tot-1);
    int m=deepest->tot;
    HistoryTree **nodes=malloc((size_t)(m>0?m:1)*sizeof(HistoryTree*));
    for(int j=0;j<m;j++)nodes[j]=((AuxData*)deepest->items[j])->h;
    TrimAuxDataOneLevel();
    for(int j=0;j<m;j++){
        HistoryTree *node=nodes[j];
        HistoryTree *par=node->parent;
        if(par){
            for(int i=0;i<par->children->tot;i++){
                if(par->children->items[i]==node){
                    DeinsertVector(par->children,i);
                    break;
                }
            }
        }
        FreeHistoryTree(node);
    }
    free(nodes);
}

static void TrimFinalHistoryToLevel(int keepLevel){
    int keepAux=keepLevel+2;
    while(aux&&aux->tot>keepAux)TrimFinalHistoryLastLevel();
}

/* Re-execute only the last round after its interactions changed. */
void ReExecuteLastRound(void){
    int R=network->rounds->tot;
    if(R==0||!FinalHistoryHasLevels(R+2)){ExecuteNetwork();return;}
    TrimFinalHistoryLastLevel();
    ExtendFinalHistoryByRound(R-1);
}

/* Drop the deepest level after DeleteRound(last) removed the last round. */
void RollBackLastRound(void){
    if(!FinalHistoryHasLevels(network->rounds->tot+3)){ExecuteNetwork();return;}
    TrimFinalHistoryLastLevel();
}

/* Add one level for the round InsertRound() just appended at the end. */
void AppendLastRound(void){
    double totalStart=PerfNowMs();
    int R=network->rounds->tot;
    if(R==0||!FinalHistoryHasLevels(R+1))ExecuteNetwork();
    else ExtendFinalHistoryByRound(R-1);
    double elapsed=PerfNowMs()-totalStart;
    RecordAppendPerf(0.0,elapsed,0.0,0.0,elapsed);
}

/* firstRound<0 rebuilds from an empty tree. firstRound>=0 keeps levels up to firstRound
   (level 0 is the input layer; round r produces level r+1) and replays [firstRound, R). */
void ExecuteNetworkReplayBegin(int firstRound){
    FreeSimState();
    int n=network->entities->tot;
    simN=n;
    simLeaves=malloc(((size_t)n+1)*sizeof(HistoryTree*));
    AssignEntityIndices();
    if(firstRound>=0&&CanTrimToLevel(firstRound)){
        TrimFinalHistoryToLevel(firstRound);
        for(int i=0;i<n;i++)simLeaves[i]=GetEntity(i)->finalLeaf;
        simNext=firstRound;
        simEnd=network->rounds->tot;
        return;
    }
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=NewHistoryTree();
    SimulateLevel0(finalHistory,simLeaves);
    simNext=0;
    simEnd=network->rounds->tot;
}

int ExecuteNetworkReplayStep(void){
    if(simNext>=simEnd)return 1;
    SimulateRound(simNext,simLeaves);
    simNext++;
    return simNext>=simEnd;
}

void ExecuteNetworkReplayFinish(void){
    CommitSimLeaves();
    FreeSimState();
    ComputeAuxData(finalHistory);
}

static void RunReplay(int firstRound){
    double t0=PerfNowMs();
    ExecuteNetworkReplayBegin(firstRound);
    while(!ExecuteNetworkReplayStep());
    ExecuteNetworkReplayFinish();
    double elapsed=PerfNowMs()-t0;
    lastRebuildMs=elapsed; sumRebuildMs+=elapsed; rebuildSamples++;
}

void ExecuteNetworkFromRound(int firstRound){
    ComputeJob_CancelActive();
    RunReplay(firstRound<0?0:firstRound);
}

void RecordRebuildPerf(double elapsedMs){
    lastRebuildMs=elapsedMs; sumRebuildMs+=elapsedMs; rebuildSamples++;
}

void ExecuteNetwork(void){
    ComputeJob_CancelActive();
    RunReplay(-1);
}

void DoneNetwork(void){
    ComputeJob_CancelActive();
    FreeSimState();
    FreeAuxData();
    if(finalHistory)FreeHistoryTree(finalHistory);
    finalHistory=NULL;
    if(!network)return;
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
    // Cancel first: the global `network` pointer below is about to be swapped to a brand
    // new (empty, then progressively filled) object, which a paused job must never see.
    ComputeJob_CancelActive();
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

EMSCRIPTEN_KEEPALIVE void BenchLoadNetworkText(const char *text){
    if(!text || !*text)return;
    DoneNetwork();
    size_t len=SDL_strlen(text);
    char *copy=SDL_malloc(len);
    if(!copy)return;
    SDL_memcpy(copy,text,len);
    HandleLoadedFile(copy,(int)len);
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

/* Aux level aligned with the history-tree row highlighted for currentRound (see render.c). */
static int AuxLevelForCurrentRound(void){
    if(!aux || aux->tot==0 || currentRound<0)return -1;
    int level=currentRound+2;
    if(level<0 || level>=aux->tot)return -1;
    return level;
}

EMSCRIPTEN_KEEPALIVE int GetNumAnonymityClasses(void){
    int level=AuxLevelForCurrentRound();
    if(level<0)return 0;
    return GetLevel(level)->tot;
}

EMSCRIPTEN_KEEPALIVE int GetNumUniqueAgents(void){
    int level=AuxLevelForCurrentRound();
    if(level<0)return 0;
    Vector *v=GetLevel(level);
    int count=0;
    for(int j=0;j<v->tot;j++){
        AuxData *data=v->items[j];
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

EMSCRIPTEN_KEEPALIVE int GetAuxCellVisible(int level,int j){
    if(!aux || level<0 || level>=aux->tot)return -1;
    Vector *v=GetLevel(level);
    if(j<0 || j>=v->tot)return -1;
    return ((AuxData*)v->items[j])->visible?1:0;
}

EMSCRIPTEN_KEEPALIVE unsigned GetVistaVisibleFingerprint(void){
    unsigned h=2166136261u;
    if(!aux)return h;
    for(int i=0;i<aux->tot;i++){
        Vector *lv=GetLevel(i);
        h=SpecHashMix(h,(unsigned)lv->tot);
        for(int j=0;j<lv->tot;j++){
            AuxData *d=lv->items[j];
            h=SpecHashMix(h,d->visible?1u:0u);
            h=SpecHashMix(h,(unsigned)d->anonymity);
            h=SpecHashMix(h,(unsigned)(d->guess+1));
        }
    }
    return h;
}

EMSCRIPTEN_KEEPALIVE int GetEntityFinalLeafCell(int agent){
    if(!network || agent<0 || agent>=network->entities->tot)return -1;
    Entity *e=GetEntity(agent);
    if(!e||!e->finalLeaf||!e->finalLeaf->data)return -1;
    AuxData *d=e->finalLeaf->data;
    return d->i*100000+d->j;
}

EMSCRIPTEN_KEEPALIVE int GetAuxLevelUniqueCount(int level){
    if(!aux || level<0 || level>=aux->tot)return 0;
    Vector *v=GetLevel(level);
    int count=0;
    for(int j=0;j<v->tot;j++)
        if(((AuxData*)v->items[j])->anonymity==1)count++;
    return count;
}

EMSCRIPTEN_KEEPALIVE int GetAuxLevelAnonymitySum(int level){
    if(!aux || level<0 || level>=aux->tot)return 0;
    Vector *v=GetLevel(level);
    int sum=0;
    for(int j=0;j<v->tot;j++)sum+=((AuxData*)v->items[j])->anonymity;
    return sum;
}
#endif

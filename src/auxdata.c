#include "main.h"

Vector *aux=NULL; // Vector of Vector of AuxData
AuxData *selectedNode=NULL;

Vector *GetLevel(int i){
    return aux->items[i];
}

AuxData *GetAuxData(int i,int j){
    return GetLevel(i)->items[j];
}

static int ComputeAuxDataWidth(HistoryTree *root){
    /* Phase 1: iterative pre-order DFS — allocate and register an AuxData node
       for every HistoryTree node.  Children are pushed right-to-left so left
       subtrees are visited first, which preserves the same level-index assignment
       order as the old recursive version.
       Invariant: when a node is popped, GetLevel(index-1)->tot-1 is that node's
       parent index — identical to what the recursive entry condition guaranteed. */
    Vector *stk=NewVector(64);
    AddVector(stk,root);
    while(stk->tot){
        HistoryTree *h=(HistoryTree*)DeleteVector(stk,stk->tot-1);
        int index=h->level+1;
        while(aux->tot<=index)AddVector(aux,NewVector(8));
        AuxData *data=malloc(sizeof(AuxData));
        data->i=index;
        data->j=AddVector(GetLevel(index),data);
        h->data=data;
        data->h=h;
        data->parent=(index>0)?GetLevel(index-1)->tot-1:-1;
        data->children=NewVectorI(4);
        data->observations=NewVectorI(4);
        data->multiplicities=NewVectorI(4);
        data->outdegree=h->outdegree;
        data->anonymity=0;
        data->visible=false;
        data->width=0;
        for(int i=h->children->tot-1;i>=0;i--)
            AddVector(stk,h->children->items[i]);
    }
    FreeVector(stk);
    /* Phase 2: bottom-up — fill each node's children index list and compute
       widths.  Processing the deepest level first ensures every child's width
       is ready before its parent reads it. */
    for(int lev=aux->tot-1;lev>=0;lev--){
        Vector *v=GetLevel(lev);
        for(int j=0;j<v->tot;j++){
            AuxData *data=(AuxData*)v->items[j];
            HistoryTree *h=data->h;
            for(int k=0;k<h->children->tot;k++){
                AuxData *cdata=(AuxData*)((HistoryTree*)h->children->items[k])->data;
                AddVectorI(data->children,cdata->j);
            }
            if(!data->children->tot){data->width=1;continue;}
            data->width=0;
            for(int k=0;k<data->children->tot;k++)
                data->width+=GetAuxData(lev+1,data->children->items[k])->width;
        }
    }
    return((AuxData*)root->data)->width;
}

static void ComputeAuxDataCoordinates(int i,int j,float x1,float y1,float x2,float y2){
    AuxData *data=GetAuxData(i,j);
    int w=data->width;
    int h=aux->tot;
    data->y=y1+(y2-y1)*(i+0.5f)/h;
    if(!data->children->tot)data->x=(x1+x2)*0.5f;
    else{
        float nx1=x1;
        for(int k=0;k<data->children->tot;k++){
            int c=data->children->items[k];
            float nx2=nx1+(x2-x1)*GetAuxData(i+1,c)->width/w;
            ComputeAuxDataCoordinates(i+1,c,nx1,y1,nx2,y2);
            nx1=nx2;
        }
        data->x=(GetAuxData(i+1,data->children->items[0])->x+GetAuxData(i+1,data->children->items[data->children->tot-1])->x)*0.5f;
    }
}

static void ComputeAuxDataRedEdges(void){
    SetWindowContext(win1);
    /* For each level i, build a hash map HistoryTree* → AuxData-index for level i-1,
       reducing the lookup from O(n) linear scan to O(1).
       Overall cost drops from O(R×n²×obs) to O(R×n×obs).                          */
    for(int i=1;i<aux->tot;i++){
        int pn=GetLevel(i-1)->tot;
        int cap=pn*2+3;
        HistoryTree **hkeys=calloc(cap,sizeof(HistoryTree*));
        int *hvals=malloc(cap*sizeof(int));
        for(int k=0;k<pn;k++){
            HistoryTree *h=GetAuxData(i-1,k)->h;
            int idx=(int)((uintptr_t)h%(unsigned)cap);
            while(hkeys[idx]&&hkeys[idx]!=h){if(++idx==cap)idx=0;}
            hkeys[idx]=h; hvals[idx]=k;
        }
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            for(int l=0;l<data->h->observations->tot;l++){
                Observation *obs=data->h->observations->items[l];
                int idx=(int)((uintptr_t)obs->history%(unsigned)cap);
                while(hkeys[idx]&&hkeys[idx]!=obs->history){if(++idx==cap)idx=0;}
                if(hkeys[idx]==obs->history){
                    AddVectorI(data->observations,hvals[idx]);
                    AddVectorI(data->multiplicities,obs->multiplicity);
                }
            }
        }
        free(hkeys); free(hvals);
    }
}

static void ComputeAuxDataAnonymities(void){
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        ((AuxData*)e->finalLeaf->data)->anonymity++;
    }
    for(int i=aux->tot-2;i>=0;i--)
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            for(int k=0;k<data->children->tot;k++){
                AuxData *child=GetAuxData(i+1,data->children->items[k]);
                data->anonymity+=child->anonymity;
            }
        }
}

void ResetAuxDataVariables(void){
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++){
            AuxData *data=v->items[j];
            data->guess=-1; // no guess has been made
            data->locked=false;
            data->counted=false;
            data->weight=0;
            data->cumulativeAnonymity=0;
            data->guesser=false;
        }
    }
}

void ComputeAuxData(HistoryTree *h){
    FreeAuxData();
    aux=NewVector(8);
    ComputeAuxDataWidth(h);
    ComputeAuxDataCoordinates(0,0,-1.0f,-1.0f,1.0f,1.0f);
    ComputeAuxDataRedEdges();
    ComputeAuxDataAnonymities();
    ResetAuxDataVariables();
}

/* Incrementally extend AuxData by exactly one level after ExtendFinalHistoryOneLevel()
   has added one new deepest level to finalHistory.
   Avoids rebuilding all R levels from scratch:
     - AuxData nodes for levels 0..R-1 are reused (no free/realloc)
     - Red-edge computation runs only for the new level: O(n×obs) instead of O(R×n×obs)
     - Width, coordinate, and anonymity passes still touch all levels: O(R×n) unavoidably
*/
void AppendAuxDataOneLevel(void){
    if(!aux)return; /* fall-through guard; caller should use ComputeAuxData instead */
    SetWindowContext(win1);
    int old_depth=aux->tot;    /* number of AuxData levels before this call */
    int new_level=old_depth;   /* index of the level we are about to add */
    AddVector(aux,NewVector(8));

    /* Create AuxData nodes for every HistoryTree node at the new deepest level.
       These are the children of nodes in GetLevel(old_depth-1) that were just
       created by ExtendFinalHistoryOneLevel (h->data == NULL). */
    Vector *old_deepest=GetLevel(old_depth-1);
    for(int j=0;j<old_deepest->tot;j++){
        AuxData *pdata=old_deepest->items[j];
        HistoryTree *ph=pdata->h;
        for(int k=0;k<ph->children->tot;k++){
            HistoryTree *ch=ph->children->items[k];
            if(ch->data!=NULL)continue; /* already handled (shared child from earlier entity) */
            AuxData *cdata=malloc(sizeof(AuxData));
            cdata->i=new_level;
            cdata->j=AddVector(GetLevel(new_level),cdata);
            ch->data=cdata;
            cdata->h=ch;
            cdata->parent=j;
            cdata->children=NewVectorI(4);
            cdata->observations=NewVectorI(4);
            cdata->multiplicities=NewVectorI(4);
            cdata->outdegree=ch->outdegree;
            cdata->anonymity=0;
            cdata->visible=false;
            cdata->width=1; /* leaf */
            cdata->guess=-1;
            cdata->locked=false;
            cdata->counted=false;
            cdata->weight=0;
            cdata->cumulativeAnonymity=0;
            cdata->guesser=false;
            AddVectorI(pdata->children,cdata->j); /* link parent → new child */
        }
    }

    /* Recompute widths bottom-up from old deepest level to root.
       Only nodes whose children changed (level old_depth-1) and their ancestors
       are affected; iterating all levels in that range is simplest and correct. */
    for(int i=old_depth-1;i>=0;i--){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++){
            AuxData *data=v->items[j];
            if(!data->children->tot){data->width=1;continue;}
            data->width=0;
            for(int k=0;k<data->children->tot;k++)
                data->width+=GetAuxData(i+1,data->children->items[k])->width;
        }
    }

    /* Recompute coordinates for all levels (widths changed, layout must be consistent). */
    ComputeAuxDataCoordinates(0,0,-1.0f,-1.0f,1.0f,1.0f);

    /* Compute red edges for the new level only (O(n×obs) vs O(R×n×obs) full rebuild). */
    {
        int i=new_level;
        int pn=GetLevel(i-1)->tot;
        int cap=pn*2+3;
        HistoryTree **hkeys=calloc(cap,sizeof(HistoryTree*));
        int *hvals=malloc(cap*sizeof(int));
        for(int k=0;k<pn;k++){
            HistoryTree *h=GetAuxData(i-1,k)->h;
            int idx=(int)((uintptr_t)h%(unsigned)cap);
            while(hkeys[idx]&&hkeys[idx]!=h){if(++idx==cap)idx=0;}
            hkeys[idx]=h; hvals[idx]=k;
        }
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            for(int l=0;l<data->h->observations->tot;l++){
                Observation *obs=data->h->observations->items[l];
                int idx=(int)((uintptr_t)obs->history%(unsigned)cap);
                while(hkeys[idx]&&hkeys[idx]!=obs->history){if(++idx==cap)idx=0;}
                if(hkeys[idx]==obs->history){
                    AddVectorI(data->observations,hvals[idx]);
                    AddVectorI(data->multiplicities,obs->multiplicity);
                }
            }
        }
        free(hkeys); free(hvals);
    }

    /* Recompute anonymities: zero all first, then propagate from leaves. */
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++)((AuxData*)v->items[j])->anonymity=0;
    }
    ComputeAuxDataAnonymities();

    /* Reset algorithm state for all nodes (same behaviour as full ComputeAuxData). */
    ResetAuxDataVariables();
}

/* Remove the deepest AuxData level (inverse of AppendAuxDataOneLevel).
   Used after tail rollback or before suffix replay to drop stale levels. */
void TrimAuxDataOneLevel(void){
    if(!aux || aux->tot<=1)return;
    SetWindowContext(win1);
    int last=aux->tot-1;
    if(last>0){
        Vector *prev=GetLevel(last-1);
        for(int j=0;j<prev->tot;j++){
            AuxData *pdata=prev->items[j];
            while(pdata->children->tot)DeleteVectorI(pdata->children,pdata->children->tot-1);
        }
    }
    Vector *deepest=GetLevel(last);
    for(int j=0;j<deepest->tot;j++){
        AuxData *data=deepest->items[j];
        if(data->h)data->h->data=NULL;
        FreeVectorI(data->children);
        FreeVectorI(data->observations);
        FreeVectorI(data->multiplicities);
        free(data);
    }
    FreeVector(deepest);
    DeleteVector(aux,last);
    if(aux->tot<=0)return;
    for(int i=aux->tot-1;i>=0;i--){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++){
            AuxData *data=v->items[j];
            if(!data->children->tot){data->width=1;continue;}
            data->width=0;
            for(int k=0;k<data->children->tot;k++)
                data->width+=GetAuxData(i+1,data->children->items[k])->width;
        }
    }
    ComputeAuxDataCoordinates(0,0,-1.0f,-1.0f,1.0f,1.0f);
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++)((AuxData*)v->items[j])->anonymity=0;
    }
    ComputeAuxDataAnonymities();
    ResetAuxDataVariables();
}

void FreeAuxData(void){
    if(!aux)return;
    SetWindowContext(win1);
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        for(int j=0;j<v->tot;j++){
            AuxData *data=v->items[j];
            FreeVectorI(data->children);
            FreeVectorI(data->observations);
            FreeVectorI(data->multiplicities);
            free(data);
        }
        FreeVector(v);
    }
    FreeVector(aux);
    aux=NULL;
}

static void SelectNode(AuxData *data,bool select){
    data->visible=select;
}

static void SelectViewHelper(int si,int sj){
    for(int i=aux->tot-1;i>=0;i--){
        Vector *v1=GetLevel(i);
        for(int j=0;j<v1->tot;j++)SelectNode(v1->items[j],false);
        if(i==si)SelectNode(v1->items[sj],true);
        else if(i<si){
            Vector *v2=GetLevel(i+1);
            for(int j=0;j<v2->tot;j++){
                AuxData *data=v2->items[j];
                if(!data->visible)continue;
                SelectNode(v1->items[data->parent],true);
                for(int k=0;k<data->observations->tot;k++){
                    int obs=data->observations->items[k];
                    SelectNode(v1->items[obs],true);
                }
            }
        }
    }
}

void SelectView(void){
    if(selectedEntity!=-1){
        Entity *e=GetEntity(selectedEntity);
        HistoryTree *h=e->finalLeaf;
        for(int i=h->level+1;i>currentRound+2;i--)h=h->parent;
        selectedNode=h->data;
        SelectViewHelper(selectedNode->i,selectedNode->j);
    }
    else{
        selectedNode=selectedNodeI==-1?NULL:GetAuxData(selectedNodeI,selectedNodeJ);
        SelectViewHelper(selectedNodeI,selectedNodeJ);
    }
}

void SelectNodeXY(int x,int y,int *si,int *sj){
    float minDist=-1.0f;
    *si=-1; *sj=-1;
    for(int i=0;i<aux->tot;i++){
        Vector *v=GetLevel(i);
        float cy=ToScreenY(win1,((AuxData*)v->items[0])->y);
        if(cy+NODE_SIZE*0.5f<y)continue;
        if(cy-NODE_SIZE*0.5f>y)break;
        for(int j=0;j<v->tot;j++){
            float cx=ToScreenX2(win1,((AuxData*)v->items[j])->x);
            float dist=(cx-x)*(cx-x)+(cy-y)*(cy-y);
            if(*si==-1 || dist<minDist){ minDist=dist; *si=i; *sj=j; }
        }
        break;
    }
    if(*si!=-1 && minDist<=NODE_SIZE*NODE_SIZE*0.25f)return;
    *si=-1; *sj=-1;
}

bool CorrespondsToSelectedNode(Entity *e){
    if(selectedNodeI==-1 || selectedNodeJ==-1)return false;
    AuxData *data=GetAuxData(selectedNodeI,selectedNodeJ);
    HistoryTree *h=e->finalLeaf;
    for(int i=h->level+1;i>selectedNodeI;i--)h=h->parent;
    return h==data->h;
}

Entity *FirstEntityCorrespondingToSelectedNode(void){
    if(selectedNodeI==-1 || selectedNodeJ==-1)return NULL;
    for(int i=0;i<network->entities->tot;i++){
        Entity *e=GetEntity(i);
        if(CorrespondsToSelectedNode(e))return e;
    }
    return NULL;
}

void SelectNodeFromEntity(Entity *e){
    HistoryTree *h=e->finalLeaf;
    for(int i=h->level+1;i>selectedNodeI;i--)h=h->parent;
    AuxData *data=h->data;
    selectedNodeJ=data->j;
}

void IncrementSelectedNodeI(void){
    AuxData *data=GetAuxData(selectedNodeI,selectedNodeJ);
    if(data->children->tot>0){
        selectedNodeI++;
        selectedNodeJ=data->children->items[0];
    }
    else selectedNodeI=selectedNodeJ=-1;
}

void DecrementSelectedNodeI(void){
    AuxData *data=GetAuxData(selectedNodeI,selectedNodeJ);
    if(data->parent!=-1){
        selectedNodeI--;
        selectedNodeJ=data->parent;
    }
    else selectedNodeI=selectedNodeJ=-1;
}

bool IncrementSelectedNodeJ(void){
    if(selectedNodeJ==-1)return false;
    Vector *v=GetLevel(selectedNodeI);
    if(selectedNodeJ>=v->tot-1)return false;
    selectedNodeJ++;
    return true;
}

bool DecrementSelectedNodeJ(void){
    if(selectedNodeJ<=0)return false;
    selectedNodeJ--;
    return true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void TestSelectHistoryNode(int j){
    if(!aux || !network || currentRound<0)return;
    int si=currentRound+2;
    if(si<0 || si>=aux->tot)return;
    Vector *v=GetLevel(si);
    if(j<0 || j>=v->tot)return;
    selectedEntity=-1;
    selectedNodeI=si;
    selectedNodeJ=j;
    selectedNode=GetAuxData(si,j);
    SelectViewHelper(si,j);
    numSteps=-1;
    CountingAlgorithm();
    win1->invalid=true;
}
#endif

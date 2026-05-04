#include "main.h"

/* ── hash utilities ─────────────────────────────────────────── */

static unsigned long long hash_mix(unsigned long long h,unsigned long long v){
    h^=v;
    h*=0x9e3779b97f4a7c15ULL;
    h^=h>>30;
    return h;
}

static int cmp_ull(const void *a,const void *b){
    unsigned long long x=*(const unsigned long long *)a;
    unsigned long long y=*(const unsigned long long *)b;
    return (x>y)-(x<y);
}

/* Merkle hash: input + outdegree + sorted children hashes only.
   Red edges are intentionally excluded to avoid the circular ordering dependency
   (red edges point from level L to level L-1, which has not yet been processed
   in a bottom-up (post-order) traversal). */
static void ComputeNodeHash(HistoryTree *h){
    unsigned long long hash=0xcbf29ce484222325ULL;
    hash=hash_mix(hash,(unsigned long long)h->input);
    hash=hash_mix(hash,(unsigned long long)h->outdegree);
    int nc=h->children->tot;
    hash=hash_mix(hash,(unsigned long long)nc);
    if(nc>0){
        unsigned long long *ch=malloc((size_t)nc*sizeof(unsigned long long));
        for(int i=0;i<nc;i++)ch[i]=((HistoryTree *)h->children->items[i])->hash;
        qsort(ch,(size_t)nc,sizeof(unsigned long long),cmp_ull);
        for(int i=0;i<nc;i++)hash=hash_mix(hash,ch[i]);
        free(ch);
    }
    h->hash=hash;
}

/* Post-order DFS: recompute Merkle hashes for the entire subtree rooted at h. */
void ComputeHashBottomUp(HistoryTree *h){
    for(int i=0;i<h->children->tot;i++)
        ComputeHashBottomUp(h->children->items[i]);
    ComputeNodeHash(h);
}

HistoryTree *NewHistoryTree(void){ // creates new root
    HistoryTree *h=malloc(sizeof(HistoryTree));
    h->input=-1;
    h->level=-1;
    h->parent=NULL;
    h->children=NewVector(4);
    h->observations=NewVector(4);
    h->outdegree=-1;
    h->reference=NULL;
    h->data=NULL;
    h->hash=0;
    return h;
}

void FreeHistoryTree(HistoryTree *h){
    for(int i=0;i<h->observations->tot;i++) // free red edges
        free(h->observations->items[i]);
    FreeVector(h->observations);
    for(int i=0;i<h->children->tot;i++) // free children
        FreeHistoryTree(h->children->items[i]);
    FreeVector(h->children);
    free(h);
}

Observation *NewObservation(HistoryTree *history,int multiplicity){
    Observation *o=malloc(sizeof(Observation));
    o->history=history;
    o->multiplicity=multiplicity;
    o->finalLeafAtSend=NULL;
    return o;
}

static Observation *FindRedEdge(HistoryTree *h1,HistoryTree *h2){ // binary search by h2 pointer; observations kept sorted by o->history
    int lo=0,hi=h1->observations->tot-1;
    uintptr_t key=(uintptr_t)h2;
    while(lo<=hi){
        int mid=(lo+hi)/2;
        Observation *o=h1->observations->items[mid];
        uintptr_t v=(uintptr_t)o->history;
        if(v==key)return o;
        if(v<key)lo=mid+1; else hi=mid-1;
    }
    return NULL;
}

static void AddNewRedEdge(HistoryTree *h1,HistoryTree *h2,int multiplicity){ // insert in sorted position to keep observations sorted by o->history
    Observation *o=NewObservation(h2,multiplicity);
    uintptr_t key=(uintptr_t)h2;
    int lo=0,hi=h1->observations->tot-1;
    while(lo<=hi){
        int mid=(lo+hi)/2;
        if((uintptr_t)((Observation *)h1->observations->items[mid])->history<key)lo=mid+1;
        else hi=mid-1;
    }
    InsertVector(h1->observations,lo,o);
}

void AddRedEdge(HistoryTree *h1,HistoryTree *h2,int multiplicity){ // add red edge between h1 and h2, assuming h1 is in lower level
    Observation *o1=FindRedEdge(h1,h2);
    if(o1)o1->multiplicity+=multiplicity;
    else AddNewRedEdge(h1,h2,multiplicity); // edge does not exist; create it
}

HistoryTree *AddHistoryTreeChild(HistoryTree *h,int input){ // adds a child node to h with given input and returns it
    HistoryTree *h2=NewHistoryTree();
    h2->parent=h;
    h2->input=input;
    h2->level=h->level+1;
    h2->outdegree=outAware?0:-1;
    AddVector(h->children,h2);
    return h2;
}

static void ResetReferences(HistoryTree *h){
    h->reference=NULL;
    for(int i=0;i<h->children->tot;i++)
        ResetReferences(h->children->items[i]);
}

static bool EquivalentNodes(HistoryTree *h1,HistoryTree *h2){ // used when constructing isomorphisms
    if(!h1 || !h2)return false;
    if(!h1->parent){ if(h2->parent)return false; }
    else if(!h2->parent || h1->parent->reference!=h2->parent)return false; // check if parents are isomorphic
    if(h1->level!=h2->level)return false;
    if(h1->input!=h2->input)return false;
    if(h1->outdegree!=h2->outdegree)return false;
    if(h1->observations->tot!=h2->observations->tot)return false;
    for(int i=0;i<h1->observations->tot;i++){ // check if all red edges lead to isomorphic nodes
        Observation *o1=h1->observations->items[i];
        Observation *o2=FindRedEdge(h2,o1->history->reference);
        if(!o2 || o1->multiplicity!=o2->multiplicity)return false;
    }
    return true;
}

/* ── child-lookup hash map ─────────────────────────────────────────────────
   Groups b->children by (input, outdegree, observations_count) — the exact
   fields checked in O(1) at the top of EquivalentNodes.  A key mismatch
   guarantees EquivalentNodes would return false, so we skip the call entirely.
   Collisions (same key, structurally distinct nodes) fall through to
   EquivalentNodes as usual.  Load factor is kept ≤ 50%.               */
typedef struct { unsigned long long key; HistoryTree *node; } ChildEntry;

static unsigned long long local_key(HistoryTree *h){
    unsigned long long k=0xcbf29ce484222325ULL;
    k=hash_mix(k,(unsigned long long)(h->input+0x8000ULL));
    k=hash_mix(k,(unsigned long long)(h->outdegree+2));
    k=hash_mix(k,(unsigned long long)h->observations->tot);
    return k?k:1ULL; /* 0 reserved for empty slot */
}
static void cmap_put(ChildEntry *map,int cap,HistoryTree *node){
    unsigned long long k=local_key(node);
    int idx=(int)(k%(unsigned long long)cap);
    while(map[idx].key){if(++idx==cap)idx=0;}
    map[idx].key=k; map[idx].node=node;
}
static HistoryTree *cmap_get(ChildEntry *map,int cap,HistoryTree *x){
    unsigned long long k=local_key(x);
    int idx=(int)(k%(unsigned long long)cap);
    while(map[idx].key){
        if(map[idx].key==k&&EquivalentNodes(x,map[idx].node))return map[idx].node;
        if(++idx==cap)idx=0;
    }
    return NULL;
}

HistoryTree *MergeHistoryTrees(HistoryTree *h1,HistoryTree *h2,bool *added){ // modifies h1; returns endpoint in new tree and whether any nodes were added
    HistoryTree *deepest=h2;
    if(added)*added=false;
    Queue *q=NewQueue();
    h2->reference=h1; // map root of h2 to root of h1
    AppendQueue(q,h2); // enqueue root of h2
    while(!IsQueueEmpty(q)){
        HistoryTree *a=PopQueue(q); // children of a in h2 must be mapped into h1
        if(a->level>deepest->level)deepest=a;
        HistoryTree *b=a->reference; // corresponding node in h1
        /* Build child-lookup map for b.  Capacity covers existing children plus
           up to a->children->tot new ones so load stays ≤ 50% after inserts. */
        int cap=(b->children->tot+a->children->tot)*2+3;
        ChildEntry *map=(ChildEntry *)calloc(cap,sizeof(ChildEntry));
        for(int j=0;j<b->children->tot;j++) cmap_put(map,cap,b->children->items[j]);
        for(int i=0;i<a->children->tot;i++){
            HistoryTree *x=a->children->items[i];
            AppendQueue(q,x);
            HistoryTree *y=cmap_get(map,cap,x); // O(1) expected; EquivalentNodes only for key matches
            if(!y){
                if(added)*added=true;
                y=AddHistoryTreeChild(b,x->input);
                for(int j=0;j<x->observations->tot;j++){
                    Observation *o=x->observations->items[j];
                    AddNewRedEdge(y,o->history->reference,o->multiplicity);
                }
                y->outdegree=x->outdegree;
                cmap_put(map,cap,y); // keep map current for remaining children of a
            }
            x->reference=y;
        }
        free(map);
    }
    FreeQueue(q);
    deepest=deepest->reference;
    ResetReferences(h2);
    return deepest;
}

HistoryTree *CopyHistoryTree(HistoryTree *h,HistoryTree **deepest){ // returns copied tree and deepest node
    HistoryTree *h2=NewHistoryTree();
    HistoryTree *au=MergeHistoryTrees(h2,h,NULL);
    if(deepest)*deepest=au;
    return h2;
}

bool HistoryTreeContains(HistoryTree *h1,HistoryTree *h2){ // does h1 contain an isomorphic copy of h2?
    bool result=true;
    Queue *q=NewQueue();
    h2->reference=h1; // map root of h2 to root of h1
    AppendQueue(q,h2);
    while(!IsQueueEmpty(q)){
        HistoryTree *a=PopQueue(q);
        if(!result){continue;} // drain queue after failure
        HistoryTree *b=a->reference;
        int nc=b->children->tot;
        int cap=nc*2+3;
        ChildEntry *map=(ChildEntry *)calloc(cap,sizeof(ChildEntry));
        for(int j=0;j<nc;j++) cmap_put(map,cap,b->children->items[j]);
        for(int i=0;i<a->children->tot;i++){
            HistoryTree *x=a->children->items[i];
            HistoryTree *y=cmap_get(map,cap,x);
            if(!y){result=false;break;}
            x->reference=y;
            AppendQueue(q,x);
        }
        free(map);
    }
    FreeQueue(q);
    ResetReferences(h2);
    return result;
}

bool HistoryTreeEquals(HistoryTree *h1,HistoryTree *h2){ // is h1 isomorphic to h2?
    if(h1->hash && h2->hash && h1->hash!=h2->hash)return false; // O(1) short-circuit via Merkle hash
    return HistoryTreeContains(h1,h2) && HistoryTreeContains(h2,h1);
}

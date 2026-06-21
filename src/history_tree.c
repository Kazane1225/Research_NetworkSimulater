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

static int currentMutationRound=0;

void SetHistoryTreeMutationRound(int round){
    currentMutationRound=round;
}

/* Vista hash: encodes the full isomorphism condition at a node.
   hash(input, outdegree, parent->vista_hash,
        sorted multiset of {red_edge_target->vista_hash XOR multiplicity})
   Computed top-down (BFS) so parent and previous-level red-edge targets
   are always available when a node's hash is computed.                   */
static unsigned long long compute_vista_hash_node(HistoryTree *h){
    unsigned long long vh=0xcbf29ce484222325ULL;
    vh=hash_mix(vh,(unsigned long long)(h->input+0x8000ULL));
    vh=hash_mix(vh,(unsigned long long)(h->outdegree+2));
    vh=hash_mix(vh,h->parent ? h->parent->vista_hash : 0xdeadbeefcafe0000ULL);
    int nc=h->observations->tot;
    if(nc>0){
        unsigned long long *pairs=malloc((size_t)nc*sizeof(unsigned long long));
        for(int i=0;i<nc;i++){
            Observation *o=h->observations->items[i];
            pairs[i]=hash_mix(o->history->vista_hash,(unsigned long long)o->multiplicity);
        }
        qsort(pairs,(size_t)nc,sizeof(unsigned long long),cmp_ull);
        for(int i=0;i<nc;i++)vh=hash_mix(vh,pairs[i]);
        free(pairs);
    }
    return vh?vh:1ULL;
}

void ComputeVistaHashTopDown(HistoryTree *h){
    if(!h)return;
    Queue *q=NewQueue();
    AppendQueue(q,h);
    while(!IsQueueEmpty(q)){
        HistoryTree *node=PopQueue(q);
        node->vista_hash=compute_vista_hash_node(node);
        for(int i=0;i<node->children->tot;i++)
            AppendQueue(q,node->children->items[i]);
    }
    FreeQueue(q);
}

HistoryTree *NewHistoryTree(void){ // creates new root
    HistoryTree *h=malloc(sizeof(HistoryTree));
    h->input=-1;
    h->level=-1;
    h->bornRound=-1;
    h->parent=NULL;
    h->children=NewVector(4);
    h->observations=NewVector(4);
    h->outdegree=-1;
    h->reference=NULL;
    h->data=NULL;
    h->vista_hash=0;
    return h;
}

void FreeHistoryTree(HistoryTree *h){
    if(!h)return;
    Vector *stack=NewVector(16);
    Vector *post=NewVector(16);
    h->reference=h;
    AddVector(stack,h);
    while(stack->tot){
        HistoryTree *node=DeleteVector(stack,stack->tot-1);
        AddVector(post,node);
        for(int i=0;i<node->children->tot;i++){
            HistoryTree *child=node->children->items[i];
            if(child->reference==child)continue;
            child->reference=child;
            AddVector(stack,child);
        }
    }
    while(post->tot){
        HistoryTree *node=DeleteVector(post,post->tot-1);
        for(int i=0;i<node->observations->tot;i++)
            free(node->observations->items[i]);
        FreeVector(node->observations);
        FreeVector(node->children);
        free(node);
    }
    FreeVector(post);
    FreeVector(stack);
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
    h2->bornRound=currentMutationRound;
    h2->outdegree=outAware?0:-1;
    AddVector(h->children,h2);
    return h2;
}

void TrimHistoryTreeToRound(HistoryTree *h,int prefixRound){
    if(!h)return;
    Vector *stack=NewVector(16);
    Vector *post=NewVector(16);
    AddVector(stack,h);
    while(stack->tot){
        HistoryTree *node=DeleteVector(stack,stack->tot-1);
        AddVector(post,node);
        for(int i=0;i<node->children->tot;i++)
            AddVector(stack,node->children->items[i]);
    }
    while(post->tot){
        HistoryTree *node=DeleteVector(post,post->tot-1);
        if(node==h || node->bornRound<=prefixRound)continue;
        if(node->parent && node->parent->bornRound<=prefixRound){
            for(int i=0;i<node->parent->children->tot;i++){
                if(node->parent->children->items[i]==node){
                    DeinsertVector(node->parent->children,i);
                    break;
                }
            }
        }
        for(int i=0;i<node->observations->tot;i++)
            free(node->observations->items[i]);
        FreeVector(node->observations);
        FreeVector(node->children);
        free(node);
    }
    FreeVector(post);
    FreeVector(stack);
}

static void ResetReferences(HistoryTree *h){
    if(!h)return;
    Vector *stack=NewVector(16);
    AddVector(stack,h);
    while(stack->tot){
        HistoryTree *node=DeleteVector(stack,stack->tot-1);
        node->reference=NULL;
        for(int i=0;i<node->children->tot;i++)
            AddVector(stack,node->children->items[i]);
    }
    FreeVector(stack);
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
   Groups b->children by vista_hash when available, else by (input, outdegree,
   observations_count).  Load factor is kept ≤ 50%.                         */
typedef struct { unsigned long long key; HistoryTree *node; } ChildEntry;

static unsigned long long local_key(HistoryTree *h){
    unsigned long long k=0xcbf29ce484222325ULL;
    k=hash_mix(k,(unsigned long long)(h->input+0x8000ULL));
    k=hash_mix(k,(unsigned long long)(h->outdegree+2));
    k=hash_mix(k,(unsigned long long)h->observations->tot);
    return k?k:1ULL; /* 0 reserved for empty slot */
}

static unsigned long long node_key(HistoryTree *h){
    return h->vista_hash ? h->vista_hash : local_key(h);
}

static void cmap_put(ChildEntry *map,int cap,HistoryTree *node){
    unsigned long long k=node_key(node);
    int idx=(int)(k%(unsigned long long)cap);
    while(map[idx].key){if(++idx==cap)idx=0;}
    map[idx].key=k; map[idx].node=node;
}

static HistoryTree *cmap_get(ChildEntry *map,int cap,HistoryTree *x){
    unsigned long long k=node_key(x);
    int idx=(int)(k%(unsigned long long)cap);
    while(map[idx].key){
        if(map[idx].key==k){
            HistoryTree *cand=map[idx].node;
            if(x->vista_hash&&cand->vista_hash)return cand;
            if(EquivalentNodes(x,cand))return cand;
        }
        if(++idx==cap)idx=0;
    }
    return NULL;
}

HistoryTree *MergeHistoryTrees(HistoryTree *h1,HistoryTree *h2,bool *added){ // modifies h1; returns endpoint in new tree and whether any nodes were added
    HistoryTree *deepest=h2;
    if(added)*added=false;
    Queue *q=NewQueue();
    h2->reference=h1; // map root of h2 to root of h1
    if(!h1->vista_hash)h1->vista_hash=compute_vista_hash_node(h1);
    AppendQueue(q,h2); // enqueue root of h2
    while(!IsQueueEmpty(q)){
        HistoryTree *a=PopQueue(q); // children of a in h2 must be mapped into h1
        if(a->level>deepest->level)deepest=a;
        HistoryTree *b=a->reference; // corresponding node in h1
        int cap=(b->children->tot+a->children->tot)*2+3;
        ChildEntry *map=(ChildEntry *)calloc(cap,sizeof(ChildEntry));
        for(int j=0;j<b->children->tot;j++) cmap_put(map,cap,b->children->items[j]);
        for(int i=0;i<a->children->tot;i++){
            HistoryTree *x=a->children->items[i];
            AppendQueue(q,x);
            if(!x->vista_hash)x->vista_hash=compute_vista_hash_node(x);
            HistoryTree *y=cmap_get(map,cap,x);
            if(!y){
                if(added)*added=true;
                y=AddHistoryTreeChild(b,x->input);
                for(int j=0;j<x->observations->tot;j++){
                    Observation *o=x->observations->items[j];
                    AddNewRedEdge(y,o->history->reference,o->multiplicity);
                }
                y->outdegree=x->outdegree;
                y->vista_hash=compute_vista_hash_node(y);
                cmap_put(map,cap,y);
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

HistoryTree *CopyHistoryTree(HistoryTree *h,HistoryTree **deepest){
    HistoryTree *root=NewHistoryTree();
    HistoryTree *au=root;
    Queue *q=NewQueue();
    h->reference=root;
    AppendQueue(q,h);
    while(!IsQueueEmpty(q)){
        HistoryTree *src=PopQueue(q);
        HistoryTree *dst=src->reference;
        dst->input=src->input;
        dst->level=src->level;
        dst->bornRound=src->bornRound;
        dst->outdegree=src->outdegree;
        dst->vista_hash=src->vista_hash;
        if(dst->level>au->level)au=dst;
        for(int i=0;i<src->observations->tot;i++){
            Observation *o=src->observations->items[i];
            AddNewRedEdge(dst,o->history->reference,o->multiplicity);
        }
        for(int i=0;i<src->children->tot;i++){
            HistoryTree *sc=src->children->items[i];
            HistoryTree *dc=NewHistoryTree();
            dc->parent=dst;
            AddVector(dst->children,dc);
            sc->reference=dc;
            AppendQueue(q,sc);
        }
    }
    FreeQueue(q);
    ResetReferences(h);
    if(deepest)*deepest=au;
    return root;
}

bool HistoryTreeContains(HistoryTree *h1,HistoryTree *h2){
    bool result=true;
    Queue *q=NewQueue();
    h2->reference=h1;
    AppendQueue(q,h2);
    while(!IsQueueEmpty(q)){
        HistoryTree *a=PopQueue(q);
        if(!result){continue;}
        HistoryTree *b=a->reference;
        int nc=b->children->tot;
        int cap=nc*2+3;
        ChildEntry *map=(ChildEntry *)calloc(cap,sizeof(ChildEntry));
        for(int j=0;j<nc;j++) cmap_put(map,cap,b->children->items[j]);
        for(int i=0;i<a->children->tot;i++){
            HistoryTree *x=a->children->items[i];
            if(!x->vista_hash)x->vista_hash=compute_vista_hash_node(x);
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

bool HistoryTreeEquals(HistoryTree *h1,HistoryTree *h2){
    if(!h1->vista_hash)ComputeVistaHashTopDown(h1);
    if(!h2->vista_hash)ComputeVistaHashTopDown(h2);
    return h1->vista_hash==h2->vista_hash;
}

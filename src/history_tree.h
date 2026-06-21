typedef struct{ // used for messages between entities and for red edges
    struct HistoryTree *history; // history observed
    int multiplicity; // number of times observed
    struct HistoryTree *finalLeafAtSend; // sender's finalLeaf at send time; NULL for persistent red edges
}Observation;

typedef struct HistoryTree{
    int input; // 0: leader
    int level; // level in the history tree (i.e., round number)
    int bornRound; // prefix round when this node was first materialized
    struct HistoryTree *parent; // parent node
    Vector *children; // children nodes; Vector of HistoryTree
    Vector *observations; // red edges to the previous level; Vector of Observation
    int outdegree; // messages sent in previous round; -1 if not outAware
    struct HistoryTree *reference; // target of isomorphism (only used when merging history trees)
    void *data; // only used in finalHistory
    unsigned long long vista_hash; // top-down vista hash: encodes full isomorphism condition (0 = not computed)
}HistoryTree;

HistoryTree *NewHistoryTree(void); // creates new root
void FreeHistoryTree(HistoryTree *h);
Observation *NewObservation(HistoryTree *history,int multiplicity);
void AddRedEdge(HistoryTree *h1,HistoryTree *h2,int multiplicity); // create red edge between h1 and h2, assuming h1 is in lower level
HistoryTree *AddHistoryTreeChild(HistoryTree *h,int input); // adds a child node to h with given input and return it
void SetHistoryTreeMutationRound(int round); // mark newly created nodes with the active replay/build prefix
void TrimHistoryTreeToRound(HistoryTree *h,int prefixRound); // remove nodes created after prefixRound
HistoryTree *MergeHistoryTrees(HistoryTree *h1,HistoryTree *h2,bool *added); // result is in h1; returns endpoint in new tree and whether any nodes were added
HistoryTree *CopyHistoryTree(HistoryTree *h,HistoryTree **deepest); // returns copied tree and deepest node
bool HistoryTreeContains(HistoryTree *h1,HistoryTree *h2); // does h1 contain an isomorphic copy of h2?
bool HistoryTreeEquals(HistoryTree *h1,HistoryTree *h2); // is h1 isomorphic to h2?
void ComputeVistaHashTopDown(HistoryTree *h); // compute vista hashes top-down for entire subtree

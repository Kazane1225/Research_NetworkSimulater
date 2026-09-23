typedef struct{
    Entity *e1,*e2;
    int multiplicity;
}Interaction;

typedef struct{
    Vector *entities; // Vector of Entity
    Vector *rounds; // dynamic network; Vector of Vector of Interaction
}Network;

extern Network *network;
extern int currentRound;
extern _Thread_local HistoryTree *finalHistory; // thread-local: only ever built/read by the compute
                                                 // worker thread (see src/compute_job.c); the main
                                                 // thread never dereferences this directly
extern int selectedEntity;
extern int selectedNodeI,selectedNodeJ;
extern bool drawingEdge;
extern bool draggingEntity;
extern int algorithm;
extern int numSteps; // number of counting algorithm steps to perform (-1: infinity)

Entity *GetEntity(int i);
int GetEntityIndex(Entity *e);
void SortLeaders(void); // put all leaders at the beginning of the list of entities
void InitNetwork(int type,int n);
// Each of these writes into outFinalLeaf (caller-allocated, network->entities->tot slots,
// indexed like network->entities) instead of writing Entity->finalLeaf directly, since that
// field may be read concurrently by the main (render/input) thread while ExecuteNetwork() runs
// on the compute worker thread (see src/compute_job.c and events.c's RunRecompute()). The caller
// applies outFinalLeaf to each Entity->finalLeaf once the result is ready to be made visible.
//
// Only ExecuteNetwork() is ever dispatched to the worker thread. ReExecuteLastRound/
// AppendLastRound/RollBackLastRound intentionally mutate finalHistory/aux *in place* (that's
// what makes them fast), so they always run synchronously on the main thread instead -- see
// compute_job.h for why that in-place mutation is not safe to run concurrently with rendering.
void ExecuteNetwork(HistoryTree **outFinalLeaf);
void ReExecuteLastRound(HistoryTree **outFinalLeaf); // re-execute only the last round using saved snapshots
void RollBackLastRound(HistoryTree **outFinalLeaf);  // restore to pre-last-round state after last round was deleted
void AppendLastRound(HistoryTree **outFinalLeaf);    // apply newly appended last round on top of current entity states
void DoneNetwork(void);
void InsertRound(int index,bool copy);
void DeleteInteractions(int index);
void DeleteRound(int index);
void AddEntity(int input,float x,float y);
void DeleteEntity(int index);
void AddInteraction(int round,int i,int j,int multiplicity); // interaction from entity i to entity j
void AddDoubleInteraction(int round,int i,int j,int multiplicity); // interaction between entity i and entity j
int SelectEntityXY(int x,int y);
void CountingAlgorithm(void);
void LoadNetwork(void);
void SaveNetwork(void);
#ifndef __EMSCRIPTEN__
void LoadNetworkRun(const char *filename);
void SaveNetworkRun(const char *filename);
#endif
#ifdef __EMSCRIPTEN__
int GetNumAgents(void);
int GetNumLeaders(void);
int GetNumRounds(void);
int GetCurrentRound(void);
int GetCurrentRoundLinks(void);
int GetNumAnonymityClasses(void);
int GetNumUniqueAgents(void);
int GetRootGuess(void);
void TutorialLoadNetwork(void);
int GetSelectedEntity(void);
#endif

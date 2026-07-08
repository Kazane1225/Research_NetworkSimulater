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
extern HistoryTree *finalHistory; // history tree to be displayed in window 2
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
void ExecuteNetwork(void);
void ExecuteNetworkFromRound(int firstRound); // restore nearest checkpoint and replay suffix from firstRound
void MarkNetworkDirtyFromRound(int firstRound); // defer recompute: mark simulation dirty from firstRound
void QueueSelectRestore(Entity *e); // restore history-tree highlight after deferred recompute
bool IsNetworkDirty(void); // true while a deferred recompute is still pending
bool EnsureNetworkComputed(void); // run deferred recompute if dirty; returns true iff recompute ran
void ReExecuteLastRound(void); // re-execute only the last round using saved snapshots
void RollBackLastRound(void);  // restore to pre-last-round state after last round was deleted
void AppendLastRound(void);    // apply newly appended last round on top of current entity states
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

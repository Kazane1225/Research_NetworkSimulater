typedef struct{
    int input; // 0: leader
    float x,y; // used for drawing
    HistoryTree *finalLeaf; // leaf in the final history tree representing entity
    int index; // position in network->entities; refreshed before each simulated round
}Entity;

extern bool outAware;

Entity *NewEntity(int input,float x,float y);
void FreeEntity(Entity *e);
bool IsLeader(Entity *e);

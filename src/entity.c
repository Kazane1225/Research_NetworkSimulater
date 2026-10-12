#include "main.h"

bool outAware=false;

Entity *NewEntity(int input,float x,float y){
    Entity *e=malloc(sizeof(Entity));
    e->input=input;
    e->finalLeaf=NULL;
    e->x=x;
    e->y=y;
    e->index=-1;
    return e;
}

void FreeEntity(Entity *e){
    free(e);
}

bool IsLeader(Entity *e){
    return !e->input;
}

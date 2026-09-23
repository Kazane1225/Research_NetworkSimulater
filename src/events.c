#include "main.h"

const bool *keyboardState;
bool resizeHover=false,resizing=false,helping=false;
bool capsPressed=false;
#ifdef __EMSCRIPTEN__
static bool webCtrlHeld=false;
static bool webAltHeld=false;
static bool webShiftHeld=false;
static bool webQHeld=false;
static bool webCapsTwoWayDefault=false;
#endif

bool IsDeleteLinkModifierDown(void){
    #ifdef __EMSCRIPTEN__
    return webCtrlHeld || webQHeld;
    #else
    return keyboardState[SDL_SCANCODE_LCTRL] || keyboardState[SDL_SCANCODE_RCTRL] || keyboardState[SDL_SCANCODE_Q];
    #endif
}

bool IsAllRoundsModifierDown(void){
    #ifdef __EMSCRIPTEN__
    return webAltHeld;
    #else
    return keyboardState[SDL_SCANCODE_LALT] || keyboardState[SDL_SCANCODE_RALT];
    #endif
}

bool IsCapsTwoWayDefault(void){
    #ifdef __EMSCRIPTEN__
    return webCapsTwoWayDefault;
    #else
    return (SDL_GetModState()&SDL_KMOD_CAPS)!=0;
    #endif
}

bool IsTwoWayLinkModifierDown(void){
    bool bothWays;
    #ifdef __EMSCRIPTEN__
    bothWays=webShiftHeld;
    #else
    bothWays=keyboardState[SDL_SCANCODE_LSHIFT] || keyboardState[SDL_SCANCODE_RSHIFT];
    #endif
    if(IsCapsTwoWayDefault())bothWays=!bothWays;
    return bothWays;
}

static bool CloseToSeparator(float x){
    return x>=SeparatorX()-5.0f && x<=SeparatorX()+5.0f;
}

void FlushEvents(void){
    SDL_Event event;
    while(SDL_PollEvent(&event));
}

// Interactive edits must not mutate `network` (or call the synchronous incremental recompute
// functions, which touch the same entity fields the compute worker thread may still be using)
// while a job is in flight. Callers check this first and skip the whole edit if busy, showing an
// explicit message so the user knows why nothing happened, instead of silently ignoring input.
static bool CanMutateNetwork(void){
    if(ComputeJob_IsActive()){
        DisplayMessage("Still computing the previous change, please wait...");
        return false;
    }
    return true;
}

// Runs as a ComputeJob completion callback: re-selects the history-tree node corresponding to
// `userdata` (an Entity*) now that aux/finalLeaf have just been committed.
static void OnCompleteSelectEntity(void *userdata){
    if(userdata)SelectNodeFromEntity((Entity*)userdata);
}

// Runs one of the synchronous incremental recompute functions (ReExecuteLastRound/
// AppendLastRound/RollBackLastRound), which write into a caller-provided array instead of
// Entity->finalLeaf directly (see network.h), and immediately copies the result into each
// entity's finalLeaf. This all happens in one call on the main thread, so there is no
// concurrency concern here, exactly like the original synchronous code.
static void ApplyIncremental(void(*incrementalFn)(HistoryTree**)){
    int n=network->entities->tot;
    HistoryTree **buf=malloc((size_t)(n>0?n:1)*sizeof(HistoryTree*));
    incrementalFn(buf);
    for(int i=0;i<n;i++)GetEntity(i)->finalLeaf=buf[i];
    free(buf);
}

// Performs the recompute for an edit that either only touches the last round (fast, already-
// optimized synchronous incremental path: fastPathFn is one of ReExecuteLastRound/
// AppendLastRound/RollBackLastRound) or requires a full replay (dispatched to the compute
// worker thread; see compute_job.h for why only the full-replay path can safely run there).
// Either way, `entityToReselect` (may be NULL) is re-selected and CountingAlgorithm() runs only
// once aux/finalLeaf are up to date, so both paths behave identically to the old synchronous
// code from the caller's point of view. Callers must have already checked CanMutateNetwork()
// before mutating `network` and calling this.
static void RunRecompute(bool useFastPath,void(*fastPathFn)(HistoryTree**),Entity *entityToReselect){
    if(useFastPath){
        ApplyIncremental(fastPathFn);
        if(entityToReselect)SelectNodeFromEntity(entityToReselect);
        numSteps=-1;
        CountingAlgorithm();
        win1->invalid=true;
    }
    else{
        numSteps=-1;
        ComputeJob_DispatchExecuteNetwork(OnCompleteSelectEntity,entityToReselect,true);
    }
}

static void IncrementCurrentRound(void){
    if(currentRound<network->rounds->tot-1){
        currentRound++;
        if(selectedNodeI!=-1)IncrementSelectedNodeI();
        numSteps=-1;
        CountingAlgorithm();
        win1->invalid=true;
    }
}

static void DecrementCurrentRound(void){
    if(currentRound>=-1){
        currentRound--;
        if(selectedNodeI!=-1)DecrementSelectedNodeI();
    }
    else if(selectedNodeI>0)DecrementSelectedNodeI();
    numSteps=-1;
    CountingAlgorithm();
    win1->invalid=true;
}

static void KeyPressed(SDL_Keycode key){
    Entity *e;
    int temp1,temp2,temp3;
    switch(key){
        case SDLK_ESCAPE:
            if(selectedEntity!=-1 || selectedNodeI!=-1){
                selectedEntity=-1;
                selectedNodeI=selectedNodeJ=-1;
                numSteps=-1;
                drawingEdge=false;
                CountingAlgorithm();
                win1->invalid=true;
            }
            break;
        case SDLK_UP: DecrementCurrentRound(); break;
        case SDLK_DOWN: IncrementCurrentRound(); break;
        case SDLK_LEFT:
            temp1=selectedEntity;
            temp2=selectedNodeI;
            temp3=selectedNodeJ;
            if(selectedEntity>=0){
                selectedNodeI=currentRound+2;
                SelectNodeFromEntity(GetEntity(selectedEntity));
                selectedEntity=-1;
            }
            if(DecrementSelectedNodeJ()){
                numSteps=-1;
                CountingAlgorithm();
                win1->invalid=true;
            }
            else{
                selectedEntity=temp1;
                selectedNodeI=temp2;
                selectedNodeJ=temp3;
            }
            break;
        case SDLK_RIGHT:
            temp1=selectedEntity;
            temp2=selectedNodeI;
            temp3=selectedNodeJ;
            if(selectedEntity>=0){
                selectedNodeI=currentRound+2;
                SelectNodeFromEntity(GetEntity(selectedEntity));
                selectedEntity=-1;
            }
            if(IncrementSelectedNodeJ()){
                numSteps=-1;
                CountingAlgorithm();
                win1->invalid=true;
            }
            else{
                selectedEntity=temp1;
                selectedNodeI=temp2;
                selectedNodeJ=temp3;
            }
            break;
        case SDLK_BACKSPACE:
            if(currentRound>=0 && CanMutateNetwork()){
                Entity *en=FirstEntityCorrespondingToSelectedNode();
                DeleteInteractions(currentRound);
                DisplayMessage("Delete all links in current round");
                RunRecompute(currentRound==network->rounds->tot-1,ReExecuteLastRound,en);
            }
            break;
        case SDLK_DELETE:
        case SDLK_U:
            selectedNode=NULL;
            if(selectedEntity!=-1&&network->entities->tot>1&&CanMutateNetwork()){
                DeleteEntity(selectedEntity);
                selectedEntity=-1;
                drawingEdge=false;
                DisplayMessage("Delete selected agent");
                ComputeJob_DispatchExecuteNetwork(NULL,NULL,false);
            }
            if(selectedNodeI!=-1 && selectedNodeJ!=-1 && CanMutateNetwork()){
                bool changed=false;
                for(int i=network->entities->tot-1;i>=0&&network->entities->tot>1;i--){
                    Entity *en=GetEntity(i);
                    if(CorrespondsToSelectedNode(en)){
                        changed=true;
                        DeleteEntity(i);
                    }
                }
                if(changed){
                    selectedNodeI=selectedNodeJ=-1;
                    DisplayMessage("Delete selected agents");
                    ComputeJob_DispatchExecuteNetwork(NULL,NULL,false);
                }
            }
            break;
        case SDLK_EQUALS:
        case SDLK_KP_PLUS:
            if(currentRound>=-1 && CanMutateNetwork()){
                e=FirstEntityCorrespondingToSelectedNode();
                InsertRound(++currentRound,true);
                if(selectedNodeI!=-1)selectedNodeI++;
                DisplayMessage("Insert new round");
                RunRecompute(currentRound==network->rounds->tot-1,AppendLastRound,e);
            }
            break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            if(currentRound>=0 && CanMutateNetwork()){
                e=FirstEntityCorrespondingToSelectedNode();
                DeleteRound(currentRound);
                bool wasLastRound=(currentRound==network->rounds->tot);
                if(wasLastRound){
                    currentRound--;
                    if(selectedNodeI!=-1)selectedNodeI--;
                }
                DisplayMessage("Delete current round");
                RunRecompute(wasLastRound,RollBackLastRound,e);
            }
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
        case SDLK_Q:
            if(drawingEdge)DisplayMessage("Delete link");
            break;
        case SDLK_LALT:
        case SDLK_RALT:
            if(drawingEdge)DisplayMessage("Affect all rounds");
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            if(drawingEdge){
                if(IsCapsTwoWayDefault())DisplayMessage("One-way link");
                else DisplayMessage("Two-way link");
            }
            break;
        case SDLK_CAPSLOCK:
            capsPressed=true;
            break;
        case SDLK_0:
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        case SDLK_9:
            if(selectedEntity!=-1){
                if(currentRound<-1){
                    DisplayMessage("Cannot assign input before round 0");
                    break;
                }
                int input=key-SDLK_0;
                Entity *en=GetEntity(selectedEntity);
                if(input!=en->input && CanMutateNetwork()){
                    en->input=input;
                    SortLeaders();
                    DisplayMessage("Change input of selected agent");
                    numSteps=-1;
                    ComputeJob_DispatchExecuteNetwork(NULL,NULL,true);
                }
            }
            if(selectedNodeI!=-1 && selectedNodeJ!=-1){
                if(currentRound<-1){
                    DisplayMessage("Cannot assign input before round 0");
                    break;
                }
                int input=key-SDLK_0;
                Entity *changed=NULL;
                if(CanMutateNetwork())
                    for(int i=0;i<network->entities->tot;i++){
                        Entity *en=GetEntity(i);
                        if(CorrespondsToSelectedNode(en) && input!=en->input){
                            changed=en;
                            en->input=input;
                        }
                    }
                if(changed){
                    SortLeaders();
                    DisplayMessage("Change input of selected agents");
                    numSteps=-1;
                    ComputeJob_DispatchExecuteNetwork(OnCompleteSelectEntity,changed,true);
                }
            }
            break;
        case SDLK_TAB:
            algorithm++;
            if(algorithm>2)algorithm=0;
            numSteps=-1;
            CountingAlgorithm();
            win1->invalid=true;
            break;
        case SDLK_O:
            if(CanMutateNetwork()){
                outAware=!outAware;
                if(outAware)DisplayMessage("Agents are outdegree-aware");
                else DisplayMessage("Agents are not outdegree-aware");
                numSteps=-1;
                ComputeJob_DispatchExecuteNetwork(NULL,NULL,true);
            }
            break;
        case SDLK_A:
            renderArrows=!renderArrows;
            win1->invalid=true;
            if(renderArrows)DisplayMessage("Display arrowheads");
            else DisplayMessage("Do not display arrowheads");
            break;
        case SDLK_D:
            renderLinks++;
            if(renderLinks>2)renderLinks=0;
            win1->invalid=true;
            if(outAware)
                switch(renderLinks){
                    case 0: DisplayMessage("Display all red edges and outdegrees"); break;
                    case 1: DisplayMessage("Display red edges and outdegrees in selected Vista"); break;
                    case 2: DisplayMessage("Do not display red edges and outdegrees"); break;
                    default: break;
                }
            else
                switch(renderLinks){
                    case 0: DisplayMessage("Display all red edges"); break;
                    case 1: DisplayMessage("Display red edges in selected Vista"); break;
                    case 2: DisplayMessage("Do not display red edges"); break;
                    default: break;
                }

            break;
        case SDLK_B:
            roundNodes=!roundNodes;
            win1->invalid=true;
            if(roundNodes)DisplayMessage("Round nodes");
            else DisplayMessage("Square nodes");
            break;
        case SDLK_C:
            renderBar=!renderBar;
            win1->invalid=true;
            if(renderBar)DisplayMessage("Highlight current level");
            else DisplayMessage("Do not highlight current level");
            break;
        case SDLK_G:
            for(int i=0;i<network->entities->tot;i++){
                Entity *en=GetEntity(i);
                en->x=round(en->x*ENTITY_GRID)/ENTITY_GRID;
                en->y=round(en->y*ENTITY_GRID)/ENTITY_GRID;
                win1->invalid=true;
                DisplayMessage("Snap agents to grid");
            }
            break;
        case SDLK_H:
            helping=true;
            resizeHover=resizing=false;
            win1->invalid=true;
            break;
        case SDLK_SPACE:
            if(!algorithm || !selectedNode)break;
            numSteps++;
            if(numSteps)DisplayMessage("Execute step %d",numSteps);
            else DisplayMessage("Execute counting algorithm step by step");
            CountingAlgorithm();
            win1->invalid=true;
            break;
        case SDLK_L:
            if(drawingEdge || draggingEntity)break;
            LoadNetwork();
            break;
        case SDLK_S:
            if(drawingEdge || draggingEntity)break;
            SaveNetwork();
            break;
        default: break;
    }
}

static void KeyReleased(SDL_Keycode key){
    switch(key){
        case SDLK_LCTRL:
        case SDLK_RCTRL:
        case SDLK_Q:
            if(drawingEdge)DisplayMessage("Create link");
            break;
        case SDLK_LALT:
        case SDLK_RALT:
            if(drawingEdge)DisplayMessage("Affect current round only");
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            if(drawingEdge){
                if(IsCapsTwoWayDefault())DisplayMessage("Double link");
                else DisplayMessage("Single link");
            }
            break;
        default: break;
    }
}


#ifdef __EMSCRIPTEN__
static bool IsAsciiLetter(char c){
    return (c>='a'&&c<='z') || (c>='A'&&c<='Z');
}

static bool HasWebString(const char *s){
    return s[0]!='\0';
}

static char ToUpperAscii(char c){
    if(c>='a'&&c<='z')return (char)(c-'a'+'A');
    return c;
}

static bool IsSingleChar(const char *s,char c){
    return s[0]==c && s[1]=='\0';
}

static SDL_Keycode MapWebKeyToSDL(const EmscriptenKeyboardEvent *e){
    if(HasWebString(e->code)){
        if(!SDL_strcmp(e->code,"Escape"))return SDLK_ESCAPE;
        if(!SDL_strcmp(e->code,"Backspace"))return SDLK_BACKSPACE;
        if(!SDL_strcmp(e->code,"Delete"))return SDLK_DELETE;
        if(!SDL_strcmp(e->code,"Enter") || !SDL_strcmp(e->code,"NumpadEnter"))return SDLK_RETURN;
        if(!SDL_strcmp(e->code,"ArrowDown"))return SDLK_DOWN;
        if(!SDL_strcmp(e->code,"ArrowUp"))return SDLK_UP;
        if(!SDL_strcmp(e->code,"ArrowLeft"))return SDLK_LEFT;
        if(!SDL_strcmp(e->code,"ArrowRight"))return SDLK_RIGHT;
        if(!SDL_strcmp(e->code,"Tab"))return SDLK_TAB;
        if(!SDL_strcmp(e->code,"Space"))return SDLK_SPACE;
        if(!SDL_strcmp(e->code,"CapsLock"))return SDLK_CAPSLOCK;
        if(!SDL_strcmp(e->code,"ShiftLeft"))return SDLK_LSHIFT;
        if(!SDL_strcmp(e->code,"ShiftRight"))return SDLK_RSHIFT;
        if(!SDL_strcmp(e->code,"ControlLeft"))return SDLK_LCTRL;
        if(!SDL_strcmp(e->code,"ControlRight"))return SDLK_RCTRL;
        if(!SDL_strcmp(e->code,"AltLeft"))return SDLK_LALT;
        if(!SDL_strcmp(e->code,"AltRight") || !SDL_strcmp(e->code,"AltGraph"))return SDLK_RALT;
        if(!SDL_strcmp(e->code,"NumpadAdd"))return SDLK_KP_PLUS;
        if(!SDL_strcmp(e->code,"NumpadSubtract"))return SDLK_KP_MINUS;
        if(!SDL_strcmp(e->code,"Equal"))return SDLK_EQUALS;
        if(!SDL_strcmp(e->code,"Minus"))return SDLK_MINUS;
    }
    if(HasWebString(e->key)){
        if(IsSingleChar(e->key,'+') || IsSingleChar(e->key,'='))return SDLK_EQUALS;
        if(IsSingleChar(e->key,'-'))return SDLK_MINUS;
        if(e->key[0]>='0' && e->key[0]<='9' && e->key[1]=='\0')return SDLK_0+(e->key[0]-'0');
        if(IsAsciiLetter(e->key[0]) && e->key[1]=='\0')return SDLK_A+(ToUpperAscii(e->key[0])-'A');
    }
    if(HasWebString(e->code)){
        if(!SDL_strncmp(e->code,"Digit",5) && e->code[5]>='0' && e->code[5]<='9' && e->code[6]=='\0')
            return SDLK_0+(e->code[5]-'0');
        if(!SDL_strncmp(e->code,"Numpad",6) && e->code[6]>='0' && e->code[6]<='9' && e->code[7]=='\0')
            return SDLK_0+(e->code[6]-'0');
        if(!SDL_strncmp(e->code,"Key",3) && IsAsciiLetter(e->code[3]) && e->code[4]=='\0')
            return SDLK_A+(ToUpperAscii(e->code[3])-'A');
    }
    return SDLK_UNKNOWN;
}

static bool IsWebQKey(const EmscriptenKeyboardEvent *e){
    if(HasWebString(e->key) && (IsSingleChar(e->key,'q') || IsSingleChar(e->key,'Q')))return true;
    return HasWebString(e->code) && !SDL_strcmp(e->code,"KeyQ");
}

static void UpdateWebModifierState(const EmscriptenKeyboardEvent *e,bool pressed){
    webCtrlHeld=e->ctrlKey;
    webAltHeld=e->altKey;
    webShiftHeld=e->shiftKey;
    if(IsWebQKey(e))webQHeld=pressed;
    if(pressed && HasWebString(e->code) && !SDL_strcmp(e->code,"CapsLock") && !e->repeat)webCapsTwoWayDefault=!webCapsTwoWayDefault;
}

EM_BOOL KeyPressedCallback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
    (void)eventType; (void)userData;
    if(helping){
        helping=false;
        win1->invalid=true;
        return EM_TRUE;
    }
    UpdateWebModifierState(e,true);
    SDL_Keycode key=MapWebKeyToSDL(e);
    if(key!=SDLK_UNKNOWN)KeyPressed(key);
    return EM_TRUE;
}

EM_BOOL KeyReleasedCallback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
    (void)eventType; (void)userData;
    if(helping)return EM_TRUE;
    UpdateWebModifierState(e,false);
    SDL_Keycode key=MapWebKeyToSDL(e);
    if(key!=SDLK_UNKNOWN)KeyReleased(key);
    return EM_TRUE;
}
#endif

static void MousePressed1(SDL_MouseButtonEvent *button){ // specific to left panel
    if(selectedNodeI!=-1 || selectedNodeJ!=-1){
        selectedNodeI=selectedNodeJ=-1;
        numSteps=-1;
        CountingAlgorithm();
        win1->invalid=true;
    }
    if(button->button==SDL_BUTTON_LEFT && !draggingEntity){
        int s=SelectEntityXY(button->x*win1->sx,button->y*win1->sy);
        if(s!=selectedEntity){
            selectedEntity=s;
            numSteps=-1;
            CountingAlgorithm();
            win1->invalid=true;
        }
    }
    if(button->button==SDL_BUTTON_RIGHT && !drawingEdge){
        int s=SelectEntityXY(button->x*win1->sx,button->y*win1->sy);
        if(s==-1){
            if(CanMutateNetwork()){
                AddEntity(1,ToWorldX(win1,button->x*win1->sx*2.0f/(1.0f+separator)),ToWorldY(win1,button->y*win1->sy));
                selectedEntity=network->entities->tot-1;
                DisplayMessage("Create new agent");
                numSteps=-1;
                ComputeJob_DispatchExecuteNetwork(NULL,NULL,true);
            }
        }
        else{
            draggingEntity=true;
            if(s!=selectedEntity){
                selectedEntity=s;
                Entity *e=GetEntity(selectedEntity);
                e->x=ToWorldX(win1,button->x*win1->sx*2.0f/(1.0f+separator));
                e->y=ToWorldY(win1,button->y*win1->sy);
                if(e->x<-1.0f)e->x=-1.0f;
                if(e->y<-1.0f)e->y=-1.0f;
                if(e->x>1.0f)e->x=1.0f;
                if(e->y>1.0f)e->y=1.0f;
                numSteps=-1;
                CountingAlgorithm();
                win1->invalid=true;
            }
        }
    }
}

static void MousePressed2(SDL_MouseButtonEvent *button){ // specific to right panel
    if(selectedEntity!=-1){
        selectedEntity=-1;
        numSteps=-1;
        CountingAlgorithm();
        win1->invalid=true;
    }
    if(button->button==SDL_BUTTON_LEFT){
        int si,sj;
        SelectNodeXY(button->x*win1->sx,button->y*win1->sy,&si,&sj);
        if(si!=selectedNodeI || sj!=selectedNodeJ){
            selectedNodeI=si; selectedNodeJ=sj;
            numSteps=-1;
            CountingAlgorithm();
            if(selectedNodeI!=-1)currentRound=selectedNodeI-2;
            win1->invalid=true;
        }
    }
}

static void MousePressed3(SDL_MouseButtonEvent *button){ // specific to separator
    if(button->button==SDL_BUTTON_LEFT){
        resizing=true;
        separator=ToWorldX(win1,button->x*win1->sx);
        if(separator<-1.0f)separator=-1.0f;
        if(separator>1.0f)separator=1.0f;
        win1->invalid=true;
    }
    else if(button->button==SDL_BUTTON_RIGHT){
        resizeHover=resizing=false;
        separator=0.0f;
        win1->invalid=true;
    }
}

static void MouseReleased(SDL_MouseButtonEvent *button){
    if(button->button==SDL_BUTTON_LEFT){
        if(drawingEdge){
            drawingEdge=false;
            int mult=IsDeleteLinkModifierDown()?-1:1;
            bool allRounds=IsAllRoundsModifierDown();
            bool bothWays=IsTwoWayLinkModifierDown();
            win1->invalid=true;
            int s=SelectEntityXY(button->x*win1->sx,button->y*win1->sy);
            if(selectedEntity!=-1 && s!=-1){
                if(currentRound<0)DisplayMessage("Cannot modify links before round 1");
                else if(CanMutateNetwork()){
                    if(allRounds)
                        for(int r=0;r<network->rounds->tot;r++){
                            AddInteraction(r,selectedEntity,s,mult);
                            if(bothWays && selectedEntity!=s)AddInteraction(r,s,selectedEntity,mult);
                        }
                    else{
                        AddInteraction(currentRound,selectedEntity,s,mult);
                        if(bothWays && selectedEntity!=s)AddInteraction(currentRound,s,selectedEntity,mult);
                    }
                    if(mult>0)DisplayMessage("Create new link");
                    else DisplayMessage("Delete link");
                    RunRecompute(!allRounds && currentRound==network->rounds->tot-1,ReExecuteLastRound,NULL);
                }
            }
        }
        resizing=false;
    }
    if(button->button==SDL_BUTTON_RIGHT){
        if(draggingEntity){
            draggingEntity=false;
            Entity *e=GetEntity(selectedEntity);
            e->x=ToWorldX(win1,button->x*win1->sx*2.0f/(1.0f+separator));
            e->y=ToWorldY(win1,button->y*win1->sy);
            if(e->x<-1.0f)e->x=-1.0f;
            if(e->y<-1.0f)e->y=-1.0f;
            if(e->x>1.0f)e->x=1.0f;
            if(e->y>1.0f)e->y=1.0f;
            win1->invalid=true;
        }
    }
}

static void MouseMoved(SDL_MouseMotionEvent *motion){
    if(selectedEntity!=-1 && !drawingEdge && !resizing && (motion->state & SDL_BUTTON_LMASK)){
        int mx=motion->x*win1->sx;
        int my=motion->y*win1->sy;
        Entity *e=GetEntity(selectedEntity);
        float sx=ToScreenX1(win1,e->x);
        float sy=ToScreenY(win1,e->y);
        float dist=(sx-mx)*(sx-mx)+(sy-my)*(sy-my);
        if(dist>NODE_SIZE*NODE_SIZE*0.25f)drawingEdge=true;
    }
    if(drawingEdge)win1->invalid=true;
    if(draggingEntity){
        Entity *e=GetEntity(selectedEntity);
        e->x=ToWorldX(win1,motion->x*win1->sx*2.0f/(1.0f+separator));
        e->y=ToWorldY(win1,motion->y*win1->sy);
        if(e->x<-1.0f)e->x=-1.0f;
        if(e->y<-1.0f)e->y=-1.0f;
        if(e->x>1.0f)e->x=1.0f;
        if(e->y>1.0f)e->y=1.0f;
        win1->invalid=true;
    }
    if(resizing && (motion->state & SDL_BUTTON_LMASK)){
        separator=ToWorldX(win1,motion->x*win1->sx);
        if(separator<-1.0f)separator=-1.0f;
        if(separator>1.0f)separator=1.0f;
        win1->invalid=true;
    }
    if(resizeHover && !resizing && !CloseToSeparator(motion->x*win1->sx)){
        resizeHover=false;
        win1->invalid=true;
    }
    else if(!resizeHover && !drawingEdge && !draggingEntity && CloseToSeparator(motion->x*win1->sx)){
        resizeHover=true;
        win1->invalid=true;
    }
}

static void MouseWheel(SDL_MouseWheelEvent *wheel){
    static float wheelTot=0.0f;
    if(wheel->direction==SDL_MOUSEWHEEL_FLIPPED)wheelTot-=wheel->y;
    else wheelTot+=wheel->y;
    int w=(int)truncf(wheelTot);
    if(w){
        wheelTot-=(float)w;
        while(w<0){IncrementCurrentRound(); w++;}
        while(w>0){DecrementCurrentRound(); w--;}
    }
}

void Events(void){
    SDL_Event e;
    SDL_Window *win;
    while(SDL_PollEvent(&e)){
        switch(e.type){
            case SDL_EVENT_QUIT:
                quit=true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                quit=true;
                break;
            #ifdef __EMSCRIPTEN__
            case SDL_EVENT_WINDOW_RESIZED:
                win1->invalid=true;
                RenderWindow(win1);
                break;
            #else
            case SDL_EVENT_KEY_DOWN:
                if(helping){
                    helping=false;
                    win1->invalid=true;
                    break;
                }
                KeyPressed(e.key.key);
                break;
            case SDL_EVENT_KEY_UP:
                if(helping)break;
                KeyReleased(e.key.key);
                break;
            #endif
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(helping){
                    helping=false;
                    win1->invalid=true;
                    break;
                }
                win=SDL_GetWindowFromID(e.button.windowID);
                if(win!=win1->window)break;
                if(resizeHover)MousePressed3(&e.button);
                else if(e.button.x*win1->sx<SeparatorX())MousePressed1(&e.button);
                else MousePressed2(&e.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(helping)break;
                win=SDL_GetWindowFromID(e.button.windowID);
                if(win!=win1->window)break;
                MouseReleased(&e.button);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if(helping)break;
                win=SDL_GetWindowFromID(e.motion.windowID);
                if(win!=win1->window)break;
                MouseMoved(&e.motion);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if(helping)break;
                win=SDL_GetWindowFromID(e.wheel.windowID);
                if(win!=win1->window)break;
                MouseWheel(&e.wheel);
                break;
            #ifndef __EMSCRIPTEN__
            case SDL_EVENT_LOAD_NETWORK:
                LoadNetworkRun(e.user.data1);
                break;
            case SDL_EVENT_SAVE_NETWORK:
                SaveNetworkRun(e.user.data1);
                break;
            #endif
            default:
                break;
        }
    }
    if(capsPressed){
        if(IsCapsTwoWayDefault())DisplayMessage("Create two-way links by default");
        else DisplayMessage("Create one-way links by default");
        capsPressed=false;
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void TestInsertRound(void){
    KeyPressed(SDLK_EQUALS);
}

EMSCRIPTEN_KEEPALIVE void TestDeleteRound(void){
    KeyPressed(SDLK_MINUS);
}
#endif

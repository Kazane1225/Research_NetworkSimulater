#include "main.h"

static const char *helpMessage[]={
    "MOUSE COMMANDS",
    "",
    "Left-click: select/deselect agent or Vista.",
    "Mouse wheel up/down: decrement/increment current round.",
    "History Tree: drag (or minimap) to scroll, CTRL + wheel to zoom.",
    "",
    "NETWORK PANEL",
    "",
    "Left-click on an agent and drag to draw a new link; release mouse button to confirm.",
    "Hold CTRL or Q while creating a link to delete the link instead.",
    "Hold SHIFT while creating/deleting a link to make it two-way.",
    "Hold ALT while creating/deleting a link to affect all rounds.",
    "Right-click on an agent and drag to move it.",
    "Right-click away from agents to create a new agent.",
    "",
    "HOTKEYS",
    "",
    "TAB: select counting algorithm.",
    "SPACE: execute counting algorithm step by step.",
    "ESC: deselect agents and Vista.",
    "UP/DOWN: decrement/increment current round.",
    "LEFT/RIGHT: select different distinguishable class.",
    "BACKSPACE: delete all links in current round.",
    "DEL or U: delete selected agents.",
    "+/-: insert/delete round.",
    "O: toggle outdegree awareness of agents.",
    "A: toggle arrowsheads.",
    "D: change red edge and outdegree drawing mode.",
    "B: draw round/square nodes.",
    "C: toggle highlight current level.",
    "G: snap agents to grid.",
    "Z/X: zoom history tree in/out. F: fit whole history tree.",
    "H: show command list.",
    "0..9: change input of selected agents; 0 is leader.",
    "CAPS-LOCK: create two-way links by default.",
    "L: load network.",
    "S: save network.",
    "",
    "See History Trees and Their Applications (arxiv.org/abs/2404.02673)."
};

#define TREE_AUTO_MAX_RADIUS (NODE_SIZE*0.5f)
#define TREE_ZOOM_MAX_RADIUS (NODE_SIZE*1.2f)
#define TREE_AUTO_MIN_RADIUS 11.0f
#define TREE_GAP_X 0.38f // max node radius / horizontal leaf spacing
#define TREE_GAP_Y 0.18f // max node radius / vertical level spacing; leaves room for two edge badges
#define TREE_LABEL_MIN_RADIUS 6.0f
#define TREE_BADGE_MIN_RADIUS 5.0f
#define MINIMAP_SIZE (NODE_SIZE*4.0f)
#define MINIMAP_MIN_SIZE 48.0f
#define MINIMAP_MARGIN 12.0f

bool roundNodes=false;
int renderLinks=0;
bool renderBar=true;
bool renderArrows=true;
bool renderMessage=false;
char infoMessage[512];

float treeTargetRadius=0.0f; // 0: automatic (readable minimum), <0: fit whole tree, >0: user zoom
static float treeCenterX=0.0f,treeCenterY=0.0f; // view center in tree coordinates
static float treeZoomX=1.0f,treeZoomY=1.0f;
static float treeRadius=TREE_AUTO_MAX_RADIUS; // node radius in pixels
static bool minimapVisible=false;
static float minimapX,minimapY,minimapW,minimapH; // pixels

static float Patch(float x,bool left){
    return left?(1.0f+x)*(separator-1.0f)*0.5f+x:(1.0f-x)*(separator+1.0f)*0.5f+x;
}

static void SetLinkColor(void){
    if(uiTheme)SetColor(207,34,46);
    else SetColor(255,0,0);
}

static void SetRedEdgeColor(bool active){
    if(!uiTheme)SetColor(255,112,112);
    else if(active)SetColor(225,80,80);
    else SetColor(244,196,196);
}

static float TreePanelWidth(void){
    float pw=win1->w-SeparatorX();
    return pw<1.0f?1.0f:pw;
}

static float TreeFitRadius(float hs,float vs,float cap){
    return fminf(cap,fminf(TREE_GAP_X*hs,TREE_GAP_Y*vs));
}

static void ClampTreeCenter(void){
    float mx=1.0f-1.0f/treeZoomX,my=1.0f-1.0f/treeZoomY;
    if(treeCenterX<-mx)treeCenterX=-mx;
    if(treeCenterX>mx)treeCenterX=mx;
    if(treeCenterY<-my)treeCenterY=-my;
    if(treeCenterY>my)treeCenterY=my;
}

void UpdateTreeView(void){
    treeZoomX=treeZoomY=1.0f;
    treeRadius=TREE_AUTO_MAX_RADIUS;
    if(!aux || !aux->tot || !win1->h){ ClampTreeCenter(); return; }
    float hs=TreePanelWidth()/GetAuxData(0,0)->width;
    float vs=(float)win1->h/aux->tot;
    float target=treeTargetRadius==0.0f?TREE_AUTO_MIN_RADIUS:treeTargetRadius;
    float cap=fmaxf(TREE_AUTO_MAX_RADIUS,target);
    if(target>cap)target=cap;
    if(target>TREE_GAP_X*hs)treeZoomX=target/(TREE_GAP_X*hs);
    if(target>TREE_GAP_Y*vs)treeZoomY=target/(TREE_GAP_Y*vs);
    treeRadius=TreeFitRadius(hs*treeZoomX,vs*treeZoomY,cap);
    ClampTreeCenter();
}

float TreeNodeRadius(void){
    return treeRadius;
}

static float TreeViewX(float x){
    return (x-treeCenterX)*treeZoomX;
}

static float TreeViewY(float y){
    return (y-treeCenterY)*treeZoomY;
}

static float TX(float x){
    return Patch(TreeViewX(x),false);
}

static float TY(float y){
    return TreeViewY(y);
}

float TreeScreenX(float x){
    return ToScreenX2(win1,TreeViewX(x));
}

float TreeScreenY(float y){
    return ToScreenY(win1,TreeViewY(y));
}

void PanTreeView(float dx,float dy){
    UpdateTreeView();
    treeCenterX-=dx*2.0f/(TreePanelWidth()*treeZoomX);
    treeCenterY-=dy*2.0f/(win1->h*treeZoomY);
    ClampTreeCenter();
    win1->invalid=true;
}

void ZoomTreeView(float factor,float sx,float sy){
    if(!aux || !aux->tot)return;
    UpdateTreeView();
    float vx=(sx-SeparatorX())/TreePanelWidth()*2.0f-1.0f;
    float vy=sy/win1->h*2.0f-1.0f;
    float tx=treeCenterX+vx/treeZoomX;
    float ty=treeCenterY+vy/treeZoomY;
    float hs=TreePanelWidth()/GetAuxData(0,0)->width;
    float vs=(float)win1->h/aux->tot;
    float r=treeRadius*factor;
    if(factor<1.0f && r<=TreeFitRadius(hs,vs,TREE_AUTO_MAX_RADIUS)*1.001f)treeTargetRadius=-1.0f;
    else treeTargetRadius=fminf(r,TREE_ZOOM_MAX_RADIUS);
    UpdateTreeView();
    treeCenterX=tx-vx/treeZoomX;
    treeCenterY=ty-vy/treeZoomY;
    ClampTreeCenter();
    win1->invalid=true;
}

void ZoomTreeViewCentered(float factor){
    ZoomTreeView(factor,SeparatorX()+TreePanelWidth()*0.5f,win1->h*0.5f);
}

bool ToggleTreeFit(void){
    treeTargetRadius=treeTargetRadius<0.0f?0.0f:-1.0f;
    UpdateTreeView();
    win1->invalid=true;
    return treeTargetRadius<0.0f;
}

bool TreeMinimapContains(float sx,float sy){
    return minimapVisible && sx>=minimapX && sx<=minimapX+minimapW && sy>=minimapY && sy<=minimapY+minimapH;
}

void TreeMinimapMoveTo(float sx,float sy){
    if(!minimapVisible)return;
    treeCenterX=(sx-minimapX)/minimapW*2.0f-1.0f;
    treeCenterY=(sy-minimapY)/minimapH*2.0f-1.0f;
    ClampTreeCenter();
    win1->invalid=true;
}

static void EnsureTreePointVisible(float x,float y,bool horizontal){
    float mx=fminf(treeRadius*2.5f/(TreePanelWidth()*0.5f),0.5f);
    float my=fminf(4.0f*treeZoomY/aux->tot+treeRadius*2.0f/win1->h,0.5f); // keep two neighbouring levels in view
    float vy=TreeViewY(y);
    if(vy<-1.0f+my)treeCenterY+=(vy+1.0f-my)/treeZoomY;
    else if(vy>1.0f-my)treeCenterY+=(vy-1.0f+my)/treeZoomY;
    if(horizontal){
        float vx=TreeViewX(x);
        if(vx<-1.0f+mx)treeCenterX+=(vx+1.0f-mx)/treeZoomX;
        else if(vx>1.0f-mx)treeCenterX+=(vx-1.0f+mx)/treeZoomX;
    }
    ClampTreeCenter();
}

static void FollowTreeFocus(void){
    static int lastLevel=-1000,lastI=-1000,lastJ=-1000,lastEntity=-1000,lastTot=-1;
    int level=currentRound+2;
    if(level<0)level=0;
    if(level>=aux->tot)level=aux->tot-1;
    if(level==lastLevel && selectedNodeI==lastI && selectedNodeJ==lastJ && selectedEntity==lastEntity && aux->tot==lastTot)return;
    lastLevel=level; lastI=selectedNodeI; lastJ=selectedNodeJ; lastEntity=selectedEntity; lastTot=aux->tot;
    float y=GetAuxData(level,0)->y;
    if(selectedNodeI>=0 && selectedNodeI<aux->tot && selectedNodeJ>=0 && selectedNodeJ<GetLevel(selectedNodeI)->tot){
        AuxData *data=GetAuxData(selectedNodeI,selectedNodeJ);
        EnsureTreePointVisible(data->x,data->y,true);
    }
    else if(selectedEntity>=0 && selectedEntity<network->entities->tot){
        HistoryTree *h=GetEntity(selectedEntity)->finalLeaf;
        for(int i=h->level+1;i>level && h->parent;i--)h=h->parent;
        AuxData *data=h->data;
        if(data)EnsureTreePointVisible(data->x,y,true);
        else EnsureTreePointVisible(0.0f,y,false);
    }
    else EnsureTreePointVisible(0.0f,y,false);
}

static void RenderNetwork(WindowData *win,float rx,float ry){
    bool selfLoopOfs=false;
    if(currentRound>=0){
        Vector *v=network->rounds->items[currentRound];
        SetWidth(5.0f);
        SetLinkColor();
        for(int i=0;i<v->tot;i++){
            Interaction *interaction=v->items[i];
            Entity *e1=interaction->e1;
            Entity *e2=interaction->e2;
            if(e1==e2)DrawSelfArrow(win,e1->x,e1->y,rx,ry,0.0f,renderArrows,DISCRETIZE_RENDERING);
            else DrawArrow(win,e1->x,e1->y,e2->x,e2->y,renderArrows,true,false,DISCRETIZE_RENDERING);
        }
        SetWidth(2.0f);
        PrepareRenderText();
        SetFontProperties(0.35f,0.0f,FONT_EDGE*5);
        SetFontSize(ry*1.5f*LINK_LABEL_SIZE);
        PrepareRenderLine();
        for(int i=0;i<v->tot;i++){
            Interaction *interaction=v->items[i];
            Entity *e1=interaction->e1;
            Entity *e2=interaction->e2;
            if(e1==e2){
                float x=e1->x;
                float y=e1->y;
                float dist=sqrt(x*x+y*y);
                float cx,cy;
                if(dist>0.0f){ cx=x/dist; cy=y/dist; }
                else{ cx=0.0f; cy=-1.0f; }
                x=Patch(x,true);
                cx*=rx*SELF_LOOP_RADIUS; cy*=ry*SELF_LOOP_RADIUS;
                x+=2.0f*cx; y+=2.0f*cy;
                if(interaction->multiplicity>1)DrawLabel(win,x,y,x,y,interaction->multiplicity,LINK_LABEL_POSITION,rx*LINK_LABEL_SIZE,ry*LINK_LABEL_SIZE,true,true,DISCRETIZE_RENDERING);
                if(drawingEdge && GetEntityIndex(e1)==selectedEntity)selfLoopOfs=true;
            }
            else if(interaction->multiplicity>1)DrawLabel(win,Patch(e1->x,true),e1->y,Patch(e2->x,true),e2->y,interaction->multiplicity,LINK_LABEL_POSITION,rx*LINK_LABEL_SIZE,ry*LINK_LABEL_SIZE,true,true,DISCRETIZE_RENDERING);
        }
    }
    if(selectedEntity!=-1 && drawingEdge){
        Entity *e1=GetEntity(selectedEntity);
        float x,y;
        SDL_GetMouseState(&x,&y);
        x*=win->sx; y*=win->sy;
        SetWidth(5.0f);
        SetLinkColor();
        int s=SelectEntityXY(x,y);
        if(s==selectedEntity)DrawSelfArrow(win,e1->x,e1->y,rx,ry,selfLoopOfs?0.55f:0.0f,true,DISCRETIZE_RENDERING);
        else{
            bool bothWays=IsTwoWayLinkModifierDown();
            if(s!=-1){
                Entity *e2=GetEntity(s);
                DrawArrow(win,e1->x,e1->y,e2->x,e2->y,true,true,bothWays,DISCRETIZE_RENDERING);
            }
            else DrawArrow(win,e1->x,e1->y,ToWorldX(win,x*2.0f/(1.0f+separator)),ToWorldY(win,y),true,false,bothWays,DISCRETIZE_RENDERING);
        }
    }
    SetWidth(3.0f);
    PrepareRenderText();
    SetFontProperties(0.35f,0.0f,FONT_EDGE*5);
    SetFontSize(ry*1.5f);
    SetFontOutlineColor(0.0f,0.0f,0.0f);
    SetFontColor(0.0f,0.0f,0.0f,1.0f);
    PrepareRenderLine();
    for(int i=network->entities->tot-1;i>=0;i--){
        Entity *e=GetEntity(i);
        if(i==selectedEntity || CorrespondsToSelectedNode(e)){
            if(uiTheme)SetColor(84,174,255);
            else SetColor(88,166,255);
        }
        else if(uiTheme)SetColor(255,255,255);
        else SetColor(230,237,243);
        DrawEllipse(win,Patch(e->x,true),e->y,rx,ry,true,2,DISCRETIZE_RENDERING);
        if(currentRound<-1)continue;
        if(!e->input)DrawEllipse(win,Patch(e->x,true),e->y,rx*LINK_LABEL_SIZE,ry*LINK_LABEL_SIZE,false,2,DISCRETIZE_RENDERING);
        PrepareRenderText();
        switch(e->input){
            case 0: PrintString(Patch(e->x,true)*win1->aspect+rx*LABEL_OFS_X,e->y+ry*LABEL_OFS_Y,true,"L"); break;
            case 1: PrintString(Patch(e->x,true)*win1->aspect+rx*LABEL_OFS_X1,e->y+ry*LABEL_OFS_Y,true,"1"); break;
            default: PrintString(Patch(e->x,true)*win1->aspect+rx*LABEL_OFS_X,e->y+ry*LABEL_OFS_Y,true,"%d",e->input); break;
        }
        PrepareRenderLine();
    }
}

static float ScaledWidth(float base,float minimum){
    float w=base*treeRadius/TREE_AUTO_MAX_RADIUS;
    if(w>base)w=base;
    return w<minimum?minimum:w;
}

static float EdgeLengthPx(AuxData *a,AuxData *b){
    float dx=TreeScreenX(b->x)-TreeScreenX(a->x);
    float dy=TreeScreenY(b->y)-TreeScreenY(a->y);
    return sqrtf(dx*dx+dy*dy);
}

// Multiplicity badges hug the child; outdegree badges sit just beyond them, so the
// two never collide when a red edge runs along a black edge.
static float RedLabelPosition(AuxData *data,AuxData *obs,float labelRadius){
    float len=EdgeLengthPx(data,obs);
    if(len<EPSILON)return RED_EDGE_LABEL_POSITION;
    float t=(treeRadius+labelRadius+2.0f)/len;
    if(t<0.15f)t=0.15f;
    if(t>0.5f)t=0.5f;
    return t;
}

static float OutdegreeLabelPosition(AuxData *data,AuxData *parent,float labelRadius){
    float len=EdgeLengthPx(data,parent);
    if(len<EPSILON)return BLACK_EDGE_LABEL_POSITION;
    float t=(treeRadius+2.0f*treeRadius*RED_EDGE_LABEL_SIZE+labelRadius+4.0f)/len;
    float tmax=1.0f-(treeRadius+labelRadius+2.0f)/len;
    if(t>tmax)t=tmax;
    if(t<0.5f)t=0.5f;
    return t;
}

static void RenderHistoryTree(WindowData *win,float rx,float ry){
    FollowTreeFocus();
    bool nodeLabels=treeRadius>=TREE_LABEL_MIN_RADIUS;
    bool outdegreeLabels=treeRadius*BLACK_EDGE_LABEL_SIZE>=TREE_BADGE_MIN_RADIUS;
    bool multiplicityLabels=treeRadius*RED_EDGE_LABEL_SIZE>=TREE_BADGE_MIN_RADIUS;
    int level=currentRound+2;
    if(renderBar && level>=0 && level<aux->tot){ // render gray bar on current level
        if(uiTheme)SetColor(218,232,252);
        else SetColor(22,27,34);
        AuxData *data=GetAuxData(level,0);
        DrawRectangle(win,Patch(0,false),TY(data->y),-Patch(0,true),fminf(ry*1.4f,treeZoomY/aux->tot),true,false,false);
    }
    for(int i=1;i<aux->tot;i++) // render black edges
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            AuxData *parent=GetAuxData(i-1,data->parent);
            if(!selectedNode || data->visible){
                if(uiTheme)SetColor(87,96,106);
                else SetColor(139,148,158);
            }
            else if(uiTheme)SetColor(208,215,222);
            else SetColor(72,79,88);
            SetWidth(!selectedNode || data->visible?ScaledWidth(8.0f,2.0f):ScaledWidth(3.0f,1.0f));
            DrawLine(win,TX(data->x),TY(data->y),TX(parent->x),TY(parent->y),DISCRETIZE_RENDERING);
        }
    if(renderLinks<2){
        for(int i=0;i<aux->tot;i++) // render red edges
            for(int j=0;j<GetLevel(i)->tot;j++){
                AuxData *data=GetAuxData(i,j);
                if(renderLinks && !data->visible)continue;
                SetRedEdgeColor(!selectedNode || data->visible);
                SetWidth(!selectedNode || data->visible?ScaledWidth(4.0f,1.5f):ScaledWidth(1.5f,1.0f));
                for(int k=0;k<data->observations->tot;k++){
                    AuxData *obs=GetAuxData(i-1,data->observations->items[k]);
                    DrawLine(win,TX(data->x),TY(data->y),TX(obs->x),TY(obs->y),DISCRETIZE_RENDERING);
                }
            }
    }
    PrepareRenderText();
    SetFontProperties(0.35f,0.0f,FONT_EDGE*5);
    SetFontSize(ry*1.5f);
    PrepareRenderLine();
    for(int i=aux->tot-1;i>=0;i--) // render nodes
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            if(data->visible){
                if(algorithm==1){
                    if(data->guess!=-1){
                        if(data->guess==data->anonymity)SetColor(86,211,100);
                        else SetColor(255,112,112);
                    }
                    else SetColor(227,179,65);
                }
                else if(algorithm==2){
                    if(data->i<=1 && data->guess!=-1)SetColor(57,216,245);
                    else if(data->counted)SetColor(86,211,100);
                    else if(data->guess!=-1)SetColor(255,166,87);
                    else SetColor(227,179,65);
                }
                else SetColor(227,179,65);
            }
            else if(uiTheme){
                if(selectedNode)SetColor(238,241,244);
                else SetColor(255,255,255);
            }
            else SetColor(48,54,61);
            SetWidth(ScaledWidth(3.0f,1.0f));
            int border=!selectedNode || data->visible?2:1;
            float x=TX(data->x),y=TY(data->y);
            if(roundNodes){
                DrawEllipse(win,x,y,rx,ry,true,border,DISCRETIZE_RENDERING);
                if(!data->h->input)DrawEllipse(win,x,y,rx*0.8f,ry*0.8f,false,border,DISCRETIZE_RENDERING);
            }
            else{
                DrawRectangle(win,x,y,rx,ry,true,border,DISCRETIZE_RENDERING);
                if(!data->h->input)DrawRectangle(win,x,y,rx*0.8f,ry*0.8f,false,border,DISCRETIZE_RENDERING);
            }
            if(!nodeLabels)continue;
            int label=-1;
            if(algorithm && (selectedEntity!=-1 || selectedNodeI!=-1))label=data->guess;
            else if(data->i==1){
                if(data->h->input)label=data->h->input;
                else label=0;
            }
            if(label>=0){
                PrepareRenderText();
                if(border==1){
                    SetFontOutlineColor(0.5f,0.5f,0.5f);
                    SetFontColor(0.5f,0.5f,0.5f,1.0f);
                }
                else{
                    SetFontOutlineColor(0.0f,0.0f,0.0f);
                    SetFontColor(0.0f,0.0f,0.0f,1.0f);
                }
                switch(label){
                    case 0: PrintString(x*win1->aspect+rx*LABEL_OFS_X,y+ry*LABEL_OFS_Y,true,"L"); break;
                    case 1: PrintString(x*win1->aspect+rx*LABEL_OFS_X1,y+ry*LABEL_OFS_Y,true,"1"); break;
                    default:
                        if(label>=10 && label<20)PrintString(x*win1->aspect+rx*LABEL_OFS_X1,y+ry*LABEL_OFS_Y,true,"%d",label);
                        else PrintString(x*win1->aspect+rx*LABEL_OFS_X,y+ry*LABEL_OFS_Y,true,"%d",label);
                        break;
                }
                PrepareRenderLine();
            }
        }
    // Edge labels after nodes so multiplicity / outdegree badges stay readable.
    if(renderLinks<2){
        SetWidth(ScaledWidth(2.0f,1.0f));
        PrepareRenderText();
        SetFontProperties(0.35f,0.0f,FONT_EDGE*5);
        SetFontSize(ry*1.5f*BLACK_EDGE_LABEL_SIZE);
        PrepareRenderLine();
        for(int i=aux->tot-1;i>1 && outdegreeLabels;i--) // render outdegrees
            for(int j=0;j<GetLevel(i)->tot;j++){
                AuxData *data=GetAuxData(i,j);
                if(data->outdegree<0)continue;
                if(renderLinks && !data->visible)continue;
                AuxData *parent=GetAuxData(i-1,data->parent);
                float t=OutdegreeLabelPosition(data,parent,treeRadius*BLACK_EDGE_LABEL_SIZE);
                DrawLabel(win,TX(data->x),TY(data->y),TX(parent->x),TY(parent->y),data->outdegree,t,rx*BLACK_EDGE_LABEL_SIZE,ry*BLACK_EDGE_LABEL_SIZE,!selectedNode || data->visible,false,DISCRETIZE_RENDERING);
            }
        PrepareRenderText();
        SetFontSize(ry*1.5f*RED_EDGE_LABEL_SIZE);
        PrepareRenderLine();
        for(int i=aux->tot-1;i>=0 && multiplicityLabels;i--) // render multiplicities
            for(int j=0;j<GetLevel(i)->tot;j++){
                AuxData *data=GetAuxData(i,j);
                if(renderLinks && !data->visible)continue;
                for(int k=0;k<data->observations->tot;k++){
                    Observation *obs1=data->h->observations->items[k];
                    if(obs1->multiplicity<=1)continue;
                    AuxData *obs2=GetAuxData(i-1,data->observations->items[k]);
                    float t=RedLabelPosition(data,obs2,treeRadius*RED_EDGE_LABEL_SIZE);
                    DrawLabel(win,TX(data->x),TY(data->y),TX(obs2->x),TY(obs2->y),obs1->multiplicity,t,rx*RED_EDGE_LABEL_SIZE,ry*RED_EDGE_LABEL_SIZE,!selectedNode || data->visible,true,DISCRETIZE_RENDERING);
                }
            }
    }
    PrepareRenderLine();
}

static void RenderHelp(void){
    SetFontProperties(0.35f,0.0f,FONT_EDGE*5);
    SetFontSize(FONT_SIZE*1.0f);
    if(uiTheme){
        SetFontOutlineColor(0.957f,0.965f,0.973f);
        SetFontColor(0.122f,0.137f,0.157f,1.0f);
    }
    else{
        SetFontOutlineColor(0.051f,0.067f,0.090f);
        SetFontColor(0.784f,0.820f,0.851f,1.0f);
    }

    int lines=sizeof(helpMessage)/sizeof(helpMessage[0]);
    for(int i=0;i<lines;i++)PrintString(-win1->aspect+0.05f,-1.0f+0.05f*(i+1)+((i==0||i==6||i==15||i==37)?0.025f:0.0f),false,"%s",helpMessage[i]);
}

static void ClipOff(void){
    glDisable(GL_SCISSOR_TEST);
}

static void ClipRectPx(int x,int y,int w,int h){
    if(w<=0 || h<=0){
        glEnable(GL_SCISSOR_TEST);
        glScissor(0,0,0,0);
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(x,y,w,h);
}

static float MinimapX(float x){
    return (minimapX+(x+1.0f)*0.5f*minimapW)*2.0f/win1->w-1.0f;
}

static float MinimapY(float y){
    return (minimapY+(y+1.0f)*0.5f*minimapH)*2.0f/win1->h-1.0f;
}

static void MinimapFrame(float x1,float y1,float x2,float y2){
    DrawLine(win1,MinimapX(x1),MinimapY(y1),MinimapX(x2),MinimapY(y1),false);
    DrawLine(win1,MinimapX(x2),MinimapY(y1),MinimapX(x2),MinimapY(y2),false);
    DrawLine(win1,MinimapX(x2),MinimapY(y2),MinimapX(x1),MinimapY(y2),false);
    DrawLine(win1,MinimapX(x1),MinimapY(y2),MinimapX(x1),MinimapY(y1),false);
}

static void RenderTreeMinimap(WindowData *win){
    minimapVisible=false;
    if(treeZoomX<=1.001f && treeZoomY<=1.001f)return;
    float pw=TreePanelWidth();
    float cw=pw*treeZoomX,ch=win->h*treeZoomY;
    float maxW=fminf(MINIMAP_SIZE,pw*0.35f),maxH=fminf(MINIMAP_SIZE,win->h*0.45f);
    if(maxW<MINIMAP_MIN_SIZE || maxH<MINIMAP_MIN_SIZE)return;
    float s=fminf(maxW/cw,maxH/ch);
    minimapW=fmaxf(cw*s,MINIMAP_MIN_SIZE);
    minimapH=fmaxf(ch*s,MINIMAP_MIN_SIZE);
    minimapX=SeparatorX()+MINIMAP_MARGIN;
    minimapY=MINIMAP_MARGIN;
    minimapVisible=true;
    PrepareRenderLine();
    ClipRectPx((int)minimapX-2,(int)(win->h-minimapY-minimapH)-2,(int)minimapW+4,(int)minimapH+4);
    if(uiTheme)SetColor(255,255,255);
    else SetColor(22,27,34);
    DrawRectangle(win,MinimapX(0.0f),MinimapY(0.0f),minimapW/win->w,minimapH/win->h,true,0,false);
    int level=currentRound+2;
    if(level>=0 && level<aux->tot){
        SetColor(227,179,65);
        SetWidth(1.0f);
        float y=GetAuxData(level,0)->y;
        DrawLine(win,MinimapX(-1.0f),MinimapY(y),MinimapX(1.0f),MinimapY(y),false);
    }
    SetWidth(1.0f);
    for(int i=1;i<aux->tot;i++)
        for(int j=0;j<GetLevel(i)->tot;j++){
            AuxData *data=GetAuxData(i,j);
            AuxData *parent=GetAuxData(i-1,data->parent);
            if(selectedNode && data->visible){
                if(uiTheme)SetColor(9,105,218);
                else SetColor(88,166,255);
            }
            else if(uiTheme)SetColor(101,109,118);
            else SetColor(139,148,158);
            DrawLine(win,MinimapX(data->x),MinimapY(data->y),MinimapX(parent->x),MinimapY(parent->y),false);
        }
    if(uiTheme)SetColor(208,215,222);
    else SetColor(48,54,61);
    SetWidth(1.0f);
    MinimapFrame(-1.0f,-1.0f,1.0f,1.0f);
    if(uiTheme)SetColor(9,105,218);
    else SetColor(88,166,255);
    SetWidth(2.0f);
    MinimapFrame(treeCenterX-1.0f/treeZoomX,treeCenterY-1.0f/treeZoomY,treeCenterX+1.0f/treeZoomX,treeCenterY+1.0f/treeZoomY);
}

void RenderWindow1(WindowData *win){
    int w=win->w;
    int h=win->h;
    if(!w || !h)return;
    float rx=NODE_SIZE/w;
    float ry=NODE_SIZE/h;
    PrepareRenderText();
    BindFont(&font);
    GL_Uniform1f(unifTextAspect,win1->aspect2);
    if(helping){
        RenderHelp();
        return;
    }
    PrepareRenderLine();
    if(uiTheme)SetColor(208,215,222);
    else SetColor(48,54,61);
    SetWidth(1.0f);
    if(SeparatorX()>1.0f){
        ClipRectPx(0,0,SeparatorX(),h);
        RenderNetwork(win,rx,ry);
        ClipOff();
    }
    UpdateTreeView();
    rx=2.0f*treeRadius/w;
    ry=2.0f*treeRadius/h;
    minimapVisible=false;
    if(SeparatorX()<w-2.0f){
        ClipRectPx(SeparatorX(),0,w-SeparatorX(),h);
        RenderHistoryTree(win,rx,ry);
        RenderTreeMinimap(win);
        ClipOff();
    }
    if(resizeHover){
        if(uiTheme)SetColor(101,109,118);
        else SetColor(139,148,158);
        SetWidth(5.0f);
    }
    else{
        if(uiTheme)SetColor(208,215,222);
        else SetColor(48,54,61);
        SetWidth(1.0f);
    }
    DrawLine(win1,separator,-1.0f,separator,1.0f,true);
    PrepareRenderText();
    SetFontProperties(0.15f,0.4f,FONT_EDGE*4);
    SetFontSize(FONT_SIZE*1.0f);
    if(uiTheme)SetFontOutlineColor(0.957f,0.965f,0.973f);
    else SetFontOutlineColor(0.0f,0.0f,0.0f);
    if(network){
        if(uiTheme)SetFontColor(0.259f,0.290f,0.325f,1.0f);
        else SetFontColor(0.545f,0.580f,0.620f,1.0f);
        PrintString(-win->aspect+0.025f,-1.0f+0.01f,false,"Round %d of %d",currentRound+1,network->rounds->tot);
        if(network->entities->tot==1)PrintString(-win->aspect+0.025f,-1.0f+0.07f,false,"1 agent");
        else PrintString(-win->aspect+0.025f,-1.0f+0.07f,false,"%d agents",network->entities->tot);

    }
    if(renderMessage){
        if(uiTheme)SetFontColor(0.035f,0.412f,0.855f,1.0f);
        else SetFontColor(0.0f,0.8f,0.8f,1.0f);
        PrintString(-win->aspect+0.025f,1.0f-0.075f,false,"%s",infoMessage);
    }
}

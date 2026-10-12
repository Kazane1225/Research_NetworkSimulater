extern bool roundNodes;
extern int renderLinks;
extern bool renderBar;
extern bool renderArrows;
extern bool renderMessage;
extern char infoMessage[512];
extern float treeTargetRadius;

void UpdateTreeView(void);
float TreeNodeRadius(void);
float TreeScreenX(float x);
float TreeScreenY(float y);
void PanTreeView(float dx,float dy);
void ZoomTreeView(float factor,float sx,float sy);
void ZoomTreeViewCentered(float factor);
bool ToggleTreeFit(void);
bool TreeMinimapContains(float sx,float sy);
void TreeMinimapMoveTo(float sx,float sy);
void RenderWindow1(WindowData *win);

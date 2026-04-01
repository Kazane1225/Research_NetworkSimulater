extern const bool *keyboardState;
extern bool resizeHover,resizing,helping;

bool IsDeleteLinkModifierDown(void);
bool IsAllRoundsModifierDown(void);
bool IsTwoWayLinkModifierDown(void);
bool IsCapsTwoWayDefault(void);

void FlushEvents(void);
#ifdef __EMSCRIPTEN__
EM_BOOL KeyPressedCallback(int eventType,const EmscriptenKeyboardEvent *e,void *userData);
EM_BOOL KeyReleasedCallback(int eventType,const EmscriptenKeyboardEvent *e,void *userData);
#endif
void Events(void);

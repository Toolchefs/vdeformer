
#include <lwmodule.h>


extern void *        Startup (void);
extern void        Shutdown (void *serverData);
extern "C" ServerRecord     ServerDesc[];

 
 #ifdef __BORLANDC__
ModuleDescriptor mod_descrip =
 #else
ModuleDescriptor _mod_descrip =
 #endif
 {
    MOD_SYSSYNC,
    MOD_SYSVER,
    MOD_MACHINE,
    Startup,
    Shutdown,
    ServerDesc
 };

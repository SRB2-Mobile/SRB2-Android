#include "SDL.h"
#include "SDL_main.h"
#include "SDL_config.h"

#include "../../doomdef.h"
#include "../../d_main.h"
#include "../../m_argv.h"
#include "../../i_system.h"

#include <jni_android.h>

int main(int argc, char* argv[])
{
    myargc = argc;
    myargv = argv;

    JNI_Startup();

    CONS_Printf("Setting up SRB2...\n");
    D_SRB2Main();

    CONS_Printf("Entering main game loop...\n");
    D_SRB2Loop();

    return 0;
}

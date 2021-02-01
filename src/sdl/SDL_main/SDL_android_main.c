#include "SDL.h"
#include "SDL_main.h"
#include "SDL_config.h"

#include "../../doomdef.h"
#include "../../d_main.h"
#include "../../m_argv.h"
#include "../../i_system.h"

#ifdef SPLASH_SCREEN
#include "../../i_video.h"
#include "../ogl_es_sdl.h"
#endif

#include <jni_android.h>

#ifdef SPLASH_SCREEN
static INT32 displayingSplash = 0;

static void ShowSplashScreen(void)
{
	displayingSplash = VID_LoadSplashScreen();

	if (displayingSplash)
	{
		// Present it for a single second.
		UINT32 delay = SDL_GetTicks() + 1000;

		VID_BlitSplashScreen();

		while (SDL_GetTicks() < delay)
			VID_PresentSplashScreen();
	}
}
#endif

#define REQUEST_STORAGE_PERMISSION

#define REQUEST_MESSAGE_TITLE "Storage access request"
#define REQUEST_MESSAGE_TEXT "Sonic Robo Blast 2 needs permission to store game data."

#ifdef REQUEST_STORAGE_PERMISSION
static void PolitelyRequestPermission(void)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, REQUEST_MESSAGE_TITLE, REQUEST_MESSAGE_TEXT, NULL);
}
#else
static boolean PolitelyOpenAppSettings(void)
{
	int id = 0;

	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Open app settings"},
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Ignore"},
	};

	const SDL_MessageBoxData mbox = {
		SDL_MESSAGEBOX_INFORMATION, NULL,
		REQUEST_MESSAGE_TITLE, REQUEST_MESSAGE_TEXT,
		2, buttons, NULL
	};

	if (SDL_ShowMessageBox(&mbox, &id) < 0)
		return false; // Couldn't show the message box

	if (id <= 0)
		return false; // Didn't open the app settings

	return (I_OpenAppSettings() > 0);
}
#endif

static boolean StorageInit(void)
{
	JNI_SharedStorage = I_SharedStorageLocation();
	return (JNI_SharedStorage != NULL);
}

static void StorageGrantedPermission(void)
{
	if (JNI_SharedStorage)
	{
		JNI_DisplayToast("Storage permission granted");
		I_mkdir(JNI_SharedStorage, 0755);
	}
}

static boolean StorageCheckPermission(void)
{
	const char *permission = "android.permission.WRITE_EXTERNAL_STORAGE";

	if (JNI_SharedStorage == NULL)
		return false;

	if (I_CheckSystemPermission(permission))
		return true; // Permission was already granted.

#ifdef REQUEST_STORAGE_PERMISSION
	PolitelyRequestPermission();
	if (I_RequestSystemPermission(permission))
	{
		// Permission granted -- create the directory.
		StorageGrantedPermission();
		return true;
	}
#else
	if (PolitelyOpenAppSettings())
	{
		boolean waiting = true;

		// Wait for the app to resume
		while (waiting)
		{
			SDL_Event evt;

			while (SDL_PollEvent(&evt))
			{
				if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_RESTORED)
					waiting = false;
			}
		}

		// Destroy the current context, and create a new one.
		VID_DestroyContext();
		VID_CreateContext();

#ifdef SPLASH_SCREEN
		if (displayingSplash)
		{
			// Blit the splash screen, then present it.
			VID_BlitSplashScreen();
			VID_PresentSplashScreen();
		}
#endif

		// Permission granted -- create the directory.
		StorageGrantedPermission();
		return true;
	}
#endif // REQUEST_STORAGE_PERMISSION

	return false;
}

int main(int argc, char* argv[])
{
#ifdef LOGMESSAGES
	boolean logging = (!M_CheckParm("-nolog"));
#endif

	myargc = argc;
	myargv = argv;

	// Obtain the activity class before doing anything else.
	JNI_Startup();

	// Start up the main system.
	I_OutputMsg("I_StartupSystem()...\n");
	I_StartupSystem();

#ifdef SPLASH_SCREEN
	// Load the splash screen, and display it.
	ShowSplashScreen();
#endif

	// Init shared storage.
	StorageInit();

#ifdef LOGMESSAGES
	// Start logging.
	if (logging)
		I_InitLogging();
#endif

	CONS_Printf("Sonic Robo Blast 2 for Android\n");

	if (JNI_SharedStorage)
	{
		CONS_Printf("Shared storage location: %s\n", JNI_SharedStorage);

		// Check storage permissions.
		if (!StorageCheckPermission())
			JNI_DisplayToast("Storage permission was not granted\n");
	}
	else
		JNI_DisplayToast("There is no shared storage");

#ifdef LOGMESSAGES
	if (logging)
		CONS_Printf("Logfile: %s\n", logfilename);
#endif

	// Begin the normal game setup and loop.
	JNI_DisplayToast("Setting up SRB2...");
	D_SRB2Main();

	CONS_Printf("Entering main game loop...\n");
	D_SRB2Loop();

	return 0;
}

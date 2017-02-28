
#include <iostream>
#include <cstring>
#include <cstdio>

#include "libretro.h"

#define LOGI printf

char Key_Sate[512];
char Key_Sate2[512];
char RPATH[512];

int retrow=800;
int retroh=600;
int VIRTUAL_WIDTH;
int SHIFTON=-1,CTRLON=-1;

int mx=320,my=240;
int pauseg=0;

signed short soundbuf[735*2];

extern void stream_func(unsigned char *stream, int stream_len);
extern void QuitEmulator(void);
extern int keycode_table[256*2];

bool opt_analog=false;

uint32_t videoBuffer[1280*1024];

//BasiliskII headers
#include "sysdeps.h"
#include "main.h"
#include "adb.h"

static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_batch_t audio_batch_cb;
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void texture_init(){
        memset(videoBuffer, 0, sizeof(videoBuffer));
} 

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   struct retro_variable variables[] = {
      {
         "basilisk2_resolution","Internal resolution; 640x480|720x480|800x600|1024x768|1152x870|1280x1024",
         
      },
      {
         "basilisk2_analog","Use Analog; OFF|ON",
      },
      { NULL, NULL },
   };

   bool no_rom = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{
   struct retro_variable var = {0};

   var.key = "basilisk2_resolution";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      char *pch;
      char str[100];
      snprintf(str, sizeof(str), var.value);
      
      pch = strtok(str, "x");
      if (pch)
         retrow = strtoul(pch, NULL, 0);
      pch = strtok(NULL, "x");
      if (pch)
         retroh = strtoul(pch, NULL, 0);

        fprintf(stderr, "[libretro-test]: Got size: %u x %u.\n", retrow, retroh);
retrow=1280;retroh=1024;
        VIRTUAL_WIDTH = retrow;
        texture_init();
 
   }

   var.key = "basilisk2_analog";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      fprintf(stderr, "value: %s\n", var.value);
      if (strcmp(var.value, "OFF") == 0)
         opt_analog = false;
      if (strcmp(var.value, "ON") == 0)
         opt_analog = true;

        fprintf(stderr, "[libretro-test]: Analog: %s.\n",opt_analog?"ON":"OFF");
   }

}

/*static*/ void update_input()
{
    if (pauseg==-1)
        return;

    	input_poll_cb();

/******************************************************************************/
//MOUSE
/******************************************************************************/
      	static int mbL=0,mbR=0;
      	static int oldx=320,oldy=240;

      	int mouse_l;
      	int mouse_r;
      	int16_t mouse_x;
      	int16_t mouse_y;

      	mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      	mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      	mouse_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      	mouse_r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

	mx+=mouse_x;
	my+=mouse_y;

	if(mx<0)mx=0;
	else if(mx>retrow-1)mx=retrow-1;
	if(my<0)my=0;
	else if(my>retroh-1)my=retroh-1;

	if(oldx!=mx || oldy!=my)
		ADBMouseMoved(mx,my);

      	if(mbL==0 && mouse_l)
      	{
      	   mbL=1;		
		ADBMouseDown(0);
      	}
      	else if(mbL==1 && !mouse_l)
      	{	
		ADBMouseUp(0);
      	   mbL=0;
      	}

      	if(mbR==0 && mouse_r)
      	{
      	   mbR=1;
		ADBMouseDown(1);
      	}
      	else if(mbR==1 && !mouse_r)
      	{
		ADBMouseUp(1);
      	   mbR=0;
      	}

/******************************************************************************/
//	Joy
/******************************************************************************/
   /* 
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ? joypad_0_buttons[BUTTON_U] = 0xff : joypad_0_buttons[BUTTON_U] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ? joypad_0_buttons[BUTTON_D] = 0xff : joypad_0_buttons[BUTTON_D] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ? joypad_0_buttons[BUTTON_L] = 0xff : joypad_0_buttons[BUTTON_L] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ? joypad_0_buttons[BUTTON_R] = 0xff : joypad_0_buttons[BUTTON_R] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) ? joypad_0_buttons[BUTTON_A] = 0xff : joypad_0_buttons[BUTTON_A] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) ? joypad_0_buttons[BUTTON_B] = 0xff : joypad_0_buttons[BUTTON_B] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y) ? joypad_0_buttons[BUTTON_C] = 0xff : joypad_0_buttons[BUTTON_C] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) ? joypad_0_buttons[BUTTON_PAUSE] = 0xff : joypad_0_buttons[BUTTON_PAUSE] = 0x00;
    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT) ? joypad_0_buttons[BUTTON_OPTION] = 0xff : joypad_0_buttons[BUTTON_OPTION] = 0x00;
*/

}
    
/******************************************************************************/
//	Kbd
/******************************************************************************/

                                                                         
void retro_key_up(int retrok){
	ADBKeyUp(retrok);
}

void retro_key_down(int retrok){
	ADBKeyDown(retrok);
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
         int retrok=keycode_table/*keyboard_translation*/[keycode];

          // printf( "Down: %s, Code: %d, Char: %u, Mod: %u. ,(%d)\n",
          // down ? "yes" : "no", keycode, character, mod,retrok);

//code == 0x39 // Caps Lock pressed
//
        if (keycode>=321);
        else{
                //if(SHOWKEY==1)return;

                if(down && retrok==0x39){
                                                                                
                        if(SHIFTON == 1)retro_key_up(retrok);
                        else retro_key_down(retrok);
                        SHIFTON=-SHIFTON;                                                        
                        
                }
                else if(down && retrok==0x29){
                                                                                
                        if(CTRLON == 1)retro_key_up(retrok);
                        else retro_key_down(retrok);
                        CTRLON=-CTRLON;                                                        
                        
                }
                else {
                        if(down && retrok!=-1)                
                                retro_key_down(retrok);        
                        else if(!down && retrok!=-1)
                                retro_key_up(retrok);
                }
        }

}

/*
//        "./MacStartup.img",
static const char* xargv[] = {
        "basilisk",
        "--rom",
 //       "./PERFORMA.ROM",
        "./Quadra605.rom",
        "--config",
        "./BskII_prefs",
        "--disk",
       "../../../test.img",
        "--disk",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
};

extern int bmain(int argc, char **argv);

static void retro_wrap_emulator()
{
	int paramCount;

	for (paramCount = 0; xargv[paramCount] != NULL; paramCount++);
 
        xargv[paramCount++] = (char*)RPATH;

	bmain(paramCount, ( char **)xargv);
 
}
*/
extern int bmain(int argc, char **argv);
static char CMDFILE[512];

int loadcmdfile(char *argv)
{
    int res=0;

    FILE *fp = fopen(argv,"r");

    if( fp != NULL )
    {
	if ( fgets (CMDFILE , 512 , fp) != NULL )
		res=1;	
	fclose (fp);
    }

    return res;
}

int HandleExtension(char *path,char *ext)
{
   int len = strlen(path);

   if (len >= 4 &&
         path[len-4] == '.' &&
         path[len-3] == ext[0] &&
         path[len-2] == ext[1] &&
         path[len-1] == ext[2])
   {
      return 1;
   }

   return 0;
}

#include <ctype.h>

//Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

//extern int  skel_main(int argc, char *argv[]);
void parse_cmdline( const char *argv );

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;	
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s",option);
}

int pre_main(const char *argv)
{
   int i=0;
   bool Only1Arg;

   if (strlen(argv) > strlen("cmd"))
   {
      if( HandleExtension((char*)argv,"cmd") || HandleExtension((char*)argv,"CMD"))
          i=loadcmdfile((char*)argv);     
   }

   if(i==1)
   {
      parse_cmdline(CMDFILE);      
      LOGI("Starting game from command line :%s\n",CMDFILE);  
   }
   else
   parse_cmdline(argv); 

   Only1Arg = (strcmp(ARGUV[0],"BasiliskII") == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;


   if(Only1Arg)
   {  
/*
      if (strlen(RPATH) >= strlen("crt"))
         if(!strcasecmp(&RPATH[strlen(RPATH)-strlen("crt")], "crt"))
            Add_Option("-cartcrt");
*/
      Add_Option("BasiliskII");
      Add_Option("--rom");
      Add_Option("./Quadra605.rom");
      Add_Option("--config");
      Add_Option("./BskII_prefs");
      Add_Option("--disk");
      Add_Option(RPATH/*ARGUV[0]*/);
   }
   else
   { // Pass all cmdline args
      for(i = 0; i < ARGUC; i++)
         Add_Option(ARGUV[i]);
   }

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      LOGI("%2d  %s\n",i,XARGV[i]);
   }

   bmain(PARAMCOUNT,( char **)xargv_cmd); 

   xargv_cmd[PARAMCOUNT - 2] = NULL;

   return 0;
}

void parse_cmdline(const char *argv)
{
	char *p,*p2,*start_of_word;
	int c,c2;
	static char buffer[512*4];
	enum states { DULL, IN_WORD, IN_STRING } state = DULL;
	
	strcpy(buffer,argv);
	strcat(buffer," \0");

	for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }	
   }
}

/************************************
 * libretro implementation
 ************************************/

//static struct retro_system_av_info g_av_info;

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
	info->library_name = "BasiliskII";
	info->library_version = "0";
	info->need_fullpath = true;
	info->valid_extensions = "cmd|img|hfv|dsk";
}

/*
void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    info->timing.fps            = 60;
    info->timing.sample_rate    = 44100;
    info->geometry.base_width   = retrow;
    info->geometry.base_height  = retroh;
    info->geometry.max_width    = 1280;
    info->geometry.max_height   = 1024;
    info->geometry.aspect_ratio = 4.0 / 3.0;
}
*/

int NEWGAME_FROM_OSD  = 0;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
 // update_variables();

   info->geometry.base_width  = retrow;
   info->geometry.base_height = retroh;
printf( "AV_INFO: width=%d height=%d\n",info->geometry.base_width,info->geometry.base_height);

   info->geometry.max_width  = 1280;
   info->geometry.max_height = 1024;


printf("AV_INFO: max_width=%d max_height=%d\n",info->geometry.max_width,info->geometry.max_height);

   info->geometry.aspect_ratio = 4.0 / 3.0;
printf("AV_INFO: aspect_ratio = %f\n",info->geometry.aspect_ratio);

   info->timing.fps            = 60;
   info->timing.sample_rate    = 44100.0;
printf( "AV_INFO: fps = %f sample_rate = %f\n",info->timing.fps,info->timing.sample_rate);

}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

size_t retro_serialize_size(void)
{
	return 0;
}

bool retro_serialize(void *data, size_t size)
{
    return false;
}

bool retro_unserialize(const void *data, size_t size)
{
    return false;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{    
	const char *full_path;

    	full_path = info->path;

	strcpy(RPATH,full_path);

	printf("LOAD EMU\n");

    	return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    (void)game_type;
    (void)info;
    (void)num_info;
    return false;
}

void retro_unload_game(void)
{
     pauseg=0;
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return 0;
}

void retro_init(void)
{

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        fprintf(stderr, "Pixel format XRGB8888 not supported by platform, cannot use.\n");
        exit(0);
    }

    struct retro_keyboard_callback cbk = { keyboard_cb };
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cbk);

 	memset(Key_Sate,0,512);
 	memset(Key_Sate2,0,512);

  	update_variables();

}

void retro_deinit(void)
{
   
	ADBKeyDown(0x7f);	// Power key
	ADBKeyUp(0x7f);
	QuitEmulator();
  
        printf("Retro DeInit\n");
}

void retro_reset(void)
{

}

int RLOOP=1;
extern void retroloop();

void retro_run(void)
{       

	static int fois=0;
	if(fois==0)
	{
		fois=1;
		pre_main(RPATH);
		return;
	}

   if (NEWGAME_FROM_OSD == 1)
   {
      struct retro_system_av_info ninfo;

      retro_get_system_av_info(&ninfo);

      environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &ninfo);

      printf("ChangeAV: w:%d h:%d ra:%f.\n",
               ninfo.geometry.base_width, ninfo.geometry.base_height, ninfo.geometry.aspect_ratio);

      NEWGAME_FROM_OSD=0;
   }

	retroloop();


	if(pauseg!=-1){

		stream_func((unsigned char *)&soundbuf[0], 735*2*2);
		audio_batch_cb(soundbuf,735);
	}

        video_cb(videoBuffer, retrow, retroh, retrow << 2);

}


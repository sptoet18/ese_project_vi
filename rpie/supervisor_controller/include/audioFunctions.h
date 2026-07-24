//Author: Emiliano Perez 
//Date: 23/07/2026
//Comments: This will be the Definition of AudioFunctions 
//Version: 1.0 

#ifndef AUDIO_FUNCTIONS
#define AUDIO_FUNCTIONS

//Functions
int audioInit(void); //init ma_engine + preload cips; return 0 on success
void audioUninit(void); //Shutdown
void audioPlayFloor(int floor); //Ding, then FloorN clip - NON BLOVKING

#endif


//Author: Emiliano Perez 
//Date: 23/07/2026
//Comments: This will be the Development of AudioFunctions 
//Version: 1.0 



#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"
#include "../include/audioFunctions.h"

#include <stdio.h>

//Where the Audio Files live
#define AUDIO_DIR "/var/www/ese_project_vi/rpie/Music/"
#define NUM_FLOORS 3
#define DING_GAP_MS 120 //Small Pause 

static ma_engine g_engine;
static ma_sound  g_ding;
static ma_sound  g_floor[NUM_FLOORS];
static int g_audioReady = 0;
static int g_dingReady  = 0; //The ding is optional - a missing ding must not kill the voice clips

static const char *g_floorFiles[NUM_FLOORS] = {
    AUDIO_DIR "floor-one.mp3",
    AUDIO_DIR "floor-two.mp3", 
    AUDIO_DIR "floor-three.mp3"
}; 

//Loade one clip fully into RAM so on arrival never pays for file I/O decoding 
static int audioLoad(ma_sound *snd, const char *path){
    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION; 

    if(ma_sound_init_from_file(&g_engine, path, flags, NULL, NULL, snd) != MA_SUCCESS){
        printf("[AUDIO] Could not load %s\n", path);
        return -1; 
    }

    return 0; 
}

int audioInit(void){
    if(g_audioReady){
        return 0; 
    }

    if(ma_engine_init(NULL, &g_engine) != MA_SUCCESS){
        printf("[AUDIO] Could not start the audio engine - Continuing silently\n");
        return -1; 
    }

    //The ding is a nice-to-have: if it is missing we still announce the floor
    g_dingReady = (audioLoad(&g_ding, AUDIO_DIR "ding.wav") == 0) ? 1 : 0;
    if(!g_dingReady){
        printf("[AUDIO] No ding.wav - announcing floors without the ding\n");
    }

    for(int i = 0; i < NUM_FLOORS; i++){
        if(audioLoad(&g_floor[i], g_floorFiles[i]) != 0){
            //Unwind everything already loaded before bailing out
            if(g_dingReady){
                ma_sound_uninit(&g_ding);
                g_dingReady = 0;
            }
            for(int j = 0; j < i; j++){
                ma_sound_uninit(&g_floor[j]);
            }
            ma_engine_uninit(&g_engine);
            return -1; 
        }
    }

    g_audioReady = 1; 
    printf("[AUDIO] Engine Ready (%u Hz) - clips preloaded\n", ma_engine_get_sample_rate(&g_engine));
    return 0; 
}

void audioUninit(void){
    if(!g_audioReady){
        return; 
    }

    if(g_dingReady){
        ma_sound_uninit(&g_ding);
        g_dingReady = 0;
    }
    for(int i = 0; i < NUM_FLOORS; i++){
        ma_sound_uninit(&g_floor[i]);
    }
    ma_engine_uninit(&g_engine);

    g_audioReady = 0;
    printf("[AUDIO] Engine Stopped\n");
}

//Rewind a Clip so it can be triggered again on the next rrival 
static void audioRewind(ma_sound *snd){
    if(ma_sound_is_playing(snd)){
        ma_sound_stop(snd);
    }
    ma_sound_seek_to_pcm_frame(snd, 0); 
}

//Play the audio at floor 
void audioPlayFloor(int floor){
    if(!g_audioReady){
        return; 
    }

    if(floor < 1 || floor > NUM_FLOORS){
        printf("[AUDIO] Ignoring bad floor %d\n", floor); 
        return; 
    }

    ma_sound *voice = &g_floor[floor - 1]; 

    audioRewind(voice);

    ma_uint32 rate = ma_engine_get_sample_rate(&g_engine);
    ma_uint64 now  = ma_engine_get_time_in_pcm_frames(&g_engine);
    ma_uint64 dingLen = 0;

    if(g_dingReady){
        audioRewind(&g_ding);

        //Measure this ding in seconds and conver using the Engine sample rate
        //(the clip's own rate may differ from the engine's, so never feed
        // ma_sound_get_length_in_pcm_frames straight into the engine clock)
        float dingSecs = 0.0f;
        if(ma_sound_get_length_in_seconds(&g_ding, &dingSecs) != MA_SUCCESS){
            dingSecs = 0.0f;
        }

        dingLen = (ma_uint64)(dingSecs * rate) + ((ma_uint64)DING_GAP_MS * rate / 1000);

        //Schedule both on the engines clock. Ding then vouce
        ma_sound_set_start_time_in_pcm_frames(&g_ding, now);
        ma_sound_start(&g_ding);
    }

    ma_sound_set_start_time_in_pcm_frames(voice, now + dingLen);
    ma_sound_start(voice);

    printf("[AUDIO] %sFloor %d announcement queued\n", g_dingReady ? "Ding + " : "", floor);
}

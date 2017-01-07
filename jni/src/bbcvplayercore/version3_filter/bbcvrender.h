#ifndef __BBCVRENDER_H_
#define __BBCVRENDER_H_

#include <stdio.h>
#include <stdlib.h>
#include "DataHead.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "SDL.h"  
#include "SDL_log.h"  
#include "SDL_main.h"  
#ifdef __cplusplus
}
#endif
#include "RingBuffer.h"
#include <pthread.h>


//Refresh Event  
#define REFRESH_EVENT  (SDL_USEREVENT + 1)  
#define QUIT_EVENT  (SDL_USEREVENT + 2)  
#define CHANGESIZE_EVENT (SDL_USEREVENT + 3)  
#define PCM_TEST (SDL_USEREVENT + 4)  

class BBCVRender{
public:

	BBCVRender();
	~BBCVRender();
	int Init();
	int InitAll(SDLParam sdlParam);
	int Render(unsigned char* buffer);
	int loop();
	int SendRenderMsg(Uint32 uMsg);
	static void* Thread_Render(void* param);
	static void Fill_audioData(void* data,Uint8* stream,int len);
	int PlayVideoData(unsigned char* data,pthread_mutex_t *lock);
	int PlayAudioData(unsigned char* data,int len);
	int InitAudioPlayer(AudioParam audiopm);
	int inputToRingbuffer(unsigned char* recv_buffer,int recv_len);
	int getFromRingbuffer(unsigned char* outdata,int ilen);
	struct SDL_Window *m_screen ;
    	struct SDL_Renderer *m_sdlRenderer ;
    	struct SDL_Surface *m_bmp ;
    	struct SDL_Texture *m_sdlTexture ;
	SDL_AudioSpec m_wantedSpec;
	
	bool m_bstop;// get quit event
	int m_iScreenWidth;
	int m_iScreenHeight;
	int m_iTextureWidth;
	int m_iTextureHeight;
	unsigned char* m_renderbuffer;//set after decode
	pthread_mutex_t* m_videolock; //video lock set by user
	pthread_mutex_t m_renderlock; //protect video lock for not change in render 
	int m_audio_len; //can read audio len
	unsigned char* m_audiobuffer;
	unsigned char* m_audiobuffer_pos;//current buffer pos
	RingBuffer * m_RingBuffer;// ringbuffer;
	SDLParam m_sdlparam; //save sdl param
};


#endif

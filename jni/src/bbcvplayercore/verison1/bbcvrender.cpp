#include "bbcvrender.h"
#include <pthread.h>
/*
//#ifdef __cplusplus
extern "C" {
//#endif
#include "SDL.h"  
#include "SDL_log.h"  
#include "SDL_main.h"  
//#ifdef __cplusplus
}
//#endif
*/
#define LOAD_BGRA    0  
#define LOAD_RGB24   0  
#define LOAD_BGR24   0  
#define LOAD_YUV420P 1 

BBCVRender::BBCVRender()
{

}
BBCVRender::~BBCVRender()
{

}

int BBCVRender::InitAll(SDLParam sdlParam)
{
	m_iScreenHeight = sdlParam.iScreenHeight;
	m_iScreenWidth = sdlParam.iScreenWidth;
	m_iTextureHeight = sdlParam.iTextureHeight; //yuv
	m_iTextureWidth = sdlParam.iTextureWidth; //yuv size
	m_audio_len = 0;
	m_audiobuffer_pos =NULL;
	m_audiobuffer = NULL;
	m_renderbuffer = NULL;
 	LOGD("yyd info init sdl\n"); 
/* 
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == -1) {  
       LOGD("SDL_Init failed %s", SDL_GetError());  
	return -1;
    }  
       LOGD("SDL_Init success \n");  
*/	//SDL 2.0 Support for multiple windows  
#if 1
   m_screen = SDL_CreateWindow("bbcvplayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
        m_iScreenWidth, m_iScreenHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN );  
 //  m_screen = SDL_CreateWindow("", 50, 50,  
 //       m_iScreenWidth, m_iScreenHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);  
    if(!m_screen) {    
        LOGD("SDL: could not create window - exiting:%s\n",SDL_GetError());    
        return -1;  
    }    
	LOGD("yyd info create window success");
	m_sdlRenderer = SDL_CreateRenderer(m_screen, -1, 0);    
#else
	  if(SDL_CreateWindowAndRenderer(0, 0, 0, &m_screen, &m_sdlRenderer) < 0){
		LOGD("create window render failed %s \n",SDL_GetError);
		return -1;	
	}
	LOGD("yyd info create window success");
  
#endif
	    Uint32 pixformat=0;  //д��
	
#if 1
#if LOAD_BGRA  
    //Note: ARGB8888 in "Little Endian" system stores as B|G|R|A  
    pixformat= SDL_PIXELFORMAT_ARGB8888;    
#elif LOAD_RGB24  
    pixformat= SDL_PIXELFORMAT_RGB888;    
#elif LOAD_BGR24  
    pixformat= SDL_PIXELFORMAT_BGR888;    
#elif LOAD_YUV420P  
    //IYUV: Y + U + V  (3 planes)  
    //YV12: Y + V + U  (3 planes)  
    pixformat= SDL_PIXELFORMAT_IYUV;    
#endif  
#endif 
	
 //   pixformat= SDL_PIXELFORMAT_YV12;    

	m_sdlTexture = SDL_CreateTexture(m_sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,m_iTextureWidth,m_iTextureHeight);  
	if(m_sdlTexture == NULL)
		LOGD("creadt texture error %s\n",SDL_GetError());

	LOGD("yyd info init sdl over begin create render thread\n");
	                pthread_t render_thread;
                        pthread_create(&render_thread, NULL, BBCVRender::Thread_Render, this);
                        pthread_detach(render_thread);	
	
	//init audio
	int iret = InitAudioPlayer(sdlParam.audioParam);	
	if(-1 == iret){
		
		LOGD("open audio sdl faild \n");
		return -1;
	}
	


	return 0;
}

int BBCVRender::Init()
{
 
	m_iScreenHeight = 720;
	m_iScreenWidth = 1280;
	m_iTextureHeight = 720; //yuv
	m_iTextureWidth = 1280; //yuv size
	m_audio_len = 0;
	m_audiobuffer_pos =NULL;
	m_audiobuffer = NULL;
	m_renderbuffer = NULL;
 //   char *filepath = "/storage/emulated/0/test.bmp";  
 	LOGD("yyd info init sdl\n"); 
//	   SDL_SetMainReady();
/* 
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == -1) {  
       LOGD("SDL_Init failed %s", SDL_GetError());  
	return -1;
    }  
       LOGD("SDL_Init success \n");  
*/	//SDL 2.0 Support for multiple windows  
#if 1
   m_screen = SDL_CreateWindow("bbcvplayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
        m_iScreenWidth, m_iScreenHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN );  
 //  m_screen = SDL_CreateWindow("", 50, 50,  
 //       m_iScreenWidth, m_iScreenHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);  
    if(!m_screen) {    
        LOGD("SDL: could not create window - exiting:%s\n",SDL_GetError());    
        return -1;  
    }    
	LOGD("yyd info create window success");
	m_sdlRenderer = SDL_CreateRenderer(m_screen, -1, 0);    
#else
	  if(SDL_CreateWindowAndRenderer(0, 0, 0, &m_screen, &m_sdlRenderer) < 0){
		LOGD("create window render failed %s \n",SDL_GetError);
		return -1;	
	}
	LOGD("yyd info create window success");
  
#endif
	    Uint32 pixformat=0;  
#if 1
#if LOAD_BGRA  
    //Note: ARGB8888 in "Little Endian" system stores as B|G|R|A  
    pixformat= SDL_PIXELFORMAT_ARGB8888;    
#elif LOAD_RGB24  
    pixformat= SDL_PIXELFORMAT_RGB888;    
#elif LOAD_BGR24  
    pixformat= SDL_PIXELFORMAT_BGR888;    
#elif LOAD_YUV420P  
    //IYUV: Y + U + V  (3 planes)  
    //YV12: Y + V + U  (3 planes)  
    pixformat= SDL_PIXELFORMAT_IYUV;    
#endif  
#endif 
	
 //   pixformat= SDL_PIXELFORMAT_YV12;    

#if 1
	m_sdlTexture = SDL_CreateTexture(m_sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,m_iTextureWidth,m_iTextureHeight);  
	if(m_sdlTexture == NULL)
		LOGD("creadt texture error %s\n",SDL_GetError());

#endif  
	LOGD("yyd info init sdl over begin create render thread\n");
	                pthread_t render_thread;
                        pthread_create(&render_thread, NULL, BBCVRender::Thread_Render, this);
                        pthread_detach(render_thread);

}

int BBCVRender::Render(unsigned char* buffer)
{
	if(buffer == NULL)
	{
		LOGD("yyd info bbcvrender render failed buffer=null  textwidth=%d\n",m_iTextureWidth);
		return -1;
	}	
#if 1
	LOGD("yyd info bbcvrender render begin  textwidth=%d\n",m_iTextureWidth);
#if LOAD_BGRA  
            //We don't need to change Endian  
            //Because input BGRA pixel data(B|G|R|A) is same as ARGB8888 in Little Endian (B|G|R|A)  
            SDL_UpdateTexture(m_sdlTexture, NULL, buffer, m_iTextureWidth*4);    
#elif LOAD_RGB24|LOAD_BGR24  
            //change 24bit to 32 bit  
            //and in Windows we need to change Endian  
            CONVERT_24to32(buffer,buffer_convert,m_iTextureWidth,m_iTextureHeight);  
            SDL_UpdateTexture( m_sdlTexture, NULL, buffer_convert, m_iTextureWidth*4);    
#elif LOAD_YUV420P  
            SDL_UpdateTexture( m_sdlTexture, NULL, buffer, m_iTextureWidth);    
#endif  
	            //FIX: If window is resize  
  		SDL_Rect sdlRect;    
            sdlRect.x = 0;    
            sdlRect.y = 0;    
            sdlRect.w = m_iScreenWidth;    
            sdlRect.h = m_iScreenHeight;    
              
            SDL_RenderClear( m_sdlRenderer );     
            SDL_RenderCopy( m_sdlRenderer, m_sdlTexture, NULL, &sdlRect);    
            SDL_RenderPresent( m_sdlRenderer );  
#else
	                SDL_SetRenderDrawColor(m_sdlRenderer, 0xA0, 0xA0, 0xA0, 0xFF);
                SDL_RenderClear(m_sdlRenderer);
            SDL_UpdateTexture( m_sdlTexture, NULL, buffer, m_iTextureWidth);    
	        int w, h;
        SDL_GetWindowSize(m_screen, &w, &h);
//	LOGD("yyd info bbcvrender render begin  textwidth=%d texheight=%d,sw=%d sh=%d\n",
//		m_iTextureWidth,m_iTextureHeight,w,h);
        SDL_Rect sdlRect = {w/2 - m_iTextureWidth/2, h/2 - m_iTextureHeight/2, m_iTextureWidth, m_iTextureHeight};
        /* Blit the sprite onto the screen */
        SDL_RenderCopy( m_sdlRenderer, m_sdlTexture, NULL, &sdlRect);    
        SDL_RenderPresent( m_sdlRenderer );  
//	SDL_Delay(10);	

#endif

//	LOGD("yyd info bbcvrender render over\n");
	return 0;
}

int BBCVRender::loop()
{
	 SDL_Event event; 
	while(1){
	 	SDL_WaitEvent(&event);  
        	if(event.type==REFRESH_EVENT){  
			LOGD("yyd info SDL get Refresh event \n");
			Render(m_renderbuffer);
		}
		else if(event.type==SDL_WINDOWEVENT){  
            		//If Resize  
           	  SDL_GetWindowSize(m_screen,&m_iScreenWidth,&m_iScreenHeight);  
        	}else if(event.type==SDL_QUIT){  
           	 break;  
        	} 

	}
	return 0;
}
void* BBCVRender::Thread_Render(void* param)
{
	BBCVRender* this0 = (BBCVRender*)param;
	this0->loop();
	return NULL;
}
int BBCVRender::SendRenderMsg(Uint32 uMsg)
{

        SDL_Event event;  
        event.type = uMsg;  
        SDL_PushEvent(&event);  
	return 0;
}
void BBCVRender::Fill_audioData(void* data,Uint8* stream,int len)
{
	//LOGD("yyd info mixer audio in sdl  begin len =%d \n",len);
	BBCVRender* this0 = (BBCVRender*)data;
	if(NULL == this0->m_audiobuffer_pos){
		
	//	LOGD("yyd info sdl audio buffer null\n");
		return;
	}
	SDL_memset(stream,0,len);
	if(this0->m_audio_len <= 0){
		SDL_Delay(1);
		return ;
	}
	len = (len>this0->m_audio_len)?this0->m_audio_len:len;
	LOGD("yyd info mixer audio in sdl len =%d \n",len);
	SDL_MixAudio(stream,this0->m_audiobuffer_pos,len,SDL_MIX_MAXVOLUME);
	this0->m_audiobuffer_pos += len;
	this0->m_audio_len -= len;

}
int BBCVRender::InitAudioPlayer(AudioParam pm)
{
	SDL_memset(&m_wantedSpec, 0, sizeof(m_wantedSpec));
	m_wantedSpec.freq = pm.audioFreq;
	m_wantedSpec.channels = pm.audioChannels;
	m_wantedSpec.samples = pm.audioSamples;
	m_wantedSpec.silence = pm.audiosilence;
	m_wantedSpec.callback = BBCVRender::Fill_audioData;  //当前会出现第一次调用音频指针未赋值，导致崩溃
	m_wantedSpec.userdata = this;
	switch(pm.audioFormat)
	{
		case 1:
			m_wantedSpec.format = AUDIO_U8;
			break;
		case 2:
			m_wantedSpec.format = AUDIO_S8;
			break;
		case 3:
			m_wantedSpec.format = AUDIO_U16LSB;	
			break;
		case 4:
			m_wantedSpec.format = AUDIO_S16LSB;
			break;
		case 5:
			m_wantedSpec.format = AUDIO_U16MSB;
			break;
		case 6: 
			m_wantedSpec.format = AUDIO_S16MSB;
			break;
		default:
			m_wantedSpec.format = AUDIO_S16LSB;
			break;
	}
	if(SDL_OpenAudio(&m_wantedSpec,NULL)!=0)
	{
		LOGD("open audio sdl faild \n");
		return -1;
		
	}
	
	LOGD("yyd info  audio init sdl success \n");
	SDL_PauseAudio(0); //paly audio
	return 0;
}

int BBCVRender::PlayAudioData(unsigned char* data,int len)
{
	m_audiobuffer_pos = data;
	m_audio_len = len;
		
	return 0;
}

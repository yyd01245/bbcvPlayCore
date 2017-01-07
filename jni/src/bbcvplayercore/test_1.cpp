#include "Decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "bbcvrender.h"

const int BUFFSIZE = 1280*720*3/2;

FILE *fp =NULL;
int main(int argc,char**argv)
{
//	printf("--input file %s \n",argv[1]);
	int iwidth = 0;
	int iHeight = 0;
	//const char* filename = "/home/ky/rsm-yyd/DecoderTs/1.ts";
	DecoderControlParam m_dcParam={0};
	        m_dcParam.bSmooth = true;
        m_dcParam.bNeedControlPlay = false;
        m_dcParam.bNeedLog = false;
        m_dcParam.bSaveVideoData= false;
        m_dcParam.bDelayCheckModel=false;
        m_dcParam.bAutoMatch= true;
	TSDecoder_Instance *pInstance = new TSDecoder_Instance;
	SDLParam sdlParam;
	sdlParam.iScreenHeight = 720;
	sdlParam.iScreenWidth = 1280;
	sdlParam.iTextureHeight = 720;
	sdlParam.iTextureWidth = 1280;
	sdlParam.pixformat =0;
	sdlParam.audioParam.audioChannels =  2;
	sdlParam.audioParam.audioFormat = 4;
	sdlParam.audioParam.audioFreq = 48000;
	sdlParam.audioParam.audioSamples = 1152;
	sdlParam.audioParam.audiosilence = 0;
	
	int iret = pInstance->init_SDL(sdlParam);
	
	pInstance->init_TS_Decoder("udp://@:55555",m_dcParam);
	LOGD("create decoder ,init success \n");
	//pInstance = init_TS_Decoder("/home/ky/rsm-yyd/DecoderTs/dsa.ts",iwidth,iHeight);
	//
	//usleep(40*1000);

	pInstance->m_pRender->loop();

	//udp @ 0x7facd4000e60] bind failed: Address already in use

	int output_video_size = BUFFSIZE;
//	unsigned char *output_video_yuv420 = new unsigned char[BUFFSIZE];

	int input_audio_size = 1024*100;
//	unsigned char *output_audio_data = new unsigned char[1024*100];
//	fp = fopen("/home/ky/rsm-yyd/DecoderTs/overlay/yuv","wb+");
//	{
//		if(NULL == fp)
//			return -1;
//	}

//	FILE *fpaudio = fopen("/home/ky/rsm-yyd/DecoderTs/overlay/audio.pcm","wb+");
//	if(NULL == fpaudio)
//		return -1;
#if 0
	unsigned long ulpts = 0;
	unsigned long audio_pts = 0;
	int iloop = 25*20000;
	while(1)
		{
			usleep(39*100000000);
/*			output_video_size = BUFFSIZE;
			int ret = get_Video_data(pInstance,output_video_yuv420,&output_video_size,&iwidth,&iHeight,&ulpts);
			if(--iloop < 0){
				Set_tsDecoder_stat(pInstance,true);
				
			}
			if(ret ==0)
			{
				//do
				//{
					ret = get_Audio_data(pInstance,output_audio_data,&input_audio_size,&audio_pts);
					//fwrite(output_audio_data,1,input_audio_size,fpaudio);
					printf("----audio outputsize=%d,pts =%d,ret=%d\n",input_audio_size,audio_pts,ret);
				//}while(ret ==0);
				
				fprintf(stderr,"video outputsize=%d ,w=%d,h=%d audiopts=%d ,videopts=%d\n",output_video_size,iwidth,iHeight,audio_pts,ulpts);
			//fwrite(output_video_yuv420,1,output_video_size,fp);
				iwidth=0;
				iHeight=0;
			}
*/
		}
#endif
	return 0;
}

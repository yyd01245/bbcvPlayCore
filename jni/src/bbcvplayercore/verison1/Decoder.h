/*
	author yyd
		
*/

#ifndef __DECODER_H_
#define __DECODER_H_

#include <deque>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>

#ifndef INT64_C
#define INT64_C(c) c##LL
#endif
#ifndef UINT64_C
#define UINT64_C(c) c##LL
#endif
extern "C" {

#include "libavutil/pixfmt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/samplefmt.h"

#include "libavutil/avstring.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"

}

#include "DataHead.h"
#include "TSDecoder.h"
#include "TSStreamInfo.h"
#include "RecvQueue.h"
#include "bbcvrender.h"



class TSDecoder_Instance
{
public:
	TSDecoder_Instance();
	~TSDecoder_Instance();

		int init_SwrConverAudio(AVFrame * pAudioDecodeFrame,uint8_t * out_buf);

	int init_TS_Decoder(const char* cfilename,DecoderControlParam dcParam);

	int init_SDL(SDLParam sdlParam);
	int get_Video_data(unsigned char *output_video_yuv420,int *output_video_size,
	int* iwidth,int* iHeight,unsigned long* video_pts=NULL); 

	void Push_Video_Data(int iWidth, int iHight, AVFrame *pAVfram,unsigned long ulPTS);
	
	int get_Audio_data(unsigned char *output_audio_data,int* input_audio_size,
	unsigned long* audio_pts);

	void Push_Audio_Data(unsigned char *sample,int isize,unsigned long ulPTS);

	int uninit_TS_Decoder();

	//static unsigned int _stdcall decoder_threadFun(void *param);

	static void* decoder_threadFun(void *param);

	void stopDecoder(bool bstop);

	bool initQueue(int isize);

	bool freeQueue();

	int get_video_param(int *iwidth,int *iheight);

	int initdecoder();

	bool set_tsDecoder_stat(bool bstat);
	
	static int read_data(void *opaque, uint8_t *buf, int buf_size);

	int get_queue(uint8_t* buf, int size);

	bool open_inputdata();

	int init_open_input();

	bool Clean_Video_audioQue();
	bool Get_tsDecoder_sem(void **pSem);
	bool Set_tsDecoder_Volume(int iVolume);
	bool Get_tsDecoder_Volume(int &iVolume);

	//设置码率周期
	bool Set_tsRate_period(int iperiod);

	//获取到码率
	bool Get_tsRate(int* iRate);
	//计算延时
	bool Set_tsTime_delay(int begintime,int* relsutTime);

	bool Get_tsIFrame_size(int* iSize);

	bool Set_tsDecoder_SaveStream(bool bSaveStream);
	BBCVRender *m_pRender;// render 
private:

	DecoderControlParam m_dcControlParam;//解码控制
	
	URLTYPE m_iURLType;

	char m_strRtspServerIP[256];
	int m_iRtspPort;

	long m_lDisplayPort;
	unsigned char *m_yuvBuff[2];
	int m_iCurrentOutBuf;
	//CRITICAL_SECTION m_csVideoYUV[2];

	uint8_t *m_avbuf;

	FILE *fpLog ;

	FILE *m_MediaInfofp ;
		char m_meidaInfoPath[256];

	bool m_bNeedControlPlay;
	unsigned char *m_audioBuff;

	struct SwsContext *m_img_convert_ctx;  
	//AVAudioResampleContext *m_audio_resample;
	struct SwrContext* m_swr_ctx;
	AVCodec *m_pVideoCodec;
	AVCodec *m_pAudioCodec;
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx;
	AVIOContext * m_pb;
	//AVInputFormat *m_piFmt;
	AVFormatContext *m_pFmt;
	AVFrame *m_pframe;
	int m_iWidht;
	int m_iHeight;
	int m_videoindex;
	int m_audioindex;
	std::deque<VideoData*> m_usedQueue;
	std::deque<VideoData*> m_freeQueue;	
	pthread_mutex_t m_locker;
	std::deque<AudioData*> m_audioUsedQueue;
	std::deque<AudioData*> m_audioFreeQueue;
	pthread_mutex_t m_audioLocker;

	unsigned long m_ulvideo_pts;
	unsigned m_iThreadid;
	bool m_bstop;
	TSFILETYPE m_ifileType;//url 1 or file 2

	NewQueue m_recvqueue;
	char m_cfilename[1024];

	char m_strDstIP[256];
	int m_iPort;

	VideoCodeType m_vCodetype;
	AuidoCodeType m_aCodeType;
	bool m_bDecoderFlag;//true decoder Auto,
	bool m_bFirstDecodeSuccess; //I frame decode

	TSstreamInfo m_tsStreamPrase;//解析ts

	bool m_bSmoothPlay;
	int m_iDelayFrame;
	bool m_bSaveTSStream; //保存ts流
	FILE *m_tsStreamfp;


	int m_iperiod;//码率周期
	int m_iIFrameSize;//上一个Iframe大小
		int m_ISInterlacedFrame; //隔行
			bool m_bGetData ; //标识获取到yuv数据

};


#endif

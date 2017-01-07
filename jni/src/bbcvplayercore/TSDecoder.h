/*
	author yyd
		
*/

#ifndef __TSDECODER_H__
#define __TSDECODER_H__

#include "DataHead.h"

//<summary>
//<param cfilename> 文件名
//<param iWidht>  指定输出YUV的widht
//<param iHight> 指定输出YUV的height
TSDecoder_t* init_TS_Decoder(const char* cfilename,DecoderControlParam dcParam);

//<summary>
//<param ts_decoder>  instance
//<param > OUT 输出YUV的宽和高等
int get_Video_Param(TSDecoder_t *ts_decoder,DecodeParam* pdcParam); 


//<summary>
//<param ts_decoder>  instance
//<param pvideobuff>  OUT 输出YUV420数据
//OUT IN 输入output_video_yuv420的容量大小， 输出output_video_yuv420实际长度
//OUT 输出YUV的宽和高
//OUT video PTS

int get_Video_data(TSDecoder_t *ts_decoder,VideoData *pVideobuff); 

//<summary>
//<param ts_decoder>  instance
//<param pAudiobuff>  OUT 输出音频数据
//OUT IN 输入output_video_data的容量大小， 输出output_audio_data实际长度
//OUT 输出音频数据的PTS
int get_Audio_data(TSDecoder_t *ts_decoder,AudioData* pAudiobuff);

//<summary> 设置解码开关，true 为自动匹配，FALSE为指定解码
bool Set_tsDecoder_stat(TSDecoder_t *ts_decoder,bool bStart);

//设置码率周期
bool Set_tsRate_period(TSDecoder_t *ts_decoder,int iperiod);

//获取到码率
bool Get_tsRate(TSDecoder_t *ts_decoder,int* iRate);
//计算延时
bool Set_tsTime_delay(TSDecoder_t *ts_decoder,int begintime,int* relsutTime);

bool Get_tsIFrame_size(TSDecoder_t *ts_decoder,int* iSize);


bool Get_tsDecoder_sem(TSDecoder_t *ts_decoder,void **pSem);

bool Set_tsDecoder_Volume(TSDecoder_t *ts_decoder,int iVolume);

bool Get_tsDecoder_Volume(TSDecoder_t *ts_decoder,int &iVolume);

bool Set_tsDecoder_SaveStream(TSDecoder_t *ts_decoder,int bsave);

int uninit_TS_Decoder(TSDecoder_t *audioencoder);

#endif


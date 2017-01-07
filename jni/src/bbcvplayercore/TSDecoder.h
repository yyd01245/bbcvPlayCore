/*
	author yyd
		
*/

#ifndef __TSDECODER_H__
#define __TSDECODER_H__

#include "DataHead.h"

//<summary>
//<param cfilename> �ļ���
//<param iWidht>  ָ�����YUV��widht
//<param iHight> ָ�����YUV��height
TSDecoder_t* init_TS_Decoder(const char* cfilename,DecoderControlParam dcParam);

//<summary>
//<param ts_decoder>  instance
//<param > OUT ���YUV�Ŀ�͸ߵ�
int get_Video_Param(TSDecoder_t *ts_decoder,DecodeParam* pdcParam); 


//<summary>
//<param ts_decoder>  instance
//<param pvideobuff>  OUT ���YUV420����
//OUT IN ����output_video_yuv420��������С�� ���output_video_yuv420ʵ�ʳ���
//OUT ���YUV�Ŀ�͸�
//OUT video PTS

int get_Video_data(TSDecoder_t *ts_decoder,VideoData *pVideobuff); 

//<summary>
//<param ts_decoder>  instance
//<param pAudiobuff>  OUT �����Ƶ����
//OUT IN ����output_video_data��������С�� ���output_audio_dataʵ�ʳ���
//OUT �����Ƶ���ݵ�PTS
int get_Audio_data(TSDecoder_t *ts_decoder,AudioData* pAudiobuff);

//<summary> ���ý��뿪�أ�true Ϊ�Զ�ƥ�䣬FALSEΪָ������
bool Set_tsDecoder_stat(TSDecoder_t *ts_decoder,bool bStart);

//������������
bool Set_tsRate_period(TSDecoder_t *ts_decoder,int iperiod);

//��ȡ������
bool Get_tsRate(TSDecoder_t *ts_decoder,int* iRate);
//������ʱ
bool Set_tsTime_delay(TSDecoder_t *ts_decoder,int begintime,int* relsutTime);

bool Get_tsIFrame_size(TSDecoder_t *ts_decoder,int* iSize);


bool Get_tsDecoder_sem(TSDecoder_t *ts_decoder,void **pSem);

bool Set_tsDecoder_Volume(TSDecoder_t *ts_decoder,int iVolume);

bool Get_tsDecoder_Volume(TSDecoder_t *ts_decoder,int &iVolume);

bool Set_tsDecoder_SaveStream(TSDecoder_t *ts_decoder,int bsave);

int uninit_TS_Decoder(TSDecoder_t *audioencoder);

#endif


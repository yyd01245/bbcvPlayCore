#ifndef __DATAHEAD_H_
#define __DATAHEAD_H_

#include "Common.h"

typedef void TSDecoder_t;

typedef enum enVideoType
{
	CODE_HD_VIDEO = 1,
	CODE_SD_VIDEO
}VideoCodeType;

typedef enum enAuidoCodeType
{
	CODE_AUIDO_MP2=1,
	CODE_AUIDO_MP3,
	CODE_AUDIO_AAC
}AuidoCodeType;


typedef enum
{
	tsUDP =1,
	tsRTSP = 2,
	tsLocalFile =3,
	othertype
}URLTYPE;

typedef struct _DecodeParam
{
	bool bNeedLog;				//��Ҫ��־
	bool bNeedControlPlay;		//���Ʋ���
	bool bSmooth;				//ƽ������
	bool bAutoMatch;			//�Զ�ƥ��
	bool bSaveVideoData;		//������Ƶ����
	bool bDelayCheckModel;		//��ʱ������
	VideoCodeType vCodetype;    //��Ƶ����
	AuidoCodeType aCodetype;	//��Ƶ����
	char strDstIP[128];			//Ŀ��ip
	int iport;					//Ŀ�Ķ˿�

}DecoderControlParam;


typedef struct _DecoderParam{
	int iWidth;
	int iHeight;
	
}DecodeParam;


typedef struct _VideoData {
	unsigned char *data;
	long size;
	unsigned long pts;
	int iWidth;
	int iHeight;
} VideoData;

typedef enum
{
	internetfile =1,
	localfile = 2,
	other =3
}TSFILETYPE;

typedef struct _AudioData {
	unsigned char *data;
	long size;
	unsigned long pts;
} AudioData;


typedef struct _AudioParam{
	int audioFreq;  /**< DSP frequency -- samples per second */
	int audioFormat; /**< Audio data format */
	/*
	1 SDL::AUDIO_U8      	*< Unsigned 8-bit samples 
	2 SDL::AUDIO_S8     	*< Signed 8-bit samples 
	3 SDL::AUDIO_U16LSB   	*< Unsigned 16-bit samples 
	4 SDL::AUDIO_S16LSB  	*< Signed 16-bit samples 
	5 SDL::AUDIO_U16MSB 	*< As above, but big-endian byte order 
	6 SDL::AUDIO_S16MSB  	*< As above, but big-endian byte order 
	*/
	int audioChannels;  /**< Number of channels: 1 mono, 2 stereo */
	int audiosilence;   /**< Audio buffer silence value (calculated) */
	int audioSamples;  /**< Audio buffer size in samples (power of 2) */



}AudioParam;

typedef struct _SDLParam{
	//video
	int iTextureWidth;
	int iTextureHeight;
	int iScreenWidth;
	int iScreenHeight;
	unsigned int pixformat;
	//audio
	AudioParam audioParam;
	

}SDLParam;


#endif



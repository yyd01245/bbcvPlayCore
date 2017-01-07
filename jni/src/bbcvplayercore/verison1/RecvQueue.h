/*
	author yyd
		
*/

#ifndef __RECVQUEUE_H_
#define __RECVQUEUE_H_



#if 0
//#include <pthread.h>
//#include "..\socketlayer\include\ip4socket.h"
#include <stdio.h>
//#include <sys/types.h>
#include <stdlib.h>
//#include <string.h>
#include <map>
#include "Common.h"
#include "TSStreamInfo.h"

//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <errno.h>
//#include <arpa/inet.h>
//#include <pthread.h>
//#include <semaphore.h>

#endif

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <map>
#include "Common.h"
#include "TSStreamInfo.h"
/*
void pthread_mutex_lock(void* param)
{
	//CRITICAL_SECTION *csParam = (CRITICAL_SECTION *)param;
	EnterCriticalSection((CRITICAL_SECTION *)param);
}

void pthread_mutex_unlock(void* param)
{
	LeaveCriticalSection((CRITICAL_SECTION *)param);
}

void pthread_mutex_destroy(void *param)
{

	//Destroy
}

void pthread_mutex_init(void* param,void *p)
{
	InitializeCriticalSection((CRITICAL_SECTION *)param);
}
*/



class  NewQueue
{
public:
	pthread_mutex_t locker;
	//pthread_cond_t cond;
	sem_t m_sem_send;
	uint8_t* buf;
	int bufsize;
	int write_ptr;
	int read_ptr;
	int m_iport;
	int m_iSendPort;
	char m_cdstIP[256];
	bool m_boverlay;
	bool m_hsIDRFrame;
	bool m_bIsOverlaying;
	bool m_bInitDecoder;
	bool m_bDelayFrame;

		unsigned udp_recv_thread;
	bool m_bstop;

	FILE *m_logfp;
	FILE *m_Mediafp;

	NewQueue(int iport=12000);
	~NewQueue();

//	static unsigned int _stdcall udp_ts_recv(void* param);
static void* udp_ts_recv(void* param);
	void init_queue( int size,int iport,const char* dstip,short isendPort,FILE* fp=NULL,FILE* fpInfo=NULL,bool bNeedControlPlay=false);
	void free_queue();
	void put_queue( uint8_t* buf, int size);
	int get_queue(uint8_t* buf, int size);
	bool set_tsDecoder_stat(bool bstat);
	void clean_RecvQue();
	bool dumxer(unsigned char* buff,int ilen,int *iHandleLen,int iflag=7);
	void filterNullPacket(char* buff,int ilen);
	bool ParseMediaInfo(uint8_t *buff,int ilen);

	void Adjust_PMT_table(TS_PMT* packet ,unsigned char *buffer);
	int Adjust_PAT_table(TS_PAT* packet ,unsigned char *buffer);
	void Adjust_TS_packet_header(TS_packet_Header* pHeader,unsigned char *buffer);

	bool Adjust_PES_Pakcet(unsigned char *buffer,int ilen);
	int ParseStreamInfo(uint8_t *buff,int ilen);
	
	uint64_t Parse_PTS(unsigned char *pBuf);

	bool GetVideoESInfo(unsigned char *pBuf,int itempLen);

	bool ParseH264ES(unsigned char* pBuf,int itemplen);

	bool Find_Stream_IFrame(unsigned char *buffer,int ilen);

	int FilterRTPData(char* buff,int ilen);

	//������������
	bool Set_tsRate_period(int iperiod);

	//��ȡ������
	bool Get_tsRate(int* iRate);

	bool Get_tsIFrame_size(int* iSize);

	bool m_bNeedControlPlay; //ʶ���Ƿ�rtp����

	TSstreamInfo m_tsStreamparse;

	//HANDLE ThreadHandle;

	int m_iPMTPID;
	int m_iVideoPID;
	int m_iAudioPID;
	int m_iServerPID;
	int m_iPCRPID;

	MapPIDStreamType m_mapStreamPID;

	bool m_bHasPTS ;
	bool m_bHasDTS ;
	uint64_t m_llPCR;
	uint64_t m_llDts;
	uint64_t m_llPts;
	uint64_t m_llLastPts; //���ڼ���֡��

	double m_iFrameRate;

	int m_iFramTotal;

	int m_iGopSize;

	int m_iperiod;//��������
	int m_iRate;//kb
	long long m_ltotalByte;
};








#endif

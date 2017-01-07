#ifndef __TSSTREAMINFO_H_
#define __TSSTREAMINFO_H_

#include "Common.h"

class TSstreamInfo
{
public:
	TSstreamInfo();
	~TSstreamInfo();

	bool GetVideoESInfo(unsigned char *pEsData,int itempLen);

	void Adjust_PMT_table(TS_PMT* packet ,unsigned char *buffer);
	int Adjust_PAT_table(TS_PAT* packet ,unsigned char *buffer);
	void Adjust_TS_packet_header(TS_packet_Header* pHeader,unsigned char *buffer);

	bool Adjust_PES_Pakcet(unsigned char *buffer,int ilen);
	int ParseStreamInfo(uint8_t *buff,int ilen);
	int RecordVideoInfo();
	void get_time(char*buff,int siz);

	int ParseStreamFrame(uint8_t *buff,int ilen);

	uint64_t Parse_PTS(unsigned char *pBuf);
	
	bool ParseH264ES(unsigned char* buffer,int itemplen);

	int64_t Get_PCR_Value(unsigned char* buffer,int itemplen);
	int Set_PCR_Value(unsigned char* buffer,int index,int64_t pcr_value);
	int Find_PCR_Index(unsigned char* buffer,int itemplen,int *iIndex);

	bool Find_Stream_IFrame(unsigned char *buffer,int ilen,int* pIndex);

	bool Find_IFrame(unsigned char *buffer,int ilen);

	int filternullpacket(uint8_t *buff,int ilen);
	
	int m_iPMTPID;
	int m_iVideoPID;
	int m_iAudioPID;
	int m_iServerPID;
	int m_iPCRPID;

	//pthread_cond_t cond;
//	sem_t m_sem_send;
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

	//FILE *m_logfp;
	FILE *m_Mediafp;

	//FILE * �յ�����һ֡���ݵ�ʱ����Ϣ
	FILE *m_FramDatafp;

	//pes֡ͷ��ʶ
	bool m_bPesBeginFlag;

	bool m_bIFramebeginFlag;
	

	MapPIDStreamType m_mapStreamPID;

	int m_itsStreamPacketNumber;
	int m_iIframeSize;
	int m_iframesize;
	
	bool m_bHasPTS ;
	bool m_bHasDTS ;
	uint64_t m_llPCR;
	uint64_t m_llDts;
	uint64_t m_llPts;
	uint64_t m_llLastPts; //���ڼ���֡��

	uint64_t m_llFrameTotal;

	uint64_t m_llFrameNum;
	pthread_mutex_t m_mutexlocker;

	double m_iFrameRate;

	int m_iFramTotal;

	int m_iGopSize;
	//��Ƶ��������
	
	int m_iVideoCodeType;
	//��Ƶ��������
	int m_iAudioCodeType;

	TS_PAT tmpTSPat; //pat table

private:	

};


#endif



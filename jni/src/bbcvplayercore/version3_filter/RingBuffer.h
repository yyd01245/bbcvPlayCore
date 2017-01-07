#ifndef __RINGBUFFER_H_
#define __RINGBUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

const int MINRINGBUFF = 1024*10;
const int MAXRINGBUFF = 1024*1024*1;
const int MINREADLENGHT = 128;

class RingBuffer
{
public:
	RingBuffer();
	~RingBuffer();

	//��ʼ��
	//<param iRingSize> in ����ָ��Ĵ�С
	//<return >  ���ز�����ʶ<0 ʧ��
	int InitRing(int iRingSize);

	//ȡ����
	//<param pDstBuff> out �������
	//<param iLen> in ����ָ��Ĵ�С
	//<return >  ���ز����ɹ����ݳ���<=0 ʧ��
	int GetBuffFromeRing(unsigned char* pDstBuff,int iLen);
	
	int outBuffFromeRing(unsigned char* pDstBuff,int iLen);
		
	int putBuffToRing(unsigned char* pDstBuff,int iLen);	

	//������
	//<param pDstBuff> out ��������
	//<param iLen> in ����ָ��Ĵ�С
	//<return >  ���ز����ɹ����ݳ���<=0 ʧ��
	int InputBuffToRing(unsigned char* pDstBuff,int iLen);	

	//���³�ʼ��
	//<param iRingSize> in ����ָ��Ĵ�С
	//<return >  ���ز�����ʶ<0 ʧ��
	int ReInitRing(int iRingreSize);

	//��������������ݣ����ö�дλ��
	//<return >  ���ز�����ʶ<0 ʧ��
	int ClearRing();	

	//��ȡʹ����
	//<return >ʹ����
	float GetUsedRate();
	

	unsigned char* m_pRingBuff;

	//�������ܴ�С
	unsigned int m_iRingBuffSize;
	//д��������λ�ñ�ʶ
	unsigned int m_iWriteIndex;
	//����������λ�ñ�ʶ
	unsigned int m_iReadIndex;

	//����������
	pthread_mutex_t m_mutex;

};


#endif


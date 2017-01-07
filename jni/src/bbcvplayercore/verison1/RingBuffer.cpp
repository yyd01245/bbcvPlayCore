#include "RingBuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


RingBuffer::RingBuffer()
{
	m_pRingBuff = NULL;
	m_iReadIndex = 0;
	m_iWriteIndex = 0;
	m_iRingBuffSize= 0;
}
RingBuffer::~RingBuffer()
{
	ClearRing();
	if(m_pRingBuff)
	{
		free(m_pRingBuff);
	}
	m_pRingBuff = NULL;

}

//��ȡʹ����
//<return >ʹ����
float RingBuffer::GetUsedRate()
{
	
	int iUsedLen = (m_iWriteIndex + m_iRingBuffSize - m_iReadIndex)%m_iRingBuffSize;
	float fusedRate = iUsedLen/m_iRingBuffSize;
	return fusedRate;
}


//��ʼ��
//<param iRingSize> in ����ָ��Ĵ�С
//<return >  ���ز�����ʶ<0 ʧ��
int RingBuffer::InitRing(int iRingSize)
{
	int iSize = iRingSize;
	if(iSize < MINRINGBUFF)
		iSize = MINRINGBUFF;
	if(iSize > MAXRINGBUFF)
		iSize = MAXRINGBUFF;
	
	m_pRingBuff = (unsigned char*)malloc(iSize);
	if(NULL == m_pRingBuff)
	{
		fprintf(stderr,"InitRing Error malloc %d %s %d \n",iSize,
				__FUNCTION__,__LINE__);
		return -1;
	}

	pthread_mutex_init(&m_mutex,0);
	m_iReadIndex = 0;
	m_iWriteIndex = 0;
	m_iRingBuffSize= iSize-1;
	return 0;
}

//ȡ����
//<param pDstBuff> out �������
//<param iLen> in ����ָ��Ĵ�С
//<return >  ���ز����ɹ����ݳ���<=0 ʧ��
int RingBuffer::GetBuffFromeRing(unsigned char* pDstBuff,int iLen)
{
	if(NULL == m_pRingBuff)
	{
		fprintf(stderr,"GetBuffFromeRing Error %s %d \n",
				__FUNCTION__,__LINE__);
		return -1;
	}
	pthread_mutex_lock(&m_mutex);
	int iUsedLen = (m_iWriteIndex + m_iRingBuffSize - m_iReadIndex)%m_iRingBuffSize;

	int iOptionLen = iLen;
	if(iUsedLen < MINREADLENGHT)
	{
		//С����С��ȡ���ȿ��û�������ʾ����
		//iOptionLen = m_iRingBuffSize - iUsedLen;
		fprintf(stderr,"%s %d Waring,recv is slow or send too fast!\n",__FUNCTION__,__LINE__);
		return -2;
	}

	//��������������

	if(m_iReadIndex > m_iWriteIndex )
	{
		int iDstLen = m_iRingBuffSize - m_iReadIndex;
		if(iDstLen < iOptionLen)
		{
			//�ֶο���
			memcpy(pDstBuff,m_pRingBuff+m_iReadIndex,iDstLen);
			m_iReadIndex += iDstLen;
			m_iReadIndex = m_iReadIndex%m_iRingBuffSize;
			int iSecondLen = iOptionLen - iDstLen;
			memcpy(pDstBuff,m_pRingBuff+m_iReadIndex,iSecondLen);
			m_iReadIndex += iSecondLen;
			m_iReadIndex = m_iReadIndex%m_iRingBuffSize;
		}
		else
		{
			memcpy(pDstBuff,m_pRingBuff+m_iReadIndex,iOptionLen);
			m_iReadIndex += iOptionLen;
			m_iReadIndex = m_iReadIndex%m_iRingBuffSize;

		}
			
	}
	else
	{
		int iDstLen = m_iWriteIndex - m_iReadIndex;
		if(iDstLen < iOptionLen)
		{
			iOptionLen = iDstLen;
		}

		memcpy(pDstBuff,m_pRingBuff+m_iReadIndex,iOptionLen);
		m_iReadIndex += iOptionLen;
		m_iReadIndex = m_iReadIndex%m_iRingBuffSize;

	}
	pthread_mutex_unlock(&m_mutex);

	return iOptionLen;
}
	

//������
//<param pDstBuff> out ��������
//<param iLen> in ����ָ��Ĵ�С
//<return >  ���ز����ɹ����ݳ���<=0 ʧ��
int RingBuffer::InputBuffToRing(unsigned char* pDstBuff,int iLen)
{
	if(NULL == m_pRingBuff)
	{
		fprintf(stderr,"InputBuffToRing Error %s %d \n",
				__FUNCTION__,__LINE__);
		return -1;
	}
	pthread_mutex_lock(&m_mutex);
	int iUsedLen = (m_iWriteIndex + m_iRingBuffSize - m_iReadIndex)%m_iRingBuffSize;
	int iOptionLen = iLen;
	if(iUsedLen + iLen > m_iRingBuffSize)
	{
		//���ڿ��û�������ʾ����
		//iOptionLen = m_iRingBuffSize - iUsedLen;
		fprintf(stderr,"%s %d Waring,buffer is too small,or send is slow,it need reset!\n",__FUNCTION__,__LINE__);
		return -2;
	}

	//��������������
	int iDstLen = m_iRingBuffSize - m_iWriteIndex;
	if(iDstLen < iOptionLen)
	{
		//�ֶο���
		memcpy(m_pRingBuff+m_iWriteIndex,pDstBuff,iDstLen);
		m_iWriteIndex += iDstLen;
		m_iWriteIndex = m_iWriteIndex%m_iRingBuffSize;
		int iSecondLen = iOptionLen - iDstLen;
		memcpy(m_pRingBuff+m_iWriteIndex,pDstBuff,iSecondLen);
		m_iWriteIndex += iSecondLen;
		m_iWriteIndex = m_iWriteIndex%m_iRingBuffSize;
	}
	else
	{
		memcpy(m_pRingBuff+m_iWriteIndex,pDstBuff,iOptionLen);
		m_iWriteIndex += iOptionLen;
		m_iWriteIndex = m_iWriteIndex%m_iRingBuffSize;

	}

	pthread_mutex_unlock(&m_mutex);

	return iOptionLen;
}

//���³�ʼ��
//<param iRingSize> in ����ָ��Ĵ�С
//<return >  ���ز�����ʶ<0 ʧ��
int RingBuffer::ReInitRing(int iRingreSize)
{
	ClearRing();
	//�ͷ�
	
	return 0;
}

//��������������ݣ����ö�дλ��
//<return >  ���ز�����ʶ<0 ʧ��
int RingBuffer::ClearRing()
{
	if(NULL == m_pRingBuff)
	{
		fprintf(stderr,"ClearRing Error  %s %d \n",
				__FUNCTION__,__LINE__);
		return -1;
	}
	pthread_mutex_lock(&m_mutex);

	m_iReadIndex = 0;
	m_iWriteIndex = 0;

	pthread_mutex_unlock(&m_mutex);


}



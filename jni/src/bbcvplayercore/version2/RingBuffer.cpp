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

#include "DataHead.h"

#define min(A,B) (A)<(B)?(A):(B)

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
		#ifdef ANDROID_PLAMT
		LOGD("InitRing Error malloc %d %s %d \n",iSize,__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"InitRing Error malloc %d %s %d \n",iSize,__FUNCTION__,__LINE__);
	#endif

		return -1;
	}

	pthread_mutex_init(&m_mutex,0);
	m_iReadIndex = 0;
	m_iWriteIndex = 0;
	m_iRingBuffSize= iSize;
	LOGD("init ring buffer success size=%ld \n",m_iRingBuffSize);
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
		#ifdef ANDROID_PLAMT
		LOGD("GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#endif
		return -1;
	}

	int iUsedLen = (m_iWriteIndex + m_iRingBuffSize - m_iReadIndex)%m_iRingBuffSize;
	LOGD("get ring buffer Usedlen = %d \n",iUsedLen);
	int iOptionLen = iLen;
	int minNeedLen = iUsedLen > iLen ? iLen : iUsedLen;
	if( minNeedLen < MINREADLENGHT)
	{
		//С����С��ȡ���ȿ��û�������ʾ����
		//iOptionLen = m_iRingBuffSize - iUsedLen;
	#ifdef ANDROID_PLAMT
		LOGD("%s %d Waring,recv is slow or send too fast!\n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"%s %d Waring,recv is slow or send too fast!\n",__FUNCTION__,__LINE__);
	#endif
		return -2;
	}
	LOGD("get ring buffer = %d \n",iLen);
	//��������������
	pthread_mutex_lock(&m_mutex);
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

int RingBuffer::outBuffFromeRing(unsigned char* pDstBuff,int iLen)
{
	if(NULL == m_pRingBuff)
	{
		#ifdef ANDROID_PLAMT
		LOGD("GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#endif
		return -1;
	}

	LOGD("get ring buffer org len = %d \n",iLen);
	pthread_mutex_lock(&m_mutex);
	//can read len
	int iUsedLen = m_iWriteIndex + m_iRingBuffSize - m_iReadIndex;
	iLen = min(iLen,iUsedLen);
	LOGD("get ring buffer set len = %d \n",iLen);
	int iEndlen = m_iRingBuffSize - (m_iReadIndex)%m_iRingBuffSize;
	int ifirstlen = min(iLen,iEndlen);
	LOGD("get ring buffer first len = %d \n",iLen);
	memcpy(pDstBuff,m_pRingBuff+m_iReadIndex%m_iRingBuffSize,ifirstlen);
	memcpy(pDstBuff+ifirstlen,m_pRingBuff,iLen-ifirstlen);	
	m_iReadIndex += iLen;
	pthread_mutex_unlock(&m_mutex);
	LOGD("get ring buffer = %d \n",iLen);
	return iLen;
}

	
int RingBuffer::putBuffToRing(unsigned char* pDstBuff,int iLen)
{
	if(NULL == m_pRingBuff)
	{
		#ifdef ANDROID_PLAMT
		LOGD("GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#endif
		return -1;
	}

	LOGD("input ring buffer org len =%d \n",iLen);
	pthread_mutex_lock(&m_mutex);
	//can used to input len
	int iUsedLen =  m_iRingBuffSize + m_iReadIndex - m_iWriteIndex ;
	iLen = min(iLen,iUsedLen);
	LOGD("input ring buffer set len =%d \n",iLen);
	//write index to size end len
	int iEndLen = m_iRingBuffSize - (m_iWriteIndex)%m_iRingBuffSize;
	int ifirstlen = min(iLen,iEndLen);	
	memcpy(m_pRingBuff+(m_iWriteIndex)%m_iRingBuffSize,pDstBuff,ifirstlen);
	//then put the rest at the beginning 
	memcpy(m_pRingBuff,pDstBuff+ifirstlen,iLen-ifirstlen);
	
	//must set unsigned int ,overlow canbe set 0 auto
	m_iWriteIndex += iLen;

	pthread_mutex_unlock(&m_mutex);
	LOGD("input ring buffer len=%d windex=%ld,rindex=%d \n",iLen,m_iWriteIndex,m_iReadIndex);
	
	return iLen;
}



//������
//<param pDstBuff> out ��������
//<param iLen> in ����ָ��Ĵ�С
//<return >  ���ز����ɹ����ݳ���<=0 ʧ��
int RingBuffer::InputBuffToRing(unsigned char* pDstBuff,int iLen)
{
	if(NULL == m_pRingBuff)
	{
		#ifdef ANDROID_PLAMT
		LOGD("GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"GetBuffFromeRing Error %s %d \n",__FUNCTION__,__LINE__);
	#endif
		return -1;
	}

	LOGD("input ring buffer len =%d \n",iLen);
	int iUsedLen = (m_iWriteIndex + m_iRingBuffSize - m_iReadIndex)%m_iRingBuffSize;
	int iOptionLen = iLen;
	if(iUsedLen + iLen > m_iRingBuffSize)
	{
		//���ڿ��û�������ʾ����
		//iOptionLen = m_iRingBuffSize - iUsedLen;
	#ifdef ANDROID_PLAMT
		LOGD("ilen=%d usedlen=%d Waring,buffer is too small,or send is slow,it need reset!\n",iLen,iUsedLen);
	#else
		fprintf(stderr,"%s %d Waring,buffer is too small,or send is slow,it need reset!\n",__FUNCTION__,__LINE__);
	#endif

		return -2;
	}
	pthread_mutex_lock(&m_mutex);
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
	LOGD("input ring buffer len=%d windex=%ld,rindex=%d \n",iOptionLen,
		m_iWriteIndex,m_iReadIndex);
	
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
		#ifdef ANDROID_PLAMT
		LOGD("ClearRing Error  %s %d \n",__FUNCTION__,__LINE__);
	#else
		fprintf(stderr,"ClearRing Error  %s %d \n",__FUNCTION__,__LINE__);
	#endif
		return -1;
	}
	pthread_mutex_lock(&m_mutex);

	m_iReadIndex = 0;
	m_iWriteIndex = 0;

	pthread_mutex_unlock(&m_mutex);


}


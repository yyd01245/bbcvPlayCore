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

	//初始化
	//<param iRingSize> in 输入指针的大小
	//<return >  返回操作标识<0 失败
	int InitRing(int iRingSize);

	//取数据
	//<param pDstBuff> out 输出数据
	//<param iLen> in 输入指针的大小
	//<return >  返回操作成功数据长度<=0 失败
	int GetBuffFromeRing(unsigned char* pDstBuff,int iLen);
	
	int outBuffFromeRing(unsigned char* pDstBuff,int iLen);
		
	int putBuffToRing(unsigned char* pDstBuff,int iLen);	

	//存数据
	//<param pDstBuff> out 输入数据
	//<param iLen> in 输入指针的大小
	//<return >  返回操作成功数据长度<=0 失败
	int InputBuffToRing(unsigned char* pDstBuff,int iLen);	

	//重新初始化
	//<param iRingSize> in 输入指针的大小
	//<return >  返回操作标识<0 失败
	int ReInitRing(int iRingreSize);

	//重置数据清空数据，重置读写位点
	//<return >  返回操作标识<0 失败
	int ClearRing();	

	//获取使用率
	//<return >使用率
	float GetUsedRate();
	

	unsigned char* m_pRingBuff;

	//缓冲区总大小
	unsigned int m_iRingBuffSize;
	//写缓冲区的位置标识
	unsigned int m_iWriteIndex;
	//读缓冲区的位置标识
	unsigned int m_iReadIndex;

	//锁定缓冲区
	pthread_mutex_t m_mutex;

};


#endif


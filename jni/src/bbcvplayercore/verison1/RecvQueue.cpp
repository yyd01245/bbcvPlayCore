
#include "RecvQueue.h"
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#include <unistd.h>
//#include <time.h>

const int TS_PACKET_SIZE = 188;

FILE *fpdumxer = NULL;
static int64_t GetTickCount_r()
{
	struct timeval tv1;
	long long start_time;
	
	gettimeofday(&tv1,NULL);
	start_time = tv1.tv_sec*1000+tv1.tv_usec/1000; //ms
	return start_time;
}

NewQueue::NewQueue(int iport)
{
	bufsize = 0;
	write_ptr = 0;
	read_ptr = 0;
	buf = NULL;
	m_iport = iport;
	m_boverlay = false;
	m_hsIDRFrame = false;
	m_bIsOverlaying = false;
	m_bInitDecoder = false;
	m_bDelayFrame = false;

	m_iperiod = 1000;
	m_iRate = 0;
	m_iPMTPID = 0;
	m_iVideoPID = 0;
	m_iAudioPID = 0;
	m_iPCRPID = 0;

	m_llDts = 0;
	m_llPts = 0;
	m_llLastPts = 0;
	m_iFrameRate = 0;
	m_iGopSize = 0;
	m_iFramTotal = 0;

	pthread_mutex_init(&locker,NULL);
	
	//if(NULL == fpdumxer)
	//	fpdumxer = fopen("dumxer.pes","w+b");
}

NewQueue::~NewQueue()
{
	int iloop = 5;
	while(iloop-- && udp_recv_thread !=0)
	{
		m_bstop = true;
		usleep(1000*1000);
	}
	clean_RecvQue();
	free_queue();

}

int NewQueue::FilterRTPData(char* buff,int ilen)
{
	bool ret = false;
	//rtsp �����rtpͷ
	int iFilterLen = 0;
	RTPHead tmRtpHead;
	memset(&tmRtpHead,0,sizeof(tmRtpHead));
	unsigned char firstChar = *buff;
	tmRtpHead.Version = firstChar>>6 ;
	tmRtpHead.PayloadFlag = (firstChar<<2) >>7;
	tmRtpHead.ExternData =	(firstChar<<3) >>7;
	tmRtpHead.CSRCCount = (firstChar<<4) >>4;

	int iCSRCLen = tmRtpHead.CSRCCount * 4;
	iFilterLen = 12 +iCSRCLen;
	int iExterLen = 0;
	if(tmRtpHead.ExternData == 1)
	{
		char *ptmp = buff+iFilterLen;
		int iexterlen = (int)(ptmp[2]<<8);
		iexterlen = iexterlen | ptmp[3] ;
	}

	return iFilterLen;
}

//FILE *fprcv = NULL;
void* NewQueue::udp_ts_recv(void* param)
{
	LOGD("udp_ts_recv thread begin \n");
	NewQueue* this0 = (NewQueue*)param;
	struct sockaddr_in s_addr;
	struct sockaddr_in c_addr;
	int sock;
	socklen_t addr_len;
	int len;
	uint8_t UDP_buf[4096];
	FILE *fp;

	//fp = fopen("out.ts", "wb+");

	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0))  == -1) {
		perror("socket");
		LOGD("create socket error num: %d.\n\r",errno);
		exit(errno);
	} else
		LOGD("create socket success.\n\r");

	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;

	s_addr.sin_port = htons(this0->m_iport);


	s_addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in send_addr;
	send_addr.sin_family=AF_INET;
	send_addr.sin_port = htons(this0->m_iSendPort);
	send_addr.sin_addr.s_addr = inet_addr(this0->m_cdstIP);

		LOGD("begin bind socket .\n");
	if ( (bind(sock, (struct sockaddr*)&s_addr, sizeof(s_addr))) == -1 ) {
		perror("bind");
		LOGD("bind address to socket failed .\n");
		exit(errno);
	}else
		LOGD("bind address to socket success.\n");

	int nRecvBuf = 0;
	socklen_t iLen = 4;
	getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &nRecvBuf, &iLen);
	nRecvBuf = 1024*1024;//����???a
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,sizeof(socklen_t));
	int nSize = 0;
	getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &nSize, &iLen);

	LOGD("begin recv data=========port =%d,nsize=%d \n",this0->m_iport,nSize);

	//LOGD("begin recv data=========port =%d,nsize=%d \n",this0->m_iport,nSize);
	FILE* fpLog = this0->m_logfp;
	/*
	if(fpLog)
	{
		SYSTEMTIME systm;
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fprintf(fpLog,"%2d:%2d:%3d �������ݶ˿�:%d SO_RCVBUF=%d ������%d������\n",systm.wMinute,systm.wSecond,systm.wMilliseconds,this0->m_iport,nSize,this0->bufsize);
		fflush(fpLog);
	}*/

	//if(fprcv ==NULL)
	//	fprcv = fopen("rcv.ts","wb");
	
	addr_len = sizeof(struct sockaddr);
	bool bneedwait = false;

	int iFilterLen = 0;
	this0->m_ltotalByte = 0;

	uint64_t lptime1;
	uint64_t lptime2;

	lptime1 = GetTickCount_r();
	while(!this0->m_bstop) {
		len = recvfrom(sock, UDP_buf, sizeof(UDP_buf)-1, 0, (struct sockaddr*)&c_addr, &addr_len);
		
		//len = recvfrom(sock, UDP_buf, 1500, 0, (struct sockaddr*)&c_addr, &addr_len);
		if (len < 0) {
			usleep(100);
			//perror("recvfrom");
			//exit(errno);
			LOGD("recvfrom error \n");
			continue;
		}
		iFilterLen = 0;
		//����Ƿ�RTP����
		if(this0->m_bNeedControlPlay)
		{
			//rtsp �����rtpͷ
			LOGD("controlpaly filter rtp data head\n");
			iFilterLen = this0->FilterRTPData((char*)UDP_buf,len);
		}
		//QueryPerformanceCounter(&lptime2);
		lptime2 = GetTickCount_r();

	//	fwrite(UDP_buf+iFilterLen, 1, len-iFilterLen, fprcv);
//		if(!this0->m_bNeedControlPlay)
//			this0->ParseStreamInfo((uint8_t*)UDP_buf+iFilterLen,len-iFilterLen);
//		bool bret = this0->Find_Stream_IFrame((uint8_t*)UDP_buf+iFilterLen,len-iFilterLen);

		this0->put_queue( (uint8_t*)UDP_buf+iFilterLen, len-iFilterLen);
		this0->m_ltotalByte += len;
		int iIternval = lptime2-lptime1;
		if(iIternval >= this0->m_iperiod)
		{
			lptime1 = lptime2;
			this0->m_iRate = (this0->m_ltotalByte * 8*1000/iIternval )/1024;
			this0->m_ltotalByte = 0;
		}

	//	this0->m_tsStreamparse.ParseStreamFrame((uint8_t*)UDP_buf+iFilterLen, len-iFilterLen);
		//this0->filterNullPacket(UDP_buf,len);

		//bool bhandle = false;
		//LOGD("======recv size = %d",len);
//		fwrite(buf, sizeof(char), len, fp);
//		LOGD("recv from %s:%d,msg len:%d\n\r", inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), len);

/*		if(this0->m_bDelayFrame)
		{
			int ihandleLen = 0;
			bhandle = this0->dumxer(UDP_buf,len,&ihandleLen,1);
			
			if(bhandle)
			{
				LOGD("------wait a frame delay \n");
				this0->m_bDelayFrame=false;
				int iret = sendto(sock,UDP_buf,len,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
				if(iret < 0)
				{
					perror("sendto");
					exit(errno);
				}
				//���沿����Ҫ�ͽ���
				this0->put_queue( UDP_buf, len);
			}
		}
*/		
/*	
		if((this0->m_boverlay && !this0->m_hsIDRFrame) )//|| bneedwait
		{

			int ihandleLen = 0;
			
			bhandle = this0->dumxer(UDP_buf,len,&ihandleLen);
			if(bhandle)
			{
			//	struct timeval tm;
				
			//	gettimeofday(&tm,NULL);
			//	LOGD("-----change to overlay =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
				this0->m_bIsOverlaying = true;
				this0->m_hsIDRFrame = true;
				bneedwait = !bneedwait;
				this0->m_bDelayFrame = false;
				LOGD("------dumxer handle len =%d \n",ihandleLen);
				//fwrite(UDP_buf+ihandleLen,1,len-ihandleLen,fpdumxer);
				//fflush(fpdumxer);

				//fwrite(UDP_buf,1,len,fpdumxer);
				//fflush(fpdumxer);
				//if(!bneedwait)
				{
					//ǰ�沿�ֲ���I֡����Ҫ���ͳ�ȥ
					int iret = sendto(sock,UDP_buf,ihandleLen,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
					if(iret < 0)
					{
						perror("sendto");
						exit(errno);
					}
					//���沿����Ҫ�ͽ���
					this0->put_queue( UDP_buf, len);
				}
			//else
			
		
				{
					if(ihandleLen > 0)
					{
						int iret = sendto(sock,UDP_buf,ihandleLen,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
						if(iret < 0)
						{
							perror("sendto");
							exit(errno);
						}
					}
					//���沿����Ҫ�ͽ���
					this0->put_queue( UDP_buf, len);
				}
		
				

			}
			//��Ҫ����������������������ִ���
			
		}
		else if((!this0->m_boverlay && this0->m_bIsOverlaying))
		{

			int ihandleLen = 0;
			this0->m_bDelayFrame = true;
			
			bhandle = this0->dumxer(UDP_buf,len,&ihandleLen);
			if(bhandle)
			{
				//��Ҫ����������������������ִ���
				this0->m_bIsOverlaying = false;
				this0->m_hsIDRFrame = false;				
				
				bneedwait = !bneedwait;
					//LOGD("------dumxer handle len =%d \n",ihandleLen);
				//fwrite(UDP_buf+ihandleLen,1,len-ihandleLen,fp);
				//fflush(fp);
				
					//ǰ�沿�ֲ���I֡����Ҫ�ͽ���
				this0->put_queue( UDP_buf, ihandleLen);
				//���沿����Ҫ���ͳ�ȥ 
				int iret = sendto(sock,UDP_buf+ihandleLen,len-ihandleLen,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
				if(iret < 0)
				{
					perror("sendto");
					exit(errno);
				}
				


				


				struct timeval tm;

				gettimeofday(&tm,NULL);
				LOGD("-----change to send  =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
				this0->put_queue( UDP_buf, len);

				//��tssmoothģ�鷢������֡����
				int mswait = 1000*40; //40 ms
				struct timespec ts;                         
	            clock_gettime(CLOCK_REALTIME, &ts );    //��ȡ��ǰʱ��
                ts.tv_sec += (mswait / 1000 * 1000 *1000);        //���ϵȴ�ʱ�������
                ts.tv_nsec += ( mswait % 1000 ) * 1000; //���ϵȴ�ʱ��������
                int rv = 0;
                rv=sem_timedwait( &this0->m_sem_send, &ts );

			//	sem_wait(&this0->m_sem_send);
				gettimeofday(&tm,NULL);
				LOGD("-----change to send 222 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
				//���沿����Ҫ���ͳ�ȥ
				this0->m_bDelayFrame = false;
				int iret = sendto(sock,UDP_buf+ihandleLen,len-ihandleLen,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
				if(iret < 0)
				{
					perror("sendto");
					exit(errno);
				}
					//usleep(5*1000);
			
			}	
		}
			
		if(!bhandle)
		{
			if((this0->m_boverlay && this0->m_hsIDRFrame)  ||(!this0->m_boverlay && this0->m_bIsOverlaying)){
				this0->put_queue( UDP_buf, len);
				if(!this0->m_boverlay && this0->m_bIsOverlaying)
				{
					int iret = sendto(sock,UDP_buf,len,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
					if(iret < 0)
					{
						perror("sendto");
						exit(errno);
					}
				}
			}
			else if(!this0->m_boverlay && !this0->m_bIsOverlaying )
			{
				if(!this0->m_bInitDecoder)
				{
					//LOGD("--------init data copy\n");
					this0->put_queue( UDP_buf, len);
				}
				int iret = sendto(sock,UDP_buf,len,0,(struct sockaddr*)&send_addr,sizeof(send_addr));
				if(iret < 0)
				{
					perror("sendto");
					exit(errno);
				}
			}
		}

		*/
	}
//	fclose(fp);
	close(sock);
	this0->udp_recv_thread = 0;
	return NULL;
	
}

void NewQueue::init_queue( int size,int  iport,const char* dstip,short isendPort,FILE* fp,FILE* fpInfo,bool bNeedControlPlay)
{
	LOGD("yyd info init_queue iport=%d \n",iport);
	pthread_mutex_init(&locker, NULL);
	//pthread_cond_init(&cond, NULL);
	buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
	read_ptr = write_ptr = 0;
	bufsize = size;
	m_iport = iport;
	m_iSendPort = isendPort;
	memset(m_cdstIP,0,sizeof(m_cdstIP));
	strcpy(m_cdstIP,dstip);
	m_boverlay = false;
	m_bstop = false;
	m_logfp = fp;
	m_Mediafp = fpInfo;
	m_bNeedControlPlay = bNeedControlPlay;
	
	//
	pthread_t udp_recv_thread;
	pthread_create(&udp_recv_thread, NULL, udp_ts_recv, this);
	LOGD("create udp_ts_recv thread over\n");
	pthread_detach(udp_recv_thread);
}

void NewQueue::free_queue()
{
	if(buf)
		free(buf);
	pthread_mutex_destroy(&locker);
	//pthread_cond_destroy(&cond);
//	DeleteCriticalSection(&locker);

}



void NewQueue::put_queue(uint8_t* inputbuff, int size)
{
	uint8_t* dst = buf + write_ptr;
	//if(m_boverlay)
	//{
	//	fwrite(inputbuff,1,size,fpdumxer);
	//	fflush(fpdumxer);
	//}

	if((write_ptr > read_ptr && (read_ptr+bufsize < write_ptr+size)) || (write_ptr < read_ptr && (write_ptr+size > read_ptr)))
	//if((write_ptr-read_ptr >= bufsize-1) || (write_ptr <= read_ptr -2))
	{
		LOGD("====failed write_ptr < readptr\n");
		/*
		if(m_logfp)
		{
			SYSTEMTIME systm;
			memset(&systm,0,sizeof(systm));
			GetLocalTime(&systm);
			fprintf(m_logfp,"%2d:%2d:%3d ��������====failed write_ptr < readptr \n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(m_logfp);
		}*/
		usleep(100);
		
	}
	//LOGD("recv_data writelen=%d readlen=%d,buffsize=%d\n",write_ptr,read_ptr,bufsize);
	pthread_mutex_lock(&locker);

	if ((write_ptr + size) > bufsize) {
		memcpy(dst, inputbuff,(bufsize - write_ptr));
		memcpy(buf, inputbuff+(bufsize - write_ptr), size-(bufsize - write_ptr));
	} else {
		memcpy(dst, inputbuff, size*sizeof(uint8_t));
	}
	write_ptr = (write_ptr + size) % bufsize;

	//pthread_cond_signal(&cond);
	pthread_mutex_unlock(&locker);

}

int NewQueue::get_queue(uint8_t* outbuf, int size)
{
	uint8_t* src = buf + read_ptr;
	int wrap = 0;
	int irealLen = size;



	int pos = write_ptr;
//	if ( (read_ptr + size) > pos)
//	{
//		return -1;
//	}
	if(m_bstop)
		return -1;
	pthread_mutex_lock(&locker);
//	EnterCriticalSection(&locker);


	if (pos < read_ptr) {
		pos += bufsize;
		wrap = 1;
	}

	if ( (read_ptr + size) > pos) {
		//memcpy(outbuf, src, (bufsize - read_ptr));
		//LOGD("read recv data failed \n");
		pthread_mutex_unlock(&locker);
		return -1;
//		struct timespec timeout;
//		timeout.tv_sec=time(0)+1;
//		timeout.tv_nsec=0;
//		pthread_cond_timedwait(&que->cond, &que->locker, &timeout);
//		if ( (que->read_ptr + size) > pos ) {
//			pthread_mutex_unlock(&que->locker);
//			return 1;
//		}
	}

	if (wrap) {
	//	LOGD("wrap...\n");
/*		if(m_logfp)
		{
			SYSTEMTIME systm;
			memset(&systm,0,sizeof(systm));
			GetLocalTime(&systm);
			fprintf(m_logfp,"%2d:%2d:%3d ��������wrap...\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(m_logfp);
		}*/
		if(size > bufsize - read_ptr)
		{
			memcpy(outbuf, src, (bufsize - read_ptr));
			memcpy(outbuf+(bufsize - read_ptr), buf, size-(bufsize - read_ptr));
		}
		else
		{
			memcpy(outbuf, src, sizeof(uint8_t)*size);

		}
	} else {
		memcpy(outbuf, src, sizeof(uint8_t)*size);
	}
	read_ptr = (read_ptr + size) % bufsize;
	pthread_mutex_unlock(&locker);

	return 0;
}


bool NewQueue::set_tsDecoder_stat(bool bstat)
{
	if(m_boverlay != bstat)
	{

		m_boverlay= bstat;

		if(m_boverlay)
		{
			clean_RecvQue();
		}
		struct timeval tm;
		

		//gettimeofday(&tm,NULL);
		//LOGD("-----Set decoder %d stat =%ld\n",bstat,tm.tv_sec*1000+tm.tv_usec/1000);
		//m_hsIDRFrame = false;
		//m_bFirstDecodeSuccess = false;
	}
	return true;
}

void NewQueue::filterNullPacket(char* buff,int ilen)
{
	
	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	char *packet;
	char filterbuff[4096];

	int ifilterLen = 0; //ts len

	bool hasFindIframe = false;
	bool hasFindSPS = false;
	bool hasFindPPS = false;
	if(ilen <= TS_PACKET_SIZE)
	{
		// to que
		put_queue( (uint8_t*)buff, ilen);
		return ;
	}
	while(index < ilen-TS_PACKET_SIZE)
	{
		if(buff[index]==0x47 && buff[index+TS_PACKET_SIZE]==0x47)
		{
			//LOGD("-----find a ts packet \n");
			packet = &buff[index];
			int len, pid, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			unsigned short *sPID =  (unsigned short*)(packet + 1);
			pid = ntohs(*sPID)& 0x1fff;
			if(pid != 0x1fff)
			{
				//packet 
				memcpy(filterbuff+ifilterLen,packet,TS_PACKET_SIZE);
				ifilterLen += TS_PACKET_SIZE;
					
			}
				//LOGD("-------pid=%d\n",pid);
			
			index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}
	
	//
	if(ilen - index > 0)
	{
		memcpy(filterbuff+ifilterLen,buf+index,ilen - index);
		ifilterLen += (ilen - index);
	}

	// to que
	put_queue( (uint8_t*)filterbuff, ifilterLen);

}

bool NewQueue::dumxer(unsigned char * buff,int ilen,int *iHandleLen,int ineedflag)
{

	
	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	unsigned char *packet;
	const uint8_t *pESH264_IDR= NULL;

	int iseekLen = 0; //ts head len
	int iPesHeadLen = 0; //pes head len
	int iEsSeekLen = 0;
	int ifindLen =0;

	bool hasFindIframe = false;
	bool hasFindSPS = false;
	bool hasFindPPS = false;
	while(index < ilen && !hasFindIframe)
	{
		if(buff[index]==0x47 && buff[index+TS_PACKET_SIZE]==0x47)
		{
			hasFindIframe = false;
			hasFindSPS = false;
			hasFindPPS = false;
			//LOGD("-----find a ts packet \n");
			packet = &buff[index];
			if(packet[1] & 0x40)  //is_start = packet[1] & 0x40;
			{
				int len, pid, cc,  afc, is_start, is_discontinuity,
					has_adaptation, has_payload;
				unsigned short *sPID =  (unsigned short*)(packet + 1);
				pid = ntohs(*sPID)& 0x1fff;
				//LOGD("-------pid=%d\n",pid);
				

				afc = (packet[3] >> 4) & 3;//adaption
				
				if (afc == 0) /* reserved value */
					return 0;
				has_adaptation = afc & 2;
				has_payload = afc & 1;
				is_discontinuity = has_adaptation
					&& packet[4] != 0 /* with length > 0 */
					&& (packet[5] & 0x80); /* and discontinuity indicated */

				cc = (packet[3] & 0xf);
				const uint8_t *p, *p_end,*pEsData;

				p = packet + 4;
				if (has_adaptation) {
					/* skip adaptation field */
					p += p[0] + 1;
					/* if past the end of packet, ignore */
					p_end = packet + TS_PACKET_SIZE;
					if (p >= p_end)
						return 0;
				}
				//pes
				iseekLen = p - packet;
				if(p[0]==0x00&&p[1]==0x00&&p[2]==0x01&&p[3]==0xE0)
				{
					//pes len
					int ilen = p[4]<<8;
					ilen = ilen | p[5];
					//LOGD("-----------pes len =%d\n",ilen);

					iPesHeadLen=p[8] + 9;
					//LOGD("--------------pesheadlen=%d \n",iPesHeadLen);
					pEsData = p+iPesHeadLen;
					//
					//find 00000001 7 8 5
					int itempLen = TS_PACKET_SIZE - iseekLen - iPesHeadLen;
					
					const uint8_t *pData= NULL;
					ifindLen = 0;
					while(ifindLen < itempLen)
					{
						pData = pEsData + ifindLen;
						if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x00 && *(pData+3)==0x01 )
						{
								LOGD("------------4 0 flag bits =%0x\n",*(pData+4));
								int iflag = (*(pData+4))&0x1f;
								//LOGD("---------h264 nal type =%d \n",iflag);

							/*	if(iflag == ineedflag)
								{
									LOGD("----------find need flag \n");
									pESH264_IDR = pData - 5;
									hasFindIframe = true;
									hasFindSPS = true;
									hasFindPPS = true;
									break;
								}
						*/		
								if(iflag == 7)
								{
									LOGD("----------find sps \n");
									pESH264_IDR = pData - 5;
									hasFindSPS = true;
									//break;
								}
								else if(iflag == 8)
								{
									LOGD("-------find pps \n");
									pESH264_IDR = pData - 5;
									hasFindPPS = true;
									//break;
									
								}
								else if(iflag == 5)
								{
									LOGD("-------find IDR \n");
									pESH264_IDR = pData - 5;
									hasFindIframe = true;
									//break;
								}
								if(hasFindIframe && hasFindPPS && hasFindSPS)
								{
									break;
								}
								ifindLen += 5;
							
						}
						else if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x01 )
						{
								LOGD("------------3 0 flag bits =%0x\n",*(pData+3));
								int iflag = (*(pData+3))&0x1f;
								//LOGD("---------h264 nal type =%d \n",iflag);

							/*	if(iflag == ineedflag)
								{
									LOGD("----------find need flag \n");
									pESH264_IDR = pData - 4;
									hasFindIframe = true;
									break;
								}
								*/
								if(iflag == 7)
								{
									LOGD("----------find sps \n");
									pESH264_IDR = pData - 4;
									hasFindSPS = true;
									//break;
								}
								else if(iflag == 8)
								{
									LOGD("-------find pps \n");
									pESH264_IDR = pData - 4;
									hasFindPPS = true;
									//break;
									
								}
								else if(iflag == 5)
								{
									LOGD("-------find IDR \n");
									pESH264_IDR = pData - 4;
									hasFindIframe = true;
									//break;
								}
								if(hasFindIframe && hasFindPPS && hasFindSPS)
								{
									LOGD("----find all info \n");
									break;
								}
								ifindLen += 4;
									
							
						}
						else
						{
							ifindLen ++;
						}
						
					}
					
				}
			}
			if(!hasFindIframe)
				index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}
	if(hasFindIframe && hasFindPPS && hasFindSPS)
	{
		//int iseek_len = index /*+ pESH264_IDR - packet*/;

		LOGD("--------find IDR SPS  PPS index =%d tsheade len=%d peslen=%d eslen=%d \n",index,iseekLen,iPesHeadLen,ifindLen);
		*iHandleLen = index;
		// less len = ilen - iseek_len;
		//send data buff to iseek_len 

		return true;
	}

	return false;

}

void NewQueue::clean_RecvQue()
{
	pthread_mutex_lock(&locker);

	read_ptr = 0;
	write_ptr = 0;
	pthread_mutex_unlock(&locker);
}

bool NewQueue::ParseMediaInfo(uint8_t *buff,int ilen)
{

	
	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	unsigned char *packet;
	const uint8_t *pESH264_IDR= NULL;

	int iseekLen = 0; //ts head len
	int iPesHeadLen = 0; //pes head len
	int iEsSeekLen = 0;
	int ifindLen =0;

	bool hasFindIframe = false;
	bool hasFindSPS = false;
	bool hasFindPPS = false;
	while(index < ilen && !hasFindIframe)
	{
		if(buff[index]==0x47 && buff[index+TS_PACKET_SIZE]==0x47)
		{
			hasFindIframe = false;
			hasFindSPS = false;
			hasFindPPS = false;
			//LOGD("-----find a ts packet \n");
			packet = &buff[index];


			int len, pid, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			unsigned short *sPID =  (unsigned short*)(packet + 1);
			pid = ntohs(*sPID)& 0x1fff;
			if(pid == 0x1fff)
			{
				//NULL packet
				return false;
			}

			if(m_iPMTPID == 0 && pid ==0 )
			{
				// PAT �ҵ�PMT��Ϣ
				//8+ 1+1+2+12+16+2+5+1+8+8
				//section_length 
				unsigned char *pat = packet+4;
				uint64_t iSection_length = (uint64_t)(pat[1]&0xf << 8);
				iSection_length |= pat[2];

				pat += 7;
				//section_number;
				uint64_t iSection_number = *pat;

				//��Ŀ
				//program_number 16 reserved 3 pid 13
				//Ĭ��Ϊһ��pmt
				pat += 3;
				//PMT pid
				m_iPMTPID = (int)((pat[0]&0x1f) << 8);
				m_iPMTPID |= pat[1];
				


			}
			else if(pid == m_iPMTPID && (m_iVideoPID ==0 || m_iAudioPID==0))
			{
				//����pid
				//8 +1+1+2+12+16+2+5+1+8+8
				unsigned char *pat = packet+4;
				uint64_t iSection_length = (uint64_t)(pat[1]&0xf << 8);
				iSection_length |= pat[2];

				pat += 7;
				//section_number;
				uint64_t iSection_number = *pat;

				//��Ŀ
				//program_number 16 reserved 3 pid 13
				//Ĭ��Ϊһ��pmt
				pat += 1;
				//PMT pid
				m_iPCRPID = (int)((pat[0]&0x1f) << 8);
				m_iPCRPID |= pat[1];

				//program info length
				pat += 2;
				int iprogram_info_length =0;
				iprogram_info_length = (int)((pat[0]&0x0f)<<8);
				iprogram_info_length |= pat[1];

				iprogram_info_length += 2;
				pat += iprogram_info_length;
				
				//stream type
				int iStreamType = *pat;
				pat += 1;

				//��Ƶ ����Ƶpid
				int iTmpPid = (int)((pat[0]&0x1f) << 8);
				iTmpPid |= pat[1];

				m_mapStreamPID.insert(MapPIDStreamType::value_type(iStreamType,iTmpPid));

				pat += 2;
				//
				iprogram_info_length =0;
				iprogram_info_length = (int)((pat[0]&0x0f)<<8);
				iprogram_info_length |= pat[1];
				iprogram_info_length += 2;
				pat += iprogram_info_length;




			}

			
			afc = (packet[3] >> 4) & 3;//adaption

			if (afc == 0) /* reserved value */
				return 0;

			has_adaptation = afc & 2;
			has_payload = afc & 1;
			is_discontinuity = has_adaptation
				&& packet[4] != 0 /* with length > 0 */
				&& (packet[5] & 0x80); /* and discontinuity indicated */

			cc = (packet[3] & 0xf);
			const uint8_t *p, *p_end,*pEsData;

			p = packet + 4;
			if (has_adaptation) {
				/* skip adaptation field */
				//p += p[0] + 1;  p[0]Ϊ�����ֶγ���
				int iPCRflag = p[1]&0x10;
				if(iPCRflag ==0x10)
				{
					//pcrflag 
					uint64_t pcr_high =(uint64_t)(p[2] << 25);
					pcr_high = pcr_high | (p[3] << 17);
					pcr_high = pcr_high | (p[4] << 9);
					pcr_high = pcr_high | (p[5] << 1);
					pcr_high = pcr_high | (p[6]&0x80)>>7;

					uint64_t pcr_low = (uint64_t)((p[6]&0x01)<<8);
					pcr_low == pcr_low | p[7];
					
					m_llPCR = pcr_high*300 + pcr_low;

					m_llPCR = m_llPCR /300; //����90khz
					//33bit base
					
				}
				
				p += p[0] + 1; // p[0]Ϊ�����ֶγ���
				/* if past the end of packet, ignore */
				p_end = packet + TS_PACKET_SIZE;
				if (p >= p_end)
					return 0;
			}
				
			if(packet[1] & 0x40)  //is_start = packet[1] & 0x40;
			{	
				//���� pes 
				iseekLen = p - packet;
				if(p[0]==0x00&&p[1]==0x00&&p[2]==0x01&&p[3]==0xE0)
				{
					//pes len
					int ilen = p[4]<<8;
					ilen = ilen | p[5];
					//LOGD("-----------pes len =%d\n",ilen);

					iPesHeadLen=p[8] + 9;
					//LOGD("--------------pesheadlen=%d \n",iPesHeadLen);
					pEsData = p+iPesHeadLen;
					//
					//find 00000001 7 8 5
					int itempLen = TS_PACKET_SIZE - iseekLen - iPesHeadLen;
					
					const uint8_t *pData= NULL;
					ifindLen = 0;
					while(ifindLen < itempLen)
					{
						pData = pEsData + ifindLen;
						if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x00 && *(pData+3)==0x01 )
						{
								LOGD("------------4 0 flag bits =%0x\n",*(pData+4));
								int iflag = (*(pData+4))&0x1f;
							
								if(iflag == 7)
								{
									LOGD("----------find sps \n");
									pESH264_IDR = pData - 5;
									hasFindSPS = true;
									//break;
								}
								else if(iflag == 8)
								{
									LOGD("-------find pps \n");
									pESH264_IDR = pData - 5;
									hasFindPPS = true;
									//break;
									
								}
								else if(iflag == 5)
								{
									LOGD("-------find IDR \n");
									pESH264_IDR = pData - 5;
									hasFindIframe = true;
									//break;
								}
								if(hasFindIframe && hasFindPPS && hasFindSPS)
								{
									break;
								}
								ifindLen += 5;
							
						}
						else if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x01 )
						{
								LOGD("------------3 0 flag bits =%0x\n",*(pData+3));
								int iflag = (*(pData+3))&0x1f;
								//LOGD("---------h264 nal type =%d \n",iflag);

								if(iflag == 7)
								{
									LOGD("----------find sps \n");
									pESH264_IDR = pData - 4;
									hasFindSPS = true;
									//break;
								}
								else if(iflag == 8)
								{
									LOGD("-------find pps \n");
									pESH264_IDR = pData - 4;
									hasFindPPS = true;
									//break;
									
								}
								else if(iflag == 5)
								{
									LOGD("-------find IDR \n");
									pESH264_IDR = pData - 4;
									hasFindIframe = true;
									//break;
								}
								if(hasFindIframe && hasFindPPS && hasFindSPS)
								{
									LOGD("----find all info \n");
									break;
								}
								ifindLen += 4;
									
							
						}
						else
						{
							ifindLen ++;
						}
						
					}
					
				}


			}
	
			if(!hasFindIframe)
				index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}

	return true;
}

void  NewQueue::Adjust_PMT_table(TS_PMT* packet ,unsigned char *buffer)
{
	int pos=12,len = 0;
	int i = 0;
	packet->table_id = buffer[0];
	packet->section_syntax_indicator = buffer[1]>>7;
	packet->zero = buffer[1]>>6 & 0x01;
	packet->reserved_1 = buffer[1]>>4 & 0x3;
	packet->section_length = (buffer[1]&0x0F) <<8 | buffer[2];
	packet->program_number = buffer[3] <<8 |buffer[4];
	packet->reserved_2 = buffer[5]>>6;
	packet->version_number = buffer[5]>>1 & 0x1F;
	packet->current_next_indicator = (buffer[5]<<7)>>7;
	packet->section_number = buffer[6];
	packet->last_section_number = buffer[7];
	packet->reserved_3 = buffer[8]>>5;

	packet->PCR_PID = ((buffer[8]<<8)|buffer[9]) & 0x1FFF;
	packet->reserved_4 = buffer[10]>>4;

	packet->program_info_length = (buffer[10] & 0x0F)<<8 | buffer[11];

	len = 3+packet->section_length;
	packet->CRC_32 = (buffer[len-4]&0x000000FF) <<24
		| (buffer[len-3] & 0x000000FF) << 16
		| (buffer[len-2] & 0x000000FF) << 8
		| (buffer[len-1] & 0x000000FF);

	if(packet->program_info_length != 0)
		pos += packet->program_info_length;

	for(;pos<=(packet->section_length+2)-4;)
	{
		packet->stream_type = buffer[pos];
		packet->reserved_5 = buffer[pos+1]>>5;
		packet->elementary_PID = ((buffer[pos+1] << 8) | buffer[pos+2])&0x1fff;
		packet->reserved_6 = buffer[pos+3] >> 4;
		packet->ES_info_length = (buffer[pos+3]&0x0F) << 8 | buffer[pos+4];
		
		m_mapStreamPID.insert(MapPIDStreamType::value_type(packet->stream_type,packet->elementary_PID));

		if(packet->stream_type == 0x1B)
		{
			// video
			m_iVideoPID = packet->elementary_PID;
		}
		else if(packet->stream_type == 0x03)
		{
			m_iAudioPID = packet->elementary_PID;
		}

		if(packet->ES_info_length !=0)
		{
			pos += 5;
			pos += packet->ES_info_length;

		}
		else
			pos += 5;
	}

}

int  NewQueue::Adjust_PAT_table(TS_PAT* packet ,unsigned char *buffer)
{
	int n=0,i=0;
	int len = 0;
	packet->table_id = buffer[0];
	packet->section_syntax_indicator = buffer[1]>>7;
	packet->zero = buffer[1]>>6 & 0x01;
	packet->reserved_1 = buffer[1]>>4 & 0x3;
	packet->section_length = (buffer[1]&0x0F) <<8 | buffer[2];
	packet->transport_stream_id = buffer[3] <<8 |buffer[4];
	packet->reserved_2 = buffer[5]>>6;
	packet->version_number = buffer[5]>>1 & 0x1F;
	packet->current_next_indicator = (buffer[5]<<7)>>7;
	packet->section_number = buffer[6];
	packet->last_section_number = buffer[7];
	if(packet->section_length >= TS_PACKET_SIZE-5)
		return -1;

	len = 3+packet->section_length;
	packet->CRC_32 = (buffer[len-4]&0x000000FF) <<24
					| (buffer[len-3] & 0x000000FF) << 16
					| (buffer[len-2] & 0x000000FF) << 8
					| (buffer[len-1] & 0x000000FF);

	for(n=0;n<packet->section_length-4;n++)
	{
		packet->program_number = buffer[8]<<8 |buffer[9];
		packet->reserved_3 = buffer[10]>>5;
		if(packet->program_number == 0x0)
		{
			packet->network_PID = (buffer[10]<<3)<<5|buffer[11];
			//packet->network_PID = (buffer[11]<<3)<<5|buffer[12];
		}
		else
			packet->program_map_PID = (buffer[10]<<3)<<5 |buffer[11];
		
		n += 5;
	}

	return 0;
}
void NewQueue::Adjust_TS_packet_header(TS_packet_Header* pHeader,unsigned char *buffer)
{
	if(pHeader ==NULL)
		return ;
	pHeader->transport_error_indicator = buffer[1] >> 7;
	pHeader->payload_unit_start_indicator = buffer[1]>>6 &0x01;
	pHeader->transport_prority = buffer[1] >> 5 & 0x01;
	pHeader->PID  = (buffer[1] &0x1F)<<8 | buffer[2];
	pHeader->transport_scrambling_control = buffer[3]>>6;
	pHeader->adaption_field_control = buffer[3] >> 4 & 0x03;
	pHeader->continuity_counter = buffer[3] & 0x03;

}


uint64_t NewQueue::Parse_PTS(unsigned char *pBuf)
{
	unsigned long long llpts = (((unsigned long long)(pBuf[0] & 0x0E)) << 29)
		| (unsigned long long)(pBuf[1] << 22)
		| (((unsigned long long)(pBuf[2] & 0xFE)) << 14)
		| (unsigned long long)(pBuf[3] << 7)
		| (unsigned long long)(pBuf[4] >> 1);
	return llpts;
}

bool NewQueue::GetVideoESInfo(unsigned char *pEsData,int itempLen)
{
	int ifindLen = 0;
	const uint8_t *pData= NULL;
	const uint8_t *pESH264_IDR= NULL;

	bool hasFindIframe = false;
	bool hasFindPFrame = false;
	bool hasFindPPS = false;
	bool hasFindSPS = false;
	while(ifindLen < itempLen)
	{
		pData = pEsData + ifindLen;
		if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x00 && *(pData+3)==0x01 )
		{
			LOGD("------------4 0 flag bits =%0x\n",*(pData+4));
			int iflag = (*(pData+4))&0x1f;

			if(iflag == 7)
			{
				LOGD("----------find sps \n");
				pESH264_IDR = pData - 5;
				hasFindSPS = true;
				//break;
			}
			else if(iflag == 8)
			{
				LOGD("-------find pps \n");
				pESH264_IDR = pData - 5;
				hasFindPPS = true;
				//break;

			}
			else if(iflag == 5)
			{
				LOGD("-------find IDR \n");
				pESH264_IDR = pData - 5;
				hasFindIframe = true;
			//	m_iGopSize++;
				//break;
			}
			else if(iflag == 1)
			{
				LOGD("-------find P frame or B frame \n");
				pESH264_IDR = pData - 5;
				hasFindPFrame = true;
				//break;
				m_iGopSize++;
				break;
			}
	
			if(hasFindIframe && hasFindSPS && hasFindIframe)
			{
				LOGD("----find all info \n");
				//��gopsize ���
				if(m_iGopSize != 0)
				{
					//��¼gopsize
			//		fprintf(m_Mediafp,"Gop size=%d frametotal=%d\n",m_iGopSize,m_iFramTotal);
			//		fflush(m_Mediafp);
				}
				m_iGopSize = 0;
				m_iFramTotal = 0;
				break;
			}
			ifindLen += 5;

		}
		else if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x01 )
		{
			LOGD("------------3 0 flag bits =%0x\n",*(pData+3));
			int iflag = (*(pData+3))&0x1f;
			//LOGD("---------h264 nal type =%d \n",iflagr);

			if(iflag == 7)
			{
				LOGD("----------find sps \n");
				pESH264_IDR = pData - 4;
				hasFindSPS = true;
				//break;
			}
			else if(iflag == 8)
			{
				LOGD("-------find pps \n");
				pESH264_IDR = pData - 4;
				hasFindPPS = true;
				//break;

			}
			else if(iflag == 5)
			{
				LOGD("-------find IDR \n");
				pESH264_IDR = pData - 4;
				//m_iGopSize++;
				hasFindIframe = true;
				//break;
			}
			else if(iflag == 1)
			{
				LOGD("-------find P frame or B frame \n");
				pESH264_IDR = pData - 4;
				hasFindPFrame = true;
				//break;
				m_iGopSize++;
				break;
			}

			if( hasFindPPS && hasFindSPS && hasFindIframe )
			{
				LOGD("----find all info \n");
				m_iGopSize++;
				//��gopsize ���
				if(m_iGopSize != 0)
				{
					//��¼gopsize
				//	fprintf(m_Mediafp,"Gop size=%d frametotal=%d\n",m_iGopSize,m_iFramTotal);
				//	fflush(m_Mediafp);
				}
				m_iGopSize = 0;
				m_iFramTotal = 0;
				break;
			}
			ifindLen += 4;


		}
		else
		{
			ifindLen ++;
		}

	}
	if(hasFindIframe || hasFindPFrame)
		return true;
	else
		return false;

}
bool NewQueue::Adjust_PES_Pakcet(unsigned char *p,int iseekLen)
{
	unsigned char *pEsData;
	if(p[0]==0x00&&p[1]==0x00&&p[2]==0x01)
	{
		if(p[3]==0xE0)
		{
			//video
			
			
		}
		else if(p[3]==0xC0)
		{
			//audio
			int i = 1;
			return false;
		}
		else
		{
			return false;
		}

		//pes len video
		int ilen = p[4]<<8 | p[5];
		//LOGD("-----------pes len =%d\n",ilen);
		int iptsflag = p[7]>>6;
		m_bHasDTS = false;
		m_bHasPTS = false;
		if(iptsflag == 3)
		{
			//both pts dts;
			m_bHasDTS = true;
			m_bHasPTS = true;
		}
		else if(iptsflag == 2)
		{
			m_bHasPTS = true;
		}
		else
		{
			// 01 forbidden 00 both no
		}
		if(m_bHasPTS)
		{
			m_llPts = Parse_PTS(p+9);
			m_iFramTotal++;
		//	fprintf(m_Mediafp,"PCR=%d \n",m_llPCR);
		//	fprintf(m_Mediafp,"PTS=%d \n",m_llPts);
		//	fprintf(m_Mediafp,"PTS-PCR=%d \n",m_llPts-m_llPCR);
		//	if(p[3]==0xE0)
			{
				double fps = 1000/((m_llPts - m_llLastPts)/90);
				if(m_iFrameRate != fps && m_llLastPts != 0 && fps <= 30 && fps >=15)
				{
				//	fprintf(m_Mediafp,"rate=%3.1f \n",fps);
					m_iFrameRate = fps;
				}
				m_llLastPts = m_llPts;
			}

			//fflush(m_Mediafp);
		}
		if(m_bHasDTS)
		{
			m_llDts = Parse_PTS(p+9+5);
			fprintf(m_Mediafp,"DTS=%d \n",m_llDts);
			fprintf(m_Mediafp,"DTS-PCR=%d \n",m_llDts-m_llPCR);
			
		}
		fflush(m_Mediafp);
		int iPesHeadLen=p[8] + 9;
		//LOGD("--------------pesheadlen=%d \n",iPesHeadLen);
		pEsData = p+iPesHeadLen;
		//
		//find 00000001 7 8 5
		int itempLen = TS_PACKET_SIZE - iseekLen - iPesHeadLen;

		//return GetVideoESInfo(pEsData,itempLen);
		return ParseH264ES(pEsData,itempLen);
	}

	return false;
}

//FILE* fptest = NULL;
bool NewQueue::ParseH264ES(unsigned char* buffer,int itemplen)
{
	int buf_index = 0;
	int next_avc = itemplen;
	unsigned char * pData = NULL;
//	if(fptest == NULL)
//		fptest = fopen("test.264","wb+");
	while(1)
	{
		//���ҵ���ʼ��
		for (; buf_index + 3 < next_avc; buf_index++)
			// This should always succeed in the first iteration.
			if (buffer[buf_index]     == 0 &&
				buffer[buf_index + 1] == 0 &&
				buffer[buf_index + 2] == 1)
				break;
		if (buf_index + 3 >= next_avc)
			return false;

		buf_index += 3;
		//������ǰ����
		pData = buffer + buf_index;
		int iflag = (*(pData))& 0x1f;

		if(iflag == 5)
		{
			// I frame
			m_iGopSize++;
			//��gopsize ���
			//if(m_iGopSize != 0)
			{
				//��¼gopsize
				fprintf(m_Mediafp,"Gop size=%d \n",m_iFramTotal);
				fflush(m_Mediafp);

			//	fwrite(pData,1,itemplen-5-buf_index,fptest);
			}
			m_iGopSize = 0;
			m_iFramTotal = 0;
			break;
		}
		else if(iflag==1)
		{
			m_iGopSize++;
		}


	}


	// 
	return true;
}


int NewQueue::ParseStreamInfo(uint8_t *buff,int ilen)
{

	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	unsigned char *packet;
	const uint8_t *pESH264_IDR= NULL;

	int iseekLen = 0; //ts head len
	int iPesHeadLen = 0; //pes head len
	int iEsSeekLen = 0;
	int ifindLen =0;

	bool hasFindIframe = false;
	bool hasFindSPS = false;
	bool hasFindPPS = false;
	while(index < ilen-TS_PACKET_SIZE && !hasFindIframe)
	{
		if(buff[index]==0x47 && buff[index+TS_PACKET_SIZE]==0x47)
		{
			hasFindIframe = false;
			hasFindSPS = false;
			hasFindPPS = false;
			//LOGD("-----find a ts packet \n");
			packet = &buff[index];

			TS_packet_Header tmpPacketHeader;
			memset(&tmpPacketHeader,0,sizeof(tmpPacketHeader));
			Adjust_TS_packet_header(&tmpPacketHeader,packet);

			int pid = tmpPacketHeader.PID ;
			int iPoint_fielLen = tmpPacketHeader.payload_unit_start_indicator;

			if(pid == 0x1fff)
			{
				//NULL packet
				return false;
			}
			int len, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			afc = tmpPacketHeader.adaption_field_control;
			uint8_t *p, *p_end,*pEsData;

			int iBeginlen = 4;
			int adaptation_field_length = packet[4];
			switch(tmpPacketHeader.adaption_field_control)
			{
			case 0x0:                                    // reserved for future use by ISO/IEC
				return false;
			case 0x1:                                    // �޵����ֶΣ�������Ч����  
				//iBeginlen += packet[iBeginlen] + 1;  // + pointer_field
				iBeginlen +=  0;  // + pointer_field
				break;
			case 0x2:                                     // ���������ֶΣ�����Ч����
			//	iBeginlen += packet[iBeginlen] + 1;  // + pointer_field
				iBeginlen += 0;  // + pointer_field
				break;
			case 0x3:									 // �����ֶκ���Ч����
				if (adaptation_field_length > 0) 
				{
					iBeginlen += 0;                   // adaptation_field_lengthռ8λ
					iBeginlen += adaptation_field_length; // + adaptation_field_length
				}
				else
				{
					iBeginlen += 0;
				}
				//iBeginlen += packet[iBeginlen] + 1;           // + pointer_field
				break;
			default:	
				break;

			}

			if(m_iPMTPID == 0 && pid ==0 )
			{
				// PAT �ҵ�PMT��Ϣ
				//8+ 1+1+2+12+16+2+5+1+8+8
				//section_length 

				TS_PAT tmpTSPat;
				memset(&tmpTSPat,0,sizeof(tmpTSPat));
				int iret = Adjust_PAT_table(&tmpTSPat,packet+iBeginlen+iPoint_fielLen);  //����һ���ֽ�ָ����
				if(iret < 0)
				{
					// packet not find pid
					return false;
				}

				if(tmpTSPat.program_number == 0x0)
					m_iPMTPID = tmpTSPat.network_PID;
				else
					m_iPMTPID = tmpTSPat.program_map_PID;
				m_iServerPID = tmpTSPat.program_number;
				//��¼
				fprintf(m_Mediafp,"PMT PID=%d \n",m_iPMTPID);
				fprintf(m_Mediafp,"Service PID=%d \n",m_iServerPID);
				fflush(m_Mediafp);

			}
			else if(pid == m_iPMTPID && (m_iPCRPID == 0 || m_iVideoPID ==0 || m_iAudioPID==0))
			{
				//����pid
				TS_PMT tmpPMT;
				memset(&tmpPMT,0,sizeof(tmpPMT));
				Adjust_PMT_table(&tmpPMT,packet+iBeginlen+iPoint_fielLen);  //4
				
				m_iPCRPID = tmpPMT.PCR_PID;

				fprintf(m_Mediafp,"PCR PID=%d \n",m_iPCRPID);
				fprintf(m_Mediafp,"Video PID=%d \n",m_iVideoPID);
				fprintf(m_Mediafp,"Auido PID=%d \n",m_iAudioPID);
				fflush(m_Mediafp);
	/*			MapPIDStreamType::iterator itfind = m_mapStreamPID.begin();
				while(itfind)
				{
					if()
					++itfind;
				}
	*/
			}

			p = packet + 4;
			if (afc == 0) /* reserved value */
				return 0;

			has_adaptation = afc & 2;
			has_payload = afc & 1;
			is_discontinuity = has_adaptation
				&& packet[4] != 0 /* with length > 0 */
				&& (packet[5] & 0x80); /* and discontinuity indicated */

			cc = (packet[3] & 0xf);

			if (has_adaptation) {
				/* skip adaptation field */
				//p += p[0] + 1;  p[0]Ϊ�����ֶγ���
				int iPCRflag = p[1]&0x10;
				if(iPCRflag ==0x10)
				{
					//pcrflag 
					uint64_t pcr_high =(uint64_t)(p[2] << 25);
					pcr_high = pcr_high | (p[3] << 17);
					pcr_high = pcr_high | (p[4] << 9);
					pcr_high = pcr_high | (p[5] << 1);
					pcr_high = pcr_high | (p[6]&0x80)>>7;

					uint64_t pcr_low = (uint64_t)((p[6]&0x01)<<8);
					pcr_low == pcr_low | p[7];

					m_llPCR = pcr_high*300 + pcr_low;

					m_llPCR = m_llPCR /300; //����90khz
					//33bit base

				//	fprintf(m_Mediafp,"Pcr=%d \n",m_llPCR);
				//	fflush(m_Mediafp);

				}

				p += p[0] + 1; // p[0]Ϊ�����ֶγ���
				/* if past the end of packet, ignore */
				p_end = packet + TS_PACKET_SIZE;
				if (p >= p_end)
					return 0;
			}

			if(packet[1] & 0x40)  //is_start = packet[1] & 0x40;
			{	
				//���� pes 
				iseekLen = p - packet;
				hasFindIframe = Adjust_PES_Pakcet(p,iseekLen);
				
			}		
			
			if(!hasFindIframe)
				index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}	
	return 0;
}



bool NewQueue::Find_Stream_IFrame(unsigned char *buff,int ilen)
{
	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	unsigned char *packet;
	const uint8_t *pESH264_IDR= NULL;

	int iseekLen = 0; //ts head len
	int iPesHeadLen = 0; //pes head len
	int iEsSeekLen = 0;
	int ifindLen =0;

	bool hasFindIframe = false;
	while(index <= ilen-TS_PACKET_SIZE && !hasFindIframe)
	{
		if(buff[index]==0x47)
		{
			//LOGD("-----find a ts packet \n");
			packet = &buff[index];

			TS_packet_Header tmpPacketHeader;
			memset(&tmpPacketHeader,0,sizeof(tmpPacketHeader));
			Adjust_TS_packet_header(&tmpPacketHeader,packet);

			int pid = tmpPacketHeader.PID ;
			int iPoint_fielLen = tmpPacketHeader.payload_unit_start_indicator;

			if(pid == 0x1fff)
			{
				//NULL packet
				//���������
				index += TS_PACKET_SIZE;
				continue;
			}
			if (tmpPacketHeader.payload_unit_start_indicator != 0x01) // ��ʾ�� ����PSI����PESͷ
			{
				LOGD("---no palyload data \n");
			//	fflush(stdout);
				//���������
				index += TS_PACKET_SIZE;
				continue;
			}

			int len, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			afc = tmpPacketHeader.adaption_field_control;
			uint8_t *p, *p_end,*pEsData;


			p = packet + 4;
			has_adaptation = afc & 2;
			has_payload = afc & 1;
			is_discontinuity = has_adaptation
				&& packet[4] != 0 /* with length > 0 */
				&& (packet[5] & 0x80); /* and discontinuity indicated */

			cc = (packet[3] & 0xf);

			if (has_adaptation) {
				/* skip adaptation field */
				//p += p[0] + 1;  p[0]Ϊ�����ֶγ���
				int iPCRflag = p[1]&0x10;

				p += p[0] + 1; // p[0]Ϊ�����ֶγ���
				/* if past the end of packet, ignore */
				p_end = packet + TS_PACKET_SIZE;
				if (p >= p_end)
				{
					//���������
					index += TS_PACKET_SIZE;
					continue;
				}
			}

			if(packet[1] & 0x40)  //is_start = packet[1] & 0x40; payload_unit_start_indicator
			{	
				//���� pes 
				iseekLen = p - packet;
				hasFindIframe = Adjust_PES_Pakcet(p,iseekLen);				
			}		

			if(!hasFindIframe)
				index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}	



	return hasFindIframe;
}

//������������
bool NewQueue::Set_tsRate_period(int iperiod)
{
	m_iperiod = iperiod;
	return true;
}

//��ȡ������
bool NewQueue::Get_tsRate(int* iRate)
{
	*iRate = m_iRate;
	return true;
}
bool NewQueue::Get_tsIFrame_size(int* iSize)
{
	//m_tsStreamPrase.Get_tsIFrame_size(iSize);
	//m_tsStreamparse.Get_tsIFrame_size(iSize);
	return true;
}

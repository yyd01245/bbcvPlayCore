#include "TSStreamInfo.h"


#define __ES_PARSER_

const int TS_PACKET_SIZE = 188;

TSstreamInfo::TSstreamInfo()
{
	m_iAudioCodeType = STREAMTYPE_UNKNOWN;
	m_iVideoCodeType = STREAMTYPE_UNKNOWN;

	m_bPesBeginFlag = false;
	m_iPMTPID = 0;
	m_iPCRPID = 0;
	m_iVideoPID = 0;
	m_iAudioPID = 0;
	m_FramDatafp = NULL;
	m_itsStreamPacketNumber = 0;
	m_iframesize = 0;
	m_iIframeSize = 0;
	m_bIFramebeginFlag = false;
		//��ȡ��ǰʱ����Ϊ��׺
	char tsFilePath[1024] = {0};
 	time_t now;
    struct tm tmnow;
    time(&now);
    localtime_r(&now,&tmnow);	
	sprintf(tsFilePath,"frameinfo_%d_%d_%d.log",tmnow.tm_hour,tmnow.tm_min,tmnow.tm_sec);
	printf("%s \n",tsFilePath);
#ifdef ENABLEFILE
	m_Mediafp = fopen("videoinfo.log","w");;
	m_FramDatafp = fopen(tsFilePath,"wb");
#else
	m_Mediafp = NULL;
	m_FramDatafp = NULL;
#endif
	
	m_llFrameNum = 0;
	m_iFramTotal =	0;
	m_llFrameTotal = 0;
	m_iGopSize = 0;

	pthread_mutex_init(&m_mutexlocker,NULL);
}

TSstreamInfo::~TSstreamInfo()
{
	pthread_mutex_destroy(&m_mutexlocker);
}


void  TSstreamInfo::Adjust_PMT_table(TS_PMT* packet ,unsigned char *buffer)
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

	printf("---pmt pid =%d ,program info leng=%d \n",packet->PCR_PID,packet->program_info_length);

	for(;pos<=(packet->section_length+2)-4;)
	{
		packet->stream_type = buffer[pos];
		packet->reserved_5 = buffer[pos+1]>>5;
		packet->elementary_PID = ((buffer[pos+1] << 8) | buffer[pos+2])&0x1fff;
		packet->reserved_6 = buffer[pos+3] >> 4;
		packet->ES_info_length = (buffer[pos+3]&0x0F) << 8 | buffer[pos+4];
		
		m_mapStreamPID.insert(MapPIDStreamType::value_type(packet->stream_type,packet->elementary_PID));
	
		printf("stream_type=%d,ele_pid=%d,es_length=%d \n",packet->stream_type,
			packet->elementary_PID ,packet->ES_info_length );
/*
		if(packet->stream_type == 0x1B)
		{
			// video h264
			m_iVideoPID = packet->elementary_PID;
		}
		else if(packet->stream_type == 0x03)
		{
			m_iAudioPID = packet->elementary_PID;
		}
*/
		switch(packet->stream_type)
		{
		  case STREAMTYPE_11172_AUDIO:
	      case STREAMTYPE_13818_AUDIO:
	      case STREAMTYPE_AC3_AUDIO:
	      case STREAMTYPE_AAC_AUDIO:
	      case STREAMTYPE_MPEG4_AUDIO:
  			{
				m_iAudioCodeType = packet->stream_type;
				m_iAudioPID = packet->elementary_PID;
				printf("---get audio pid %d codetype %d \n",m_iAudioPID,m_iAudioCodeType);
	            break;
	      	}
	      case STREAMTYPE_13818_VIDEO:	  	
	      case STREAMTYPE_11172_VIDEO:
	      case STREAMTYPE_H264_VIDEO:
	      case STREAMTYPE_AVS_VIDEO:
		  	{
				m_iVideoCodeType = packet->stream_type;
				m_iVideoPID = packet->elementary_PID;
				printf("---get video pid %d codetype %d \n",m_iVideoPID,m_iVideoCodeType);
	            break;
	      	}	
	      case STREAMTYPE_13818_PES_PRIVATE:
	             break;
	      case STREAMTYPE_13818_B:
	             break;
	      default:
	             break;
		
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


int  TSstreamInfo::Adjust_PAT_table(TS_PAT* packet ,unsigned char *buffer)
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

	for(n=0;n<packet->section_length-12;n+=4)
	{
		packet->program_number = buffer[8]<<8 |buffer[9];
		
		packet->reserved_3 = buffer[10]>>5;
		if(packet->program_number == 0x0)
		{
			packet->network_PID = (buffer[10]<<3)<<5|buffer[11];
			//packet->network_PID = (buffer[11]<<3)<<5|buffer[12];
		}
		else{
		     TS_PAT_Program PAT_program;  
	           PAT_program.program_map_PID = (buffer[10 + n] & 0x1F) << 8 | buffer[11 + n];  
	           PAT_program.program_number = packet->program_number;  
	           packet->program.push_back( PAT_program );  
			printf("pat program pid =%d  number=%d \n",packet->program_map_PID,packet->program_number);
	        //   packet->push_back( PAT_program );//��ȫ��PAT��Ŀ���������PAT��Ŀ��Ϣ 

		}
			//packet->program_map_PID = (buffer[10]<<3)<<5 |buffer[11];

	}

	return 0;
}


void TSstreamInfo::Adjust_TS_packet_header(TS_packet_Header* pHeader,unsigned char *buffer)
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

	//printf("---pid =%d ---\n",pHeader->PID);

}


int64_t TSstreamInfo::Get_PCR_Value(unsigned char* buffer,int itemplen)
{
	int64_t ret = 0;

	return ret;
}
int TSstreamInfo::Set_PCR_Value(unsigned char* buffer,int index,int64_t pcr_value)
{
	printf("---set pcr value = %lld \n",pcr_value);

	unsigned char* bufftmp = buffer+index;
	int64_t pcr_low = pcr_value % 300, pcr_high = pcr_value / 300;

    *bufftmp++ = pcr_high >> 25;
    *bufftmp++ = pcr_high >> 17;
    *bufftmp++ = pcr_high >>  9;
    *bufftmp++ = pcr_high >>  1;
    *bufftmp++ = pcr_high <<  7 | pcr_low >> 8 | 0x7e;
    *bufftmp++ = pcr_low;
	
	return 0;
}
int TSstreamInfo::Find_PCR_Index(unsigned char* buff,int ilen,int *iIndex)
{
	//ʶ���Ƿ�tsͷ
	int itsHead;
	int index = 0;
	unsigned char *packet;

	int iseekLen = 0; //ts head len
	int iRetPCRindex = -1;

	bool hasFindpcr = false;
	while(index <= ilen-TS_PACKET_SIZE && !hasFindpcr)
	{
		if(buff[index]==0x47 && 
			((index< ilen - TS_PACKET_SIZE) ? (buff[index+TS_PACKET_SIZE]==0x47):1))
		{			
			packet = &buff[index];
			TS_packet_Header tmpPacketHeader;
			Adjust_TS_packet_header(&tmpPacketHeader,packet);

			int pid = tmpPacketHeader.PID ;
			int iPoint_fielLen = tmpPacketHeader.payload_unit_start_indicator;

			if(pid == 0x1fff)
			{
				//NULL packet
				//���������
				//printf("---no palyload data \n");
				index += TS_PACKET_SIZE;
				continue;
			}
			if (tmpPacketHeader.payload_unit_start_indicator != 0x01) // ��ʾ�� ����PSI����PESͷ
			{
				//printf("---no start data \n");
				//���������
				index += TS_PACKET_SIZE;
				continue;
			}
			int len, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			afc = tmpPacketHeader.adaption_field_control;
			uint8_t *p, *p_end,*pEsData;

			p = packet + 4;
			if (afc == 0) /* reserved value */
				return 0;
			has_adaptation = afc & 2;
			has_payload = afc & 1;
	//		is_discontinuity = has_adaptation
	//			&& packet[4] != 0 /* with length > 0 */
	//			&& (packet[5] & 0x80); /* and discontinuity indicated */

			cc = (packet[3] & 0xf);

			if (has_adaptation) 
			{
				/* skip adaptation field */
				//p += p[0] + 1;  p[0]Ϊ�����ֶγ���
				int iPCRflag = p[1]&0x10;
				if(iPCRflag ==0x10)
				{
					iRetPCRindex = index+4+2;
					//pcrflag 
					uint64_t pcr_high =(uint64_t)(p[2] << 25);
					pcr_high = pcr_high | (p[3] << 17);
					pcr_high = pcr_high | (p[4] << 9);
					pcr_high = pcr_high | (p[5] << 1);
					pcr_high = pcr_high | (p[6]&0x80)>>7;

					uint64_t pcr_low = (uint64_t)((p[6]&0x01)<<8);
					pcr_low == pcr_low | p[7];

					m_llPCR = pcr_high*300 + pcr_low;
					*iIndex = iRetPCRindex;
					printf("---pcr value = %lld \n",m_llPCR);
					printf("----pcr1%x %x %x %x \n",buff[iRetPCRindex],buff[iRetPCRindex+1],
							buff[iRetPCRindex+2],buff[iRetPCRindex+3]);

					//�ҵ�pcr
					return 0;

					//m_llPCR = m_llPCR /300; //����90khz
				}

		//		p += p[0] + 1; // p[0]Ϊ�����ֶγ���
				/* if past the end of packet, ignore */
		//		p_end = packet + TS_PACKET_SIZE;
		//		if (p >= p_end)
		//			return 0;
			}
			
			if(!hasFindpcr)
				index += TS_PACKET_SIZE;
		}
		else
			++index;
	}
	return -1;
}


uint64_t TSstreamInfo::Parse_PTS(unsigned char *pBuf)
{
	unsigned long long llpts = (((unsigned long long)(pBuf[0] & 0x0E)) << 29)
		| (unsigned long long)(pBuf[1] << 22)
		| (((unsigned long long)(pBuf[2] & 0xFE)) << 14)
		| (unsigned long long)(pBuf[3] << 7)
		| (unsigned long long)(pBuf[4] >> 1);
	return llpts;
}


bool TSstreamInfo::GetVideoESInfo(unsigned char *pEsData,int itempLen)
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
			printf("------------4 0 flag bits =%0x\n",*(pData+4));
			int iflag = (*(pData+4))&0x1f;

			if(iflag == 7)
			{
				printf("----------find sps \n");
				pESH264_IDR = pData - 5;
				hasFindSPS = true;
				//break;
			}
			else if(iflag == 8)
			{
				printf("-------find pps \n");
				pESH264_IDR = pData - 5;
				hasFindPPS = true;
				//break;

			}
			else if(iflag == 5)
			{
				printf("-------find IDR \n");
				pESH264_IDR = pData - 5;
				hasFindIframe = true;
			//	m_iGopSize++;
				//break;
			}
			else if(iflag == 1)
			{
				printf("-------find P frame or B frame \n");
				pESH264_IDR = pData - 5;
				hasFindPFrame = true;
				//break;
				m_iGopSize++;
				break;
			}
	
			if(hasFindIframe && hasFindSPS && hasFindIframe)
			{
				printf("----find all info \n");
				//��gopsize ���
				if(m_iGopSize != 0)
				{
					//��¼gopsize
					fprintf(m_Mediafp,"Gop size=%d frametotal=%d\n",m_iGopSize,m_iFramTotal);
					fflush(m_Mediafp);
				}
				m_iGopSize = 0;
				m_iFramTotal = 0;
				break;
			}
			ifindLen += 5;

		}
		else if(*pData==0x00 && *(pData+1)==0x00&& *(pData+2)==0x01 )
		{
			printf("------------3 0 flag bits =%0x\n",*(pData+3));
			int iflag = (*(pData+3))&0x1f;
			//printf("---------h264 nal type =%d \n",iflagr);

			if(iflag == 7)
			{
				printf("----------find sps \n");
				pESH264_IDR = pData - 4;
				hasFindSPS = true;
				//break;
			}
			else if(iflag == 8)
			{
				printf("-------find pps \n");
				pESH264_IDR = pData - 4;
				hasFindPPS = true;
				//break;

			}
			else if(iflag == 5)
			{
				printf("-------find IDR \n");
				pESH264_IDR = pData - 4;
				//m_iGopSize++;
				hasFindIframe = true;
				//break;
			}
			else if(iflag == 1)
			{
				printf("-------find P frame or B frame \n");
				pESH264_IDR = pData - 4;
				hasFindPFrame = true;
				//break;
				m_iGopSize++;
				break;
			}

			if( hasFindPPS && hasFindSPS && hasFindIframe )
			{
				printf("----find all info \n");
				m_iGopSize++;
				//��gopsize ���
				if(m_iGopSize != 0)
				{
					//��¼gopsize
					fprintf(m_Mediafp,"Gop size=%d frametotal=%d\n",m_iGopSize,m_iFramTotal);
					fflush(m_Mediafp);
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


bool TSstreamInfo::Find_Stream_IFrame(unsigned char *buff,int ilen,int* pIndex)
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
			hasFindIframe = false;
			//printf("-----find a ts packet \n");
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
				//printf("---no palyload data \n");
				index += TS_PACKET_SIZE;
				continue;
			}
			if (tmpPacketHeader.payload_unit_start_indicator != 0x01) // ��ʾ�� ����PSI����PESͷ
			{
				//printf("---no palyload data \n");
				fflush(m_Mediafp);
				//���������
				index += TS_PACKET_SIZE;
				continue;
			}
	
			int len, cc,  afc, is_start, is_discontinuity,
				has_adaptation, has_payload;
			afc = tmpPacketHeader.adaption_field_control;
			uint8_t *p, *p_end,*pEsData;


			if(m_iPMTPID == 0 && pid ==0 )
			{
				// PAT �ҵ�PMT��Ϣ
				//8+ 1+1+2+12+16+2+5+1+8+8
				//section_length 
				//����е����ֶ� ��Ҫ��������

				int ihas_adaptation = afc & 2;
				int ihas_payload = afc & 1;
							
				TS_PAT tmpTSPat;
				memset(&tmpTSPat,0,sizeof(tmpTSPat));
				int iret = Adjust_PAT_table(&tmpTSPat,packet+4+iPoint_fielLen);  //����һ���ֽ�ָ����
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
				Adjust_PMT_table(&tmpPMT,packet+4+iPoint_fielLen);
				
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
				if(hasFindIframe)
					*pIndex = index;
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

bool TSstreamInfo::Find_IFrame(unsigned char *p,int iseekLen)
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
		//printf("-----------pes len =%d\n",ilen);
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
			fprintf(m_Mediafp,"PCR=%d \n",m_llPCR);
			fprintf(m_Mediafp,"PTS=%d \n",m_llPts);
			fprintf(m_Mediafp,"PTS-PCR=%d \n",m_llPts-m_llPCR);
		//	if(p[3]==0xE0)
			{
				double fps = 1000/((m_llPts - m_llLastPts)/90);
				if(m_iFrameRate != fps && m_llLastPts != 0 && fps <= 30 && fps >=15)
				{
					fprintf(m_Mediafp,"rate=%3.1f \n",fps);
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
		//printf("--------------pesheadlen=%d \n",iPesHeadLen);
		pEsData = p+iPesHeadLen;
		//
		//find 00000001 7 8 5
		int itempLen = TS_PACKET_SIZE - iseekLen - iPesHeadLen;

		//return GetVideoESInfo(pEsData,itempLen);
		return ParseH264ES(pEsData,itempLen);
	}

	return false;



}


bool TSstreamInfo::Adjust_PES_Pakcet(unsigned char *p,int iseekLen)
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
		printf("-----------pes len =%d\n",ilen);
		
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
			//m_iFramTotal++;
			fprintf(m_Mediafp,"PCR=%d \n",m_llPCR);
			fprintf(m_Mediafp,"PTS=%d \n",m_llPts);
			fprintf(m_Mediafp,"PTS-PCR=%d \n",m_llPts-m_llPCR);
			if(m_llPts - m_llLastPts > 0)
			{
				
				double fps = 1000/((m_llPts - m_llLastPts)/90);
				if(m_iFrameRate != fps && m_llLastPts != 0 && fps <= 30 && fps >=15)
				{
					fprintf(m_Mediafp,"rate=%3.1f \n",fps);
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
		//printf("--------------pesheadlen=%d \n",iPesHeadLen);
		pEsData = p+iPesHeadLen;
		//
		//find 00000001 7 8 5
		int itempLen = TS_PACKET_SIZE - iseekLen - iPesHeadLen;

		// ����һ: ��pes���������ƣ���׼ȷ
		//ȷ�����յ���ƵPES ��ͷ���ݼ�Ϊһ֡��ʼ��
		//ȷ��PES���ȣ����м��㵱ǰ���յ����ݳ���
		//�ﵽ�˳��ȣ�����Ϊһ֡�����ѽ�����ȫ��

		//������: 
		//ȷ���յ�����һ֡��ʼ������һ����ʶ
		//��������������������ݵĿ�ʼ����Ϊ��PES���ݽ��ս���
		pthread_mutex_lock(&m_mutexlocker);
		m_bPesBeginFlag = true;
		
		printf("-------------num = %ld \n",m_llFrameNum);
		m_llFrameNum++;
		pthread_mutex_unlock(&m_mutexlocker);
	
		//return GetVideoESInfo(pEsData,itempLen);
#ifdef __ES_PARSER_		
		return ParseH264ES(pEsData,itempLen);
#else
		return true;
#endif
	}

	return false;
}


//FILE* fptest = NULL;
bool TSstreamInfo::ParseH264ES(unsigned char* buffer,int itemplen)
{
	int buf_index = 0;
	int next_avc = itemplen;
	unsigned char * pData = NULL;
//	if(fptest == NULL)
//		fptest = fopen("test.264","wb+");
	bool bSPSFlag = false;
	bool bPPSFlag = false;

	bool bSeqHead = false;
	bool bSeqExten = false;
	bool bGropFlag = false;
	
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

		if(m_iVideoCodeType == STREAMTYPE_13818_VIDEO )
		{
			printf("---mpeg2 stream \n");
			if(*(pData) == 0xb8)
			{
				// b5��P֡
				//mpeg2
							// I frame
				m_iGopSize++;
				//��gopsize ���
				//if(m_iGopSize != 0)
				//{
					//��¼gopsize
					fprintf(m_Mediafp," MPEG2 Gop index=%d gopsize=%d\n",m_iGopSize,m_llFrameNum-m_iFramTotal);
					fflush(m_Mediafp);
			
				//	fwrite(pData,1,itemplen-5-buf_index,fptest);
				//}
				//m_iGopSize = 0;
				m_iFramTotal = m_llFrameNum;
				break;
			}
			else if((*(pData) == 0x00))
			{
				m_iVideoCodeType = STREAMTYPE_13818_VIDEO;
				//ͼ����ʼ�� 
				// ����temporal_reference 10λ 
				//Picture_coding_type��3λ��,���Picture_coding_typeΪ
				//001�������ƣ�����I֡��Ϊ010��P֡,011��� ��B֡,
				unsigned char * pstartcode = pData;
				pstartcode += 1;
				//����ǰ2λ��ȡ���м���λ
				int ipic_code_type = ((*pstartcode) << 2 >>2) & 0x38;
				if(ipic_code_type == 2 || ipic_code_type == 3)
				{
					//m_iGopSize++;
					fprintf(m_Mediafp," MPEG2 PB Frame=%d \n",ipic_code_type);
					fflush(m_Mediafp);
				}
			}

		}
		else if(m_iVideoCodeType == STREAMTYPE_H264_VIDEO )
		{
			//printf("---h264 stream \n");
			if(iflag == 5)
			{
				// I frame
				
				//��gopsize ���
				//if(m_iGopSize != 0)
				{
					//��¼gopsize
					printf("---last framnum =%d \n",m_iFramTotal);
					fprintf(m_Mediafp,"  Gop index=%d gopsize=%d\n",m_iGopSize,m_llFrameNum-m_iFramTotal);
					fflush(m_Mediafp);

				//	fwrite(pData,1,itemplen-5-buf_index,fptest);
				}
				//m_iGopSize = 0;
				m_iGopSize++;
				m_iFramTotal = m_llFrameNum;

				m_bIFramebeginFlag = true;
				break;
			}
			else if(iflag==1)
			{
				//m_iGopSize++;
				break;
			}
		}
		else
		{
			//PMT��û�����������߽�������ȷ
			if(*(pData) == 0xb3)
				bSeqHead = true;
			if(*(pData) == 0xb5)
				bSeqExten = true;
			if(bSeqExten && bSeqHead)
				m_iVideoCodeType = STREAMTYPE_13818_VIDEO;
			if(bSeqExten && bSeqHead && (*(pData) == 0xb8))
			{
				// b5��P֡
				//mpeg2
							// I frame
				m_iVideoCodeType = STREAMTYPE_13818_VIDEO;
				m_iGopSize++;
				//��gopsize ���
				//if(m_iGopSize != 0)
				//{
					//��¼gopsize
					fprintf(m_Mediafp," MPEG2 Gop index=%d gopsize=%d\n",m_iGopSize,m_llFrameNum-m_iFramTotal);
					fflush(m_Mediafp);
			
				//	fwrite(pData,1,itemplen-5-buf_index,fptest);
				//}
				//m_iGopSize = 0;
				m_iFramTotal = m_llFrameNum;
				break;
			}
			else if(bSeqExten && bSeqHead && (*(pData) == 0x00))
			{
				m_iVideoCodeType = STREAMTYPE_13818_VIDEO;
				//ͼ����ʼ�� 
				// ����temporal_reference 10λ 
				//Picture_coding_type��3λ��,���Picture_coding_typeΪ
				//001�������ƣ�����I֡��Ϊ010��P֡,011��� ��B֡,
				unsigned char * pstartcode = pData;
				pstartcode += 1;
				//����ǰ2λ��ȡ���м���λ
				int ipic_code_type = ((*pstartcode) << 2 >>2) & 0x38;
				if(ipic_code_type == 2 || ipic_code_type == 3)
				{
					//m_iGopSize++;
					fprintf(m_Mediafp," MPEG2 PB Frame=%d \n",ipic_code_type);
					fflush(m_Mediafp);
				}
			}
			if(m_iVideoCodeType != STREAMTYPE_13818_VIDEO)
			{
				printf("---h264 stream \n");
				// h264
				if(iflag == 7)
					bSPSFlag = true;
				if(iflag == 8)
					bPPSFlag = true;
				if(bSeqHead && bPPSFlag)
				{
					if(iflag == 5)
					{
						// I frame
						m_iGopSize++;
						{
							//��¼gopsize
							fprintf(m_Mediafp,"  Gop index=%d gopsize=%d\n",m_iGopSize,m_llFrameNum-m_iFramTotal);
							fflush(m_Mediafp);
						
						//	fwrite(pData,1,itemplen-5-buf_index,fptest);
						}
						//m_iGopSize = 0;
						m_iFramTotal = m_llFrameNum;
						m_bIFramebeginFlag = true; //I ֡��ʶ
						break;
					}
					else if(iflag==1)
					{
						//m_iGopSize++;
						break;
					}
				}
			}
		}
			

	}


	// 
	return true;
}



int TSstreamInfo::ParseStreamInfo(uint8_t *buff,int ilen)
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
			//printf("-----find a ts packet \n");
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
			if (tmpPacketHeader.payload_unit_start_indicator != 0x01) // ��ʾ�� ����PSI����PESͷ
			{
				printf("---no palyload dat \n");
				fflush(stdout);
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
			 case 0x0:									  // reserved for future use by ISO/IEC
				 return false;
			 case 0x1:									  // �޵����ֶΣ�������Ч����  
				 //iBeginlen += packet[iBeginlen] + 1;	// + pointer_field
				 iBeginlen +=  0;  // + pointer_field
				 break;
			 case 0x2:									   // ���������ֶΣ�����Ч����
			 //  iBeginlen += packet[iBeginlen] + 1;  // + pointer_field
				 iBeginlen += 0;  // + pointer_field
				 break;
			 case 0x3:									  // �����ֶκ���Ч����
				 if (adaptation_field_length > 0) 
				 {
					 iBeginlen += 0;				   // adaptation_field_lengthռ8λ
					 iBeginlen += adaptation_field_length; // + adaptation_field_length
				 }
				 else
				 {
					 iBeginlen += 0;
				 }
				 //iBeginlen += packet[iBeginlen] + 1;			 // + pointer_field
				 break;
			 default:	 
				 break;
			 
			 }


			if(m_iPMTPID == 0 && pid ==0 )
			{
				// PAT �ҵ�PMT��Ϣ
				//8+ 1+1+2+12+16+2+5+1+8+8
				//section_length 
				//����е����ֶ� ��Ҫ��������

				int ihas_adaptation = afc & 2;
				int ihas_payload = afc & 1;
							
				TS_PAT tmpTSPat;
				memset(&tmpTSPat,0,sizeof(tmpTSPat));
				int iret = Adjust_PAT_table(&tmpTSPat,packet+4+iPoint_fielLen);  //����һ���ֽ�ָ����
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
				Adjust_PMT_table(&tmpPMT,packet+4+iPoint_fielLen);
				
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

bool ISinPAT(TS_PAT *pPat,unsigned long upid)
{
	std::vector<TS_PAT_Program>::iterator it = pPat->program.begin();
	while(it != pPat->program.end())
	{
		TS_PAT_Program tmp = *it;
		if(upid == tmp.program_map_PID)
		{
			return true;
		}
		++it;
	}
	return false;
}

//����һ��TS��188 bytes���ҵ��Ƿ�����Ƶ����Ƶ�Ľ�����ʶ��
int TSstreamInfo::ParseStreamFrame(uint8_t *buff,int ilen)
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

	bool hasFindFrameFlag = false;
	bool hasFindIframe = false;
	bool hasFindSPS = false;
	bool hasFindPPS = false;
	//printf("--parse len=%d \n",ilen);
	while(index < ilen && !hasFindFrameFlag)
	{
		if(buff[index]==0x47)
		{
			hasFindIframe = false;
			hasFindSPS = false;
			hasFindPPS = false;
			//printf("-----find a ts packet \n");
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
			/*
				static FILE* fp_es = fopen("out.ts","wb");
				fwrite(packet,1,ilen,fp_es);
				fflush(fp_es);
			*/
			if((m_iVideoPID == pid) && m_bPesBeginFlag)
			{
				++m_itsStreamPacketNumber;
				//printf("---packet video %d\n",m_itsStreamPacketNumber);
			}
			if (tmpPacketHeader.payload_unit_start_indicator != 0x01) // ��ʾ�� ����PSI����PESͷ
			{
				//printf("---no palyload dat %s \n",__FUNCTION__);
				//fflush(stdout);

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
			 case 0x0:									  // reserved for future use by ISO/IEC
				 return false;
			 case 0x1:									  // �޵����ֶΣ�������Ч����  
				 //iBeginlen += packet[iBeginlen] + 1;	// + pointer_field
				 iBeginlen +=  0;  // + pointer_field
				 break;
			 case 0x2:									   // ���������ֶΣ�����Ч����
			 //  iBeginlen += packet[iBeginlen] + 1;  // + pointer_field
				 iBeginlen += 0;  // + pointer_field
				 break;
			 case 0x3:									  // �����ֶκ���Ч����
				 if (adaptation_field_length > 0) 
				 {
					 iBeginlen += 0;				   // adaptation_field_lengthռ8λ
					 iBeginlen += adaptation_field_length; // + adaptation_field_length
				 }
				 else
				 {
					 iBeginlen += 0;
				 }
				 //iBeginlen += packet[iBeginlen] + 1;			 // + pointer_field
				 break;
			 default:	 
				 break;
			 
			 }

			if(m_iPMTPID == 0 && pid ==0 )
			{
				// PAT �ҵ�PMT��Ϣ
				//8+ 1+1+2+12+16+2+5+1+8+8
				//section_length 
				//����е����ֶ� ��Ҫ��������

				int ihas_adaptation = afc & 2;
				int ihas_payload = afc & 1;
							
				//TS_PAT tmpTSPat;
				memset(&tmpTSPat,0,sizeof(tmpTSPat));
				int iret = Adjust_PAT_table(&tmpTSPat,packet+4+iPoint_fielLen);  //����һ���ֽ�ָ����
				if(iret < 0)
				{
					// packet not find pid
					return false;
				}

				std::vector<TS_PAT_Program>::iterator it = tmpTSPat.program.begin();
				while(it != tmpTSPat.program.end())
				{
					TS_PAT_Program tmp = *it;
					if(tmpTSPat.program_number == 0x0)
						m_iPMTPID = tmpTSPat.network_PID;
					else
						m_iPMTPID = tmpTSPat.program_map_PID;
					
					m_iServerPID = tmpTSPat.program_number;
					//��¼
					fprintf(m_Mediafp,"PMT PID=%d \n",m_iPMTPID);
					fprintf(m_Mediafp,"Service PID=%d \n",m_iServerPID);
					fflush(m_Mediafp);
					++it;
				}



			}
			else if(ISinPAT && (m_iPCRPID == 0 || m_iVideoPID ==0 || m_iAudioPID==0))
			{
				//����pid
				TS_PMT tmpPMT;
				memset(&tmpPMT,0,sizeof(tmpPMT));
				Adjust_PMT_table(&tmpPMT,packet+4+iPoint_fielLen);
				
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
			//write es
			/*
			if(m_iVideoPID == pid)
			{
				iseekLen = p - packet;
				static FILE* fp_es = fopen("out.es","wb");
				fwrite(p,1,iseekLen,fp_es);
				fflush(fp_es);
			}
*/
			if(packet[1] & 0x40 && (m_iVideoPID == pid))  //is_start = packet[1] & 0x40;
			{	
								
				//���� pes 
				iseekLen = p - packet;
				//���pes��ʶΪtrue,˵��һ֡����
				if(m_bPesBeginFlag == true)
				{
					m_iframesize = (184*m_itsStreamPacketNumber-iseekLen); //���������FF�����ݹʻ᲻��׼ȷ��
					m_itsStreamPacketNumber = 0;
					RecordVideoInfo();
					if(m_bIFramebeginFlag == true)
					{
						m_iIframeSize = m_iframesize;
						printf("I frame size =%d \n",m_iIframeSize);
						m_bIFramebeginFlag = false;
					}
					else
					{
						printf("P frame size=%d \n",m_iframesize);
					}
				}

				hasFindIframe = Adjust_PES_Pakcet(p,iseekLen);
				
			}

			index += TS_PACKET_SIZE;
//			if(!hasFindIframe)
//				index += TS_PACKET_SIZE;
		}
		else
		{
			index++;
		}

	}	
	return 0;
}


//����һ��TS��188 bytes���ҵ��Ƿ�����Ƶ����Ƶ�Ľ�����ʶ��
int TSstreamInfo::filternullpacket(uint8_t *buff,int ilen)
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

	while(index < ilen)
	{
		if(buff[index]==0x47)
		{
			//printf("-----find a ts packet \n");
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
			else
			{
				printf("get data len =%d \n",ilen-index);
#ifdef ENABLEFILE
				static FILE* fp_es = fopen("out.ts","wb");
				fwrite(packet,1,ilen,fp_es);
				fflush(fp_es);
#endif			
			}
			index+= TS_PACKET_SIZE;

		}
		else
		{
			index++;
			printf("no 47 head \n");
		}

	}	
	return 0;
}

void TSstreamInfo::get_time(char*buff,int siz)
{
    char lbuf[32];
    struct tm tmnow;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    localtime_r(&ts.tv_sec,&tmnow);
    strftime(lbuf,31,"%Y/%m/%d-%H:%M:%S",&tmnow);
    snprintf(buff,siz,"%s.%03ld",lbuf,(ts.tv_nsec+500000)/1000000);

}



int TSstreamInfo::RecordVideoInfo()
{
	//��¼��ǰ����ʱ��
	
	//����ʶ�ָ�Ϊfalse
	pthread_mutex_lock(&m_mutexlocker);
	m_bPesBeginFlag = false;

	char txtinfo[48] ={0};
	get_time(txtinfo,48);

	//printf("%s \n",txtinfo);
	
	char strFramInfo[128] = {0};
#if 1
	sprintf(strFramInfo,"%s \t index:%d \n",txtinfo,m_llFrameTotal);
	printf("%s",strFramInfo);
	fwrite(strFramInfo,1,strlen(strFramInfo),m_FramDatafp);
	fflush(m_FramDatafp);
	m_llFrameTotal = m_llFrameNum;
	//m_llFrameTotal++;
#endif

/*	
	sprintf(strFramInfo,"%s \t index:%d \n",txtinfo,m_llFrameNum);
	printf("%s",strFramInfo);
	fwrite(strFramInfo,1,strlen(strFramInfo),m_FramDatafp);
	fflush(m_FramDatafp);
*/
	pthread_mutex_unlock(&m_mutexlocker);
	return 0;
}



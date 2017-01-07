#include "Decoder.h"

//#define H264_ONLY

//#define ENABLEFILE 0
#define MAX_WIDTH	= 1920;
#define MAX_HEIGHT = 1080;

const int Queue_size = 25;
const int Video_Buff_size = 1920*1080*3/2;
const int Audio_Buff_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
#define BUF_SIZE  188*7*2
const int TSPACKETLEN = 188;
typedef unsigned char uint8_t;
const int Recv_Queue_size = 1024*1024*4;   //接收缓冲区大小

const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };

const int Use_ffmpeg_recv = 0; // 1为使用ffmpeg接收，0为自己接收数据。	
static int64_t GetTickCount()
{
	struct timeval tv1;
	long long start_time;
	
	gettimeofday(&tv1,NULL);
	start_time = tv1.tv_sec*1000+tv1.tv_usec/1000; //ms
	return start_time;
}


void LogToService(char *id, const char *format, ...)
{
	{
		char log[2000];
		memset(log,'\0', sizeof(log));
		if(format != NULL)
		{
			va_list mylist;
			va_start(mylist, format);    
			vsnprintf( log, sizeof(log)-1, format, mylist);
			va_end(mylist);
		}
		log[sizeof(log)-1] = '\0';
	//	fLOGD(fpLog,log);
	}
}

TSDecoder_Instance::TSDecoder_Instance()
{
	m_pVideoCodec = NULL;
	m_pAudioCodec = NULL;
	m_pFmt = NULL;
	m_pVideoCodecCtx = NULL;
	m_pAudioCodecCtx = NULL;
	m_pframe = NULL;
	m_iHeight = 0;
	m_iWidht = 0;
	m_videoindex = -1;
	m_audioindex = -1;
	fpLog = NULL;
	m_avbuf = NULL;
	m_MediaInfofp = NULL;

	m_swr_ctx = NULL;
	m_pRender = NULL;
	m_iThreadid = 0;
	m_bstop = false;
	m_ifileType = localfile;
	m_bDecoderFlag = false;
	m_bFirstDecodeSuccess = false;
	m_bNeedControlPlay = false;
	m_bSaveTSStream = false;
	m_tsStreamfp = NULL;

	char Path[512] = {0};
	LOGD("yyd info create decoder \n");
	sprintf(Path,"./BBCV-Play.log");
#ifdef ENABLEFILE
	if( NULL == fpLog)
		fpLog = fopen(Path,"w+");
#endif	
	char mediaPath[512]={0};

	sprintf(mediaPath,"./testmedia.log");
	strcpy(m_meidaInfoPath,mediaPath);


	if(NULL==m_MediaInfofp)
	{
	//	m_MediaInfofp = fopen(m_meidaInfoPath,"a+");
	}

	m_lDisplayPort = 0;
//	AMP_Display_GetHandle(m_lDisplayPort);
}

TSDecoder_Instance::~TSDecoder_Instance()
{
	LOGE("yyd info destory Decoder \n");
	int iloop = 5;
	while(iloop-- && m_iThreadid !=0)
	{
		stopDecoder(true);
		usleep(200*1000);
	}
	fclose(fpLog);
	fpLog = NULL;
	uninit_TS_Decoder();
}

FILE *fpyuv=NULL;
FILE *fpes = NULL;
void Write_esdata(AVPacket *pkt)
{
#ifdef ENABLEFILE
	if(NULL == fpes)
		fpes = fopen("es.h264","wb");
	fwrite(pkt->data,1,pkt->size,fpes);
	fflush(fpes);
#endif
}
int TSDecoder_Instance::decode_TimeCalc(unsigned long long &videopts,unsigned long long dts,double stream_timebase,unsigned long long & start_time,int netChatter)
{

#if 1		
	LOGD("vidopts=%lld,dts=%lld,stream_base=%f,starttime=%lld,netcahtter=%d\n",videopts,dts,stream_timebase,start_time,netChatter);
			struct timeval tv1;
		unsigned long long now_time = 0;
		if(videopts <= 0 || abs((int)(videopts-dts))*stream_timebase > netChatter)
		{
			videopts = dts;
			LOGE("reset video pts=%ld \n",videopts);
		}
			gettimeofday(&tv1,NULL);
			now_time= tv1.tv_sec*1000+tv1.tv_usec/1000;
		int iret=0;
		//printf("--abs =%ld\n",abs(videopts-pkt.dts)*1000*pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den );
		unsigned long dts_audiodev= abs((int)(videopts-dts))*1000*stream_timebase;
		if(dts_audiodev > netChatter){
			videopts = dts;
			LOGD("reset video pts=%ld \n",videopts);
		}
		LOGD("dts_audiodev = %ld start_tim=%lld,nowtime=%lld \n",dts_audiodev,start_time,now_time);

		do
		{

			gettimeofday(&tv1,NULL);
			now_time= tv1.tv_sec*1000+tv1.tv_usec/1000;
			//now_time = GetTickCount();
			iret = (dts - videopts)*1000*stream_timebase - (now_time - start_time ); 
			
			if(iret > 0&& iret <= 80){
				//printf("--------------sleep=%ld\n",iret*1000);
				usleep(iret*1000);
			}		
		}while(iret > 0 && iret <=80);
		LOGD("--pkt.dts=%ld,videopts=%ld,start_time=%ld,nowtime=%ld,iret=%ld\n",dts,videopts,start_time,now_time,iret);
		start_time = now_time;
		videopts = dts;
		

#endif
	return 0;
}
//unsigned int _stdcall TSDecoder_Instance::decoder_threadFun(void *param)
void* TSDecoder_Instance::decoder_threadFun(void *param)	
{

	LOGE("yyd info begin decoder_threadFun \n");
	TSDecoder_Instance* this0 = (TSDecoder_Instance*)param;

	AVCodec *pVideoCodec, *pAudioCodec;
	AVCodecContext *pVideoCodecCtx = NULL;
	AVCodecContext *pAudioCodecCtx = NULL;
	AVFormatContext *pFmt = NULL;

	this0->m_iThreadid = pthread_self();
	int ret = -1;

	bool bInitDecodeFlag = false;

	pVideoCodec = this0->m_pVideoCodec;
	pAudioCodec = this0->m_pAudioCodec;
	pFmt = this0->m_pFmt;
	pVideoCodecCtx = this0->m_pVideoCodecCtx;
	pAudioCodecCtx = this0->m_pAudioCodecCtx;

	if(!pVideoCodecCtx || !pAudioCodecCtx ||!pFmt||!pVideoCodec||!pAudioCodec ){
		LOGE("error ffmpeg context null \n");
	}
	int videoindex = this0->m_videoindex;
	int audioindex = this0->m_audioindex;

	int got_picture;
	uint8_t *samples = (uint8_t*)this0->m_audioBuff;
	LOGD("yyd info begin alloc frame \n");
	AVFrame *pframe = av_frame_alloc();  //avcodec_alloc_frame();
	AVPacket pkt;
	LOGD("yyd info begin init packet \n");
	av_init_packet(&pkt);

	this0->m_pframe = pframe;
	struct timeval tv1;
	unsigned long long start_time =0 ,audio_start_time=0,now_time=0;
	const int FrameInterval = 40;
	bool bStart = true;
	start_time = 0;
	audio_start_time = 0;
	gettimeofday(&tv1,NULL);
	start_time = tv1.tv_sec*1000+tv1.tv_usec/1000;

	unsigned long long videopts=0;
	unsigned long long audiopts=0;
	unsigned long long video_PTS = 0;
	bool firstopen = true;
	int iKeyFrameTime = 0;

	int64_t lastPTS =0;

	int64_t lastAuidoPTS = 0;
	int64_t Audio_PTS = 0;

	this0->m_bGetData = false;
	bool bNeedMediaInfo = true;
	int iFramecount =0;
	int iwritetime = 10;
		int iHight =  pVideoCodecCtx->height;
		int iWidth = pVideoCodecCtx->width;
	do
	{
		int iret = av_read_frame(pFmt, &pkt);
		if (iret >= 0) 
		{
			
			if (pFmt->streams[pkt.stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
			{
#if 0		

				if(this0->m_bSmoothPlay&&!this0->m_bGetData) 
				{
					this0->decode_TimeCalc( videopts, pkt.dts, (double)pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den, start_time, 40*2);
		
				}

#endif
				if(pkt.dts == AV_NOPTS_VALUE)
				{
					//printf("pts error =%ld,pts=%ld,add  %d \n\n",pkt.dts,audiopts, pFmt->streams[pkt.stream_index]->time_base.den/1000/pFmt->streams[pkt.stream_index]->time_base.num);
					if(videopts != 0)
						pkt.dts = videopts+40*pFmt->streams[pkt.stream_index]->time_base.den/1000/pFmt->streams[pkt.stream_index]->time_base.num;
				}
				videopts = pkt.dts;

				iFramecount++;
				//Write_esdata(&pkt);
		
				avcodec_decode_video2(pVideoCodecCtx, pframe, &got_picture, &pkt);
				
				if (got_picture) 
				{
					LOGD("****video decode success dts=%lld \n",pkt.dts);
					this0->m_bGetData = true;
#if 0					
					int iwidth = pVideoCodecCtx->width;
					int iHeight = pVideoCodecCtx->height;
					if(!bNeedMediaInfo)
					{
						char txt[512]={0};
						sprintf(txt,"width=%d height=%d iFramecount=%d \n",iwidth,iHeight,iFramecount);
						fwrite(txt,1,strlen(txt),this0->m_MediaInfofp);


						fflush(this0->m_MediaInfofp);
						bNeedMediaInfo = false;
					}

					this0->m_iWidht = iwidth;
					this0->m_iHeight = iHeight;
#endif					
					if(pframe->data[0] != NULL && pframe->data[1] != NULL && pframe->data[2] != NULL)
					{
						if(firstopen)
						{/*
							memset(&systm,0,sizeof(systm));
							GetLocalTime(&systm);
							fLOGD(this0->fpLog,"%2d:%2d:%3d 视频数据解码成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds,pVideoCodecCtx->codec_id,pAudioCodecCtx->codec_id);
							fflush(this0->fpLog);
							*/
							firstopen = false;
						}
					//	this0->showVideoData(pframe,pkt.dts);
						this0->m_avfilter_XH.filter_run(iWidth,iHight,pframe,pkt.dts);
					}
				}
				else
				{
					char txt[512]={0};
					sprintf(txt,"decoder failed iFramecount=%d \n",iFramecount);
					LOGE("yyd error %s ",txt);
				//	fwrite(txt,1,strlen(txt),this0->m_MediaInfofp);
				//	fflush(this0->m_MediaInfofp);
				}
			}
			else if (pFmt->streams[pkt.stream_index]->codec->codec_type == AVMEDIA_TYPE_AUDIO) 
			{
/*
				if(!this0->m_bGetData){
					LOGD("****audio ignore dts=%lld \n",pkt.dts);
					av_free_packet(&pkt);   //视频解码成功之后开始解码音频。
					continue;
				}
*/
					int iget_audio =-1;	
					if(this0->m_bSmoothPlay )
					{
						LOGD("****audio  dts=%lld,audiopts=%lld, num=%d,den=%d \n",pkt.dts,audiopts,pVideoCodecCtx->time_base.num,pVideoCodecCtx->time_base.den);
#if 0					
						if(audiopts== 0 || abs(audiopts-pkt.dts)*1000*pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den > 40*2)
							audiopts = pkt.dts;
						int iret=0;
						LOGD("****audio  dts=%lld,audiopts=%lld, num=%d,den=%d \n",pkt.dts,audiopts,pVideoCodecCtx->time_base.num,pVideoCodecCtx->time_base.den);
				/*
						LOGD("-------------- audio dts=%lld audiopts=%lld,timebase-num=%lld, timebase-den=%lld,codec-delay=%lld,stream-duration=%lld ,stream-den=%lld,stream-num=%lld\n",pkt.dts,audiopts, 
							pVideoCodecCtx->time_base.num,
							pVideoCodecCtx->time_base.den,
							pVideoCodecCtx->delay,
							pFmt->streams[pkt.stream_index]->duration,
							pFmt->streams[pkt.stream_index]->time_base.den,
							pFmt->streams[pkt.stream_index]->time_base.num); */

						long dts_audiodev = (pkt.dts - audiopts)*1000*pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den ;
							gettimeofday(&tv1,NULL);
							now_time= tv1.tv_sec*1000+tv1.tv_usec/1000;
						LOGD("dts_audiodev = %ld start_tim=%lld,nowtime=%lld \n",dts_audiodev,start_time,now_time);

						do
						{
							gettimeofday(&tv1,NULL);
							now_time= tv1.tv_sec*1000+tv1.tv_usec/1000;
							iret = (pkt.dts - audiopts)*1000*pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den - (now_time - start_time ); 
							
							if(iret > 0&& iret <=80){
								//printf("--------------sleep=%d\n",iret*1000);
								usleep(iret*1000);
							}		
						}while(iret > 0 && iret <=80);
						LOGD("--pkt.dts=%lld,audiopts=%lld,start_time=%lld,nowtime=%lld,iret=%lld\n",pkt.dts,audiopts,start_time,now_time,iret);
						start_time = now_time;
						audiopts = pkt.dts;
#endif
						LOGD("timecalc befor stattime=%lld audioptd=%lld,dts=%lld \n",start_time,
				audiopts,pkt.dts);
						LOGD("time_base:num=%d,den=%d, num/den= %lf \n",
						pFmt->streams[pkt.stream_index]->time_base.num,
						pFmt->streams[pkt.stream_index]->time_base.den,
					pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den);
					double time_base = (double)pFmt->streams[pkt.stream_index]->time_base.num/pFmt->streams[pkt.stream_index]->time_base.den;
					LOGD("double timebase=%f \n",time_base);
					
					this0->decode_TimeCalc(audiopts,pkt.dts,time_base,start_time,40*2);
					
						LOGD("timecalc after stattime=%lld audioptd=%lld,dts=%lld \n",start_time,
				audiopts,pkt.dts);
					}
				avcodec_decode_audio4(pAudioCodecCtx,pframe,&iget_audio,&pkt);
				if (iget_audio > 0)
				{
					this0->showAudioData(pframe, pkt.dts);
				}				
			}
			av_free_packet(&pkt);
		}
		else
		{
			av_free_packet(&pkt);
			LOGE("=========read frame failed %d\n",iret);
			break;
		}
	}while(!this0->m_bstop);
	this0->m_bstop = true;
	this0->m_pRender->SendRenderMsg(QUIT_EVENT);
	LOGE("*******decoderThread over!\n");
	this0->m_iThreadid = 0;
}

int TSDecoder_Instance::showVideoData(AVFrame* pframe,unsigned long ulpts)
{

		int iHight =  m_pVideoCodecCtx->height;
		int iWidth = m_pVideoCodecCtx->width;

		int iTemp = m_iCurrentOutBuf;  //靠靠縴uv靠縤ndex
		m_ISInterlacedFrame = pframe->interlaced_frame;

		if(m_ISInterlacedFrame == 1)
		{
		//			avpicture_deinterlace((AVPicture *)pframe,(AVPicture *)pframe,AV_PIX_FMT_YUV420P,iwidth,iHeight);
		}

		pthread_mutex_lock(&m_yuvmutex[iTemp]);
		unsigned char *buff=m_yuvBuff[iTemp];
		memset(buff,0,iWidth*iHight*3/2);
		for (int i = 0; i < iHight; i++)
		{
			memcpy(buff+i*iWidth, pframe->data[0]+i*pframe->linesize[0], iWidth);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth+i*iWidth/2, pframe->data[1]+i*pframe->linesize[1], iWidth/2);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth*5/4+i*iWidth/2, pframe->data[2]+i*pframe->linesize[2], iWidth/2);
		}
		pthread_mutex_unlock(&m_yuvmutex[iTemp]);
		LOGD("decode copy to buffer index =%d \n",iTemp);
		int irendindex = (iTemp+2) % 3 ;
		m_iCurrentOutBuf = (m_iCurrentOutBuf + 1) % 3 ;		
		//this0->m_iCurrentOutBuf = ((this0->m_iCurrentOutBuf == 0) ? 1 : 0);		
		LOGD("render copy to buffer index =%d, next decode buffer=%d \n",
			irendindex,m_iCurrentOutBuf);
		m_pRender->PlayVideoData(m_yuvBuff[irendindex],&m_yuvmutex[irendindex]);
		//this0->m_pRender->m_renderbuffer = this0->m_yuvBuff[irendindex];
		//this0->m_pRender->SendRenderMsg(REFRESH_EVENT);
		//this0->Push_Video_Data(iwidth,iHeight,pframe,pkt.dts);
	
	return 0;
}
int TSDecoder_Instance::showAudioData(AVFrame* pframe,unsigned long ulpts)
{
		uint8_t *samples = (uint8_t*)m_audioBuff;
					int frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
					int ret = 0;
#if 0
			            if ((ret = av_buffersrc_add_frame(this0->in_audio_filter, pframe)) < 0)
				   {
					LOGD("error add frame to filter \n");
					av_free_packet(&pkt);
					break;
				   }
					AVFrame* filt_frame = av_frame_alloc();  
  			              if (!filt_frame)   
                			{  
                   			 ret = AVERROR(ENOMEM);  
                    			break;  
                			}  
  			while ((ret = av_buffersink_get_frame_flags(this0->out_audio_filter, filt_frame, 0)) >= 0) {
					LOGD("get avfilter frame pts=%"PRIx64"  \n",filt_frame->pts);
					//samples = pframe->data[0];
        				frame_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(filt_frame),
                                           filt_frame->nb_samples,
                                           (AVSampleFormat)filt_frame->format, 1);

		//			av_opt_get_sample_fmt(this0->out_audio_filter, "sample_fmts", sample_fmts, )
					int fmt_len = av_get_bytes_per_sample(sample_fmts[0]);
					LOGD("get avfilter frame size=%d per samplelen=%d \n",frame_size,fmt_len);//yyd
					if(fmt_len<0){
						fprintf(stderr,"Failed to calculate data size\n");
						av_free_packet(&pkt);
						break;
					}
					memset(samples,0,sizeof(AVCODEC_MAX_AUDIO_FRAME_SIZE));
					frame_size = 0;
					LOGD("frame nb_samples=%d,channels=%d \n",filt_frame->nb_samples,filt_frame->channels);
			            	for (int i=0; i<filt_frame->nb_samples; i++){
			                	for (int ch=0; ch<filt_frame->channels; ch++){	                
							memcpy(samples+frame_size,pframe->data[ch]+fmt_len*i,fmt_len);
							frame_size += fmt_len;
		                		}
					}
					LOGD("yyd info audio decode to play len =%d \n",frame_size);
	#if 1
					if(NULL==fpyuv) fpyuv=fopen("./decode.pcm","wb");
					if(fpyuv){
						fwrite(samples,1,frame_size,fpyuv);
						fflush(fpyuv);	
					}
	#endif
					
					frame_size = this0->init_SwrConverAudio(filt_frame,samples);
					this0->m_pRender->PlayAudioData(samples,frame_size);
					
#ifdef DUMP_PCM
					this0->m_pRender->SendRenderMsg(PCM_TEST);
#endif
           		 	}
				av_frame_free(&filt_frame);  
					
#else		//no audio
	//				frame_size = this0->init_SwrConverAudio(pframe,samples);
#if 0
					//no swr org freq pcm
					  frame_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(pframe),
                                           pframe->nb_samples,
                                           (AVSampleFormat)pframe->format, 1);
					LOGD("begin get avfilter frame linselen=%d len=%d \n",pframe->linesize[0],
							frame_size);
#if 0
					if(NULL==fpyuv) fpyuv=fopen("./decode.pcm","wb");
					if(fpyuv){
						fwrite(pframe->data[0],1,frame_size,fpyuv);
						fflush(fpyuv);	
					}
#endif

					int fmt_len = av_get_bytes_per_sample(sample_fmts[0]);
					LOGD("get avfilter frame size=%d per samplelen=%d \n",frame_size,fmt_len);//yyd
					if(fmt_len<0){
						fprintf(stderr,"Failed to calculate data size\n");
						av_free_packet(&pkt);
						break;
					}
					memset(samples,0,sizeof(AVCODEC_MAX_AUDIO_FRAME_SIZE));
					frame_size = 0;
					LOGD("frame nb_samples=%d,channels=%d \n",pframe->nb_samples,pframe->channels);
			            	for (int i=0; i<pframe->nb_samples; i++){
			                	for (int ch=0; ch<pframe->channels; ch++){	                
							memcpy(samples+frame_size,pframe->data[ch]+fmt_len*i,fmt_len);
							frame_size += fmt_len;
		                		}
					}
					LOGD("yyd info audio decode to play len =%d \n",frame_size);
					//this0->m_pAudioPlay->PlayData(samples,frame_size);
#endif	
					frame_size = init_SwrConverAudio(pframe,samples);
					m_pRender->PlayAudioData(samples,frame_size);
					
					m_pRender->SendRenderMsg(PCM_TEST);
#endif
	return 0;
}


static inline
int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
                   enum AVSampleFormat fmt2, int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (channel_count1 == 1 && channel_count2 == 1)
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    else
        return channel_count1 != channel_count2 || fmt1 != fmt2;
}

static inline
int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
}
static int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
                                 AVFilterContext *source_ctx, AVFilterContext *sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;

	LOGD("begin configure filter graph \n");
    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs  = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;

        inputs->name        = av_strdup("out");
        inputs->filter_ctx  = sink_ctx;
        inputs->pad_idx     = 0;
        inputs->next        = NULL;

        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
            goto fail;
    } else {
	LOGD("begin avfilter_link \n");
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
/*    for (i = 0; i < graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);
*/

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}

int TSDecoder_Instance::configure_audio_filters( const char *afilters, int force_output_format)
{
//    static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int sample_rates[2] = { 0, -1 };
    int64_t channel_layouts[2] = { 0, -1 };
    int channels[2] = { 0, -1 };
    AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
    char aresample_swr_opts[512] = "";
    AVDictionaryEntry *e = NULL;
    char asrc_args[256];
    int ret;

    avfilter_graph_free(&agraph);
    if (!(agraph = avfilter_graph_alloc())){
	LOGE("avfilter graph alloc error :%d \n",AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
	}
	LOGD("avfilter graph alloc success \n");
/*
    while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
        av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
    if (strlen(aresample_swr_opts))
        aresample_swr_opts[strlen(aresample_swr_opts)-1] = '\0';
    av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

    ret = snprintf(asrc_args, sizeof(asrc_args),
                   "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d:channel_layout=0x%PRIx64",
                   audio_filter_src.freq, av_get_sample_fmt_name(audio_filter_src.fmt),
                   audio_filter_src.channels,
                   1, audio_filter_src.freq,audio_filter_src.channel_layout);
*/

	ret = snprintf(asrc_args, sizeof(asrc_args),  
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%d ",  
            m_pAudioCodecCtx->time_base.num, m_pAudioCodecCtx->time_base.den, m_pAudioCodecCtx->sample_rate,  
            av_get_sample_fmt_name(m_pAudioCodecCtx->sample_fmt),  
            m_pAudioCodecCtx->channel_layout); 

	LOGE("creat src args:%s \n",asrc_args);
/* 
   if (audio_filter_src.channel_layout)
        snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
                 ":channel_layout=0x%"PRIx64,  audio_filter_src.channel_layout);
*/
	LOGD("avfilter create src begin \n");
	AVFilter *buffersrc = avfilter_get_by_name("abuffer");
	if(buffersrc !=NULL)
		LOGE("avfilter get byname bufferfiletr success \n");
    ret = avfilter_graph_create_filter(&filt_asrc,
                                       buffersrc, "bbcv_abuffer",
                                      asrc_args, NULL, agraph);
    if (ret < 0){
	LOGE("error creat filter :%d \n",ret);
        goto end;
	}

	LOGE("avfilter graph creat src filter success \n");

    ret = avfilter_graph_create_filter(&filt_asink,
                                       avfilter_get_by_name("abuffersink"), "bbcv_abuffersink",
                                       NULL, NULL, agraph);
    if (ret < 0)
        goto end;

	LOGE("avfilter graph creat sink filter success \n");
    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts,  AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;
	
	LOGD("avfilter graph set sink filter fmt success \n");
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;

	LOGD("avfilter graph set sink filter channel count success \n");
    if (force_output_format) {
        channel_layouts[0] = audio_filter_src.channel_layout;  //靠
     //   channel_layouts[0] = 0;
        channels       [0] = sdlParam.audioParam.audioChannels;
        sample_rates   [0] = sdlParam.audioParam.audioFreq;
        if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts,  -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_counts" , channels       ,  -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "sample_rates"   , sample_rates   ,  -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
	LOGD("avfilter graph set sink filter force success \n");
    }

//	logd ndk error
//	LOGD("output filter channels=%d,rate=%d,channelout=%d,fmt=%s \n",channels[0],sample_rates[0],channel_layouts[0],
//		av_get_sample_fmt_name(sample_fmts[0]));

    if ((ret = configure_filtergraph(agraph, afilters, filt_asrc, filt_asink)) < 0)
        goto end;

	LOGE("avfilter graph configure sink src filter  success ret = %d \n",ret);
    in_audio_filter  = filt_asrc;
    out_audio_filter = filt_asink;

end:
    if (ret < 0)
        avfilter_graph_free(&agraph);
    return ret;
}

int TSDecoder_Instance::init_audioFilter()
{
	LOGD("init audio filter 1 \n");
	avfilter_register_all();
	LOGD("init audio filter 2 \n");
            AVFilterLink *link;
		int ret = -1;
           	audio_filter_src.freq           = m_pAudioCodecCtx->sample_rate;
            audio_filter_src.channels       = m_pAudioCodecCtx->channels;
            audio_filter_src.channel_layout = get_valid_channel_layout(m_pAudioCodecCtx->channel_layout, m_pAudioCodecCtx->channels);
            audio_filter_src.fmt            = m_pAudioCodecCtx->sample_fmt;
// logd ndk error 
//	LOGD("audio src: freq=%d,channels=%d,channelout=%d,fmt=%d \n",audio_filter_src.freq,
//		audio_filter_src.channels,audio_filter_src.channel_layout,audio_filter_src.fmt);
            //if ((ret = configure_audio_filters("anull",1)) < 0){
	LOGD("init audio filter 3 \n");
            if ((ret = configure_audio_filters(NULL,1)) < 0){
		LOGE("error configure audio filters \n");
                return -1;
		}
            link = out_audio_filter->inputs[0];
            int sample_rate    = link->sample_rate;
            int nb_channels    = link->channels;
            int channel_layout = link->channel_layout;

	LOGD("init audio filter over \n");
//		LOGD("link rate=%d,channels=%d,channellayout=%d \n",sample_rate,nb_channels,channel_layout);

}

int TSDecoder_Instance::AudioResample(AVFrame * pAudioDecodeFrame,uint8_t * out_buf)
{
	int datasize = 0;
	

	return datasize;
}

int TSDecoder_Instance::init_SwrConverAudio(AVFrame * pAudioDecodeFrame,uint8_t * out_buf)
{
#if 1
	int data_size = 0;	
	int ret = 0;  
	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO; //初始化这样根据不同文件做调整  
	int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;  // AV_CH_LAYOUT_SURROUND; ; //这里设定ok  
	int dst_nb_channels = 0;  
	int dst_linesize = 0;  
	int src_nb_samples = 0;  
	int dst_nb_samples = 0;  
	int max_dst_nb_samples = 0;  
	uint8_t **dst_data = NULL;	
	int resampled_data_size = 0; 
	SwrContext * swr_ctx = m_swr_ctx; 
	int out_channels =2;
	int out_sample_fmt = 1; //AV_SAMPLE_FMT_S16;
	int out_sample_rate = 48000;

	if(NULL == m_swr_ctx)
	{


		swr_ctx = swr_alloc();	
		if (!swr_ctx)  
		{  
		   LOGE("swr_alloc error \n");  
		   return -1;  
		}  
		src_ch_layout = (m_pAudioCodecCtx->channel_layout &&   
			  m_pAudioCodecCtx->channels ==   
			  av_get_channel_layout_nb_channels(m_pAudioCodecCtx->channel_layout)) ?	
			  m_pAudioCodecCtx->channel_layout :	
		  	  av_get_default_channel_layout(m_pAudioCodecCtx->channels);  
#if 0
		if (out_channels == 1)  
		{  
		  dst_ch_layout = AV_CH_LAYOUT_MONO;  
		}  
		else if(out_channels == 2)  
		{  
		  dst_ch_layout = AV_CH_LAYOUT_STEREO;	
		}  
		else	
		{  
		  //可扩展	
		} 
#endif			
		if (src_ch_layout <= 0)  
		{  
		  LOGE("src_ch_layout error \n");  
		  return -1;  
		}  
		src_nb_samples = pAudioDecodeFrame->nb_samples;  
	    if (src_nb_samples <= 0)  
	    {  
	        LOGE("src_nb_samples error \n");  
	        return -1;  
	    }  
		
		LOGD("---src_ch_layout =%ld ,in_channel_layout=%d, src_fmt =%ld,in_sample_rate =%ld,\n",m_pAudioCodecCtx->channels,src_ch_layout, 
			m_pAudioCodecCtx->sample_fmt,m_pAudioCodecCtx->sample_rate);	
	
#if 1	  
	    /* set options */  
	    av_opt_set_int(swr_ctx, "in_channel_layout",    m_pAudioCodecCtx->channels, 0);  
	    av_opt_set_int(swr_ctx, "in_sample_rate",       m_pAudioCodecCtx->sample_rate, 0);  
	    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", m_pAudioCodecCtx->sample_fmt, 0);  
	  
	    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);  
	    av_opt_set_int(swr_ctx, "out_sample_rate",       out_sample_rate, 0);  
	    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)out_sample_fmt, 0);
		
		
#endif
 
		swr_init(swr_ctx);  
		m_swr_ctx = swr_ctx;
	}
	src_nb_samples = pAudioDecodeFrame->nb_samples;  
    if (src_nb_samples <= 0)  
    {  
        LOGE("src_nb_samples error \n");  
        return -1;  
    }  

	LOGD("--src_nb_samples=%d,out_samplerate=%d \n",src_nb_samples, out_sample_rate);
	max_dst_nb_samples = dst_nb_samples =  
        av_rescale_rnd(src_nb_samples, out_sample_rate, m_pAudioCodecCtx->sample_rate, AV_ROUND_UP); 
	LOGD("--src sample rate=%d ,dst_nb_samples=%d \n",m_pAudioCodecCtx->sample_rate,dst_nb_samples);
    if (max_dst_nb_samples <= 0)  
    {  
        LOGE("av_rescale_rnd error \n");  
        return -1;  
    }  
  
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);  
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,  
        dst_nb_samples, (AVSampleFormat)out_sample_fmt, 0);  
    if (ret < 0)  
    {  
        LOGE("av_samples_alloc_array_and_samples error \n");  
        return -1;  
    }  

    dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, m_pAudioCodecCtx->sample_rate) +  
        src_nb_samples, out_sample_rate, m_pAudioCodecCtx->sample_rate,AV_ROUND_UP);  
	//LOGD("dst_nb_samples = %d \n",dst_nb_samples);
	if (dst_nb_samples <= 0)  
    {  
        LOGE("av_rescale_rnd error \n");  
        return -1;  
    }  
    if (dst_nb_samples > max_dst_nb_samples)  
    {  
        av_free(dst_data[0]);  
        ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,  
            dst_nb_samples, (AVSampleFormat)out_sample_fmt, 1);  
        max_dst_nb_samples = dst_nb_samples;  
    }  
  
    data_size = av_samples_get_buffer_size(NULL, m_pAudioCodecCtx->channels,  
        pAudioDecodeFrame->nb_samples,  
        m_pAudioCodecCtx->sample_fmt, 1);  
    if (data_size <= 0)  
    {  
        LOGE("av_samples_get_buffer_size error \n");  
        return -1;  
    }  
    resampled_data_size = data_size;  
	LOGD("src data size=%d \n",data_size);

	if (swr_ctx)  
    {  
      //  ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,   
      //      (const uint8_t **)pAudioDecodeFrame->data, pAudioDecodeFrame->nb_samples);  
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,   
            (const uint8_t **)pAudioDecodeFrame->extended_data, pAudioDecodeFrame->nb_samples);  
        if (ret <= 0)  
        {  
            LOGE("swr_convert error \n");  
            return -1;  
        }  
  
        resampled_data_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,  
            ret, (AVSampleFormat)out_sample_fmt, 1);  
        if (resampled_data_size <= 0)  
        {  
            LOGE("av_samples_get_buffer_size error \n");  
            return -1;  
        }  
    }  
    else   
    {  
        LOGE("swr_ctx null error \n");  
        return -1;  
    }  
  	//LOGD("--swr convert len=%d\n",resampled_data_size);
    //将值返回去  
    memcpy(out_buf,dst_data[0],resampled_data_size);  
  
    if (dst_data)  
    {  
        av_freep(&dst_data[0]);  
    }  
    av_freep(&dst_data);  
    dst_data = NULL;  
  
    return resampled_data_size;  
#endif
}

int TSDecoder_Instance::get_queue(uint8_t* buf, int size) 
{
//	LOGD("yyd info get queue begin size=%d \n",size);
	int num = size / TSPACKETLEN;
	int index = 0;
	uint8_t * bufhead = buf;
	while(index < num)
	{
		int ret = m_recvqueue.get_queue(buf,TSPACKETLEN);
		if(ret >= 0)
		{
			//分析ts流数据
		//	m_tsStreamPrase.ParseStreamFrame(buf,TSPACKETLEN);
			buf += TSPACKETLEN;
			++index;
		}
		else
		   usleep(10);
	}
#if 0
	if(m_bSaveTSStream && size > 0)
	{
		if(NULL == m_tsStreamfp)
		{

			char mediaPath[512]={0};
			/*
			GetCurrentDirectory(sizeof(mediaPath),mediaPath);

			SYSTEMTIME systm;
			GetLocalTime(&systm);
			char tmpTime[128]={0};
			sLOGD(tmpTime,"\\\%2d-%2d-%2d.ts",systm.wHour,systm.wMinute,systm.wSecond);
			strcat(mediaPath,tmpTime);
	*/
			sprintf(mediaPath,"test.ts");
#ifdef ENABLEFILE
			m_tsStreamfp = fopen(mediaPath,"wb");
#endif
		}
		if(m_tsStreamfp)
		{
			fwrite(bufhead,1,size,m_tsStreamfp);
			fflush(m_tsStreamfp);
		}

	}
#endif
//	LOGD("yyd info get queue end over size=%d \n",size);
	return 0;
}

//FILE* fpreaddata= NULL;
int TSDecoder_Instance::read_data(void *opaque, uint8_t *buf, int buf_size) {
//	UdpParam udphead;
	TSDecoder_Instance* this0 = (TSDecoder_Instance*)opaque;
	int size = buf_size;
	//int size = 2048;
	int ret;
//	if(NULL == fpreaddata)
//		fpreaddata = fopen("readdata.pes","wb+");
//	LOGD("read data %d\n", buf_size);
	do {
		
		ret = this0->get_queue(buf, size);
		if(ret < 0)
			usleep(1000);
		//LOGD("-------read_data ret = %d size=%d\n",ret,buf_size);
		//size += ret;
	} while (ret < 0);
	//LOGD("read data Ok %d\n", buf_size);
//	if(this0->m_bDecoderFlag)
//		fwrite(buf,1,size,fpreaddata);
	return size;
}

bool TSDecoder_Instance::open_inputdata()
{
	AVCodec *pVideoCodec, *pAudioCodec;
	AVInputFormat *piFmt = NULL;

	struct timeval tm;
/*
	SYSTEMTIME systm;

	//gettimeofday(&tm,NULL);
	//LOGD("-----init time 1 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
//#ifdef PROBE_DATA
	// 开始探测码流
	//
	GetLocalTime(&systm);
	fLOGD(fpLog,"%2d:%2d:%3d 开始探测码流类型\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
	fflush(fpLog);
*/
	int iloop = 5;
	do
	{
		if (av_probe_input_buffer(m_pb, &piFmt, "", NULL, 0, 188*7) < 0) {
			LOGD("probe failed!\n");
			usleep(1000);
		} else {
			LOGD("probe success!\n");
			//LOGD("format: %s[%s]\n", piFmt->name, piFmt->long_name);
			break;
		}
		
	}while(iloop-- >=0);
	m_pFmt->pb = m_pb;
/*
	memset(&systm,0,sizeof(systm));
	GetLocalTime(&systm);
	fLOGD(fpLog,"%2d:%2d:%3d 探测码流成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
	fflush(fpLog);
*/
	if (avformat_open_input(&m_pFmt, "", piFmt, NULL) < 0) {
		//LOGD("avformat open failed.\n");
		/*
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据失败\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
		*/
		return -1;
	} else {
	/*
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
		*/
	}
//#else
/*	// no probe
	if (pFFAVFormat->ff_avformat_open_input(&m_pFmt, m_cfilename, NULL, NULL) < 0) {
		//LOGD("avformat open failed.\n");
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据失败\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
		return -1;
	} else {
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
	}
*/
//#endif
	
	gettimeofday(&tm,NULL);
	LOGD("-----loop =%d	init time 2 =%ld\n",10-iloop,tm.tv_sec*1000+tm.tv_usec/1000);

	enum AVCodecID videoCodeID;
	enum AVCodecID audioCodeID;

	switch(m_vCodetype)
	{
		case CODE_HD_VIDEO:
		{
			videoCodeID = AV_CODEC_ID_H264;
			break;
		}
		case CODE_SD_VIDEO:
		{
			videoCodeID = AV_CODEC_ID_MPEG2VIDEO;
			break;
		}
		default:
		{
			videoCodeID = AV_CODEC_ID_MPEG2VIDEO;
			break;
		}
	}

	switch(m_aCodeType)
	{
		case CODE_AUIDO_MP2:
		{
			audioCodeID = AV_CODEC_ID_MP2;
			break;
		}
		case CODE_AUIDO_MP3:
		{
			audioCodeID = AV_CODEC_ID_MP3;
			break;
		}
		case CODE_AUDIO_AAC:
		{
			audioCodeID = AV_CODEC_ID_AAC;
			break;
		}
		default:
		{
			audioCodeID = AV_CODEC_ID_MP2;
			break;
		}
	}

/*
	if (pFFAVFormat->ff_av_find_stream_info(m_pFmt) < 0)
	{
		fLOGD(stderr, "could not fine stream.\n");
		return -1;
	}

	pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if (!pVideoCodec) {
		fLOGD(stderr, "could not find video decoder!\n");
		return -1;
	}
	if (avcodec_open(m_pVideoCodecCtx, pVideoCodec) < 0) {
		fLOGD(stderr, "could not open video codec!\n");
		return -1;
	}


	pAudioCodec = avcodec_find_decoder(m_pAudioCodecCtx->codec_id);
	if (!pAudioCodec) {
		fLOGD(stderr, "could not find audio decoder!\n");
		return -1;
	}
	if (avcodec_open(pAudioCodecCtx, pAudioCodec) < 0) {
		fLOGD(stderr, "could not open audio codec!\n");
		return -1;
	}
*/	

	pVideoCodec = avcodec_find_decoder(videoCodeID);
	if (!pVideoCodec) {
		LOGD("could not find video decoder!\n");
		return false;
	}
	if (avcodec_open2(m_pVideoCodecCtx, pVideoCodec,NULL) < 0) {
		LOGD("could not open video codec!\n");
		return false;
	}


//	LOGD("**********vidoe eum=%d auido enum=%d*******video type=%d audiotype=%d \n",m_vCodetype,m_aCodeType,videoCodeID,audioCodeID);

	pAudioCodec = avcodec_find_decoder(audioCodeID);
	if (!pAudioCodec) {
		LOGD("could not find audio decoder!\n");
		return false;
	}
	if (avcodec_open2(m_pAudioCodecCtx, pAudioCodec,NULL) < 0) {
		LOGD("could not open audio codec!\n");
		return false;
	}
	/*
	memset(&systm,0,sizeof(systm));
	GetLocalTime(&systm);
	fLOGD(fpLog,"%2d:%2d:%3d 媒体流数据，视频类型：H264, 音频类型 mp2 \n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
	fflush(fpLog);
*/
	m_pVideoCodec = pVideoCodec;
	m_pAudioCodec = pAudioCodec;
	m_pframe = NULL;

	return true;
}

int TSDecoder_Instance::initdecoder()
{
	
//	AVCodec *pVideoCodec, *pAudioCodec;
	AVCodecContext *pVideoCodecCtx = NULL;
	AVCodecContext *pAudioCodecCtx = NULL;
	AVIOContext * pb = NULL;
//	AVInputFormat *piFmt = NULL;
	AVFormatContext *pFmt = NULL;


	struct timeval tm;

	av_register_all();
	avformat_network_init();
	avfilter_register_all();

	std::string strtmp=m_cfilename;
	std::size_t found  = strtmp.find(":");
	if (found != std::string::npos)
		m_ifileType = internetfile;
	else
		m_ifileType = localfile;
	LOGD("-------------file Type=%d\n",m_ifileType);

	int port = 0;

	{
		//  udp://@:14000
		std::size_t found  = strtmp.find("@:");
		if (found != std::string::npos)
		{
			std::string tm = strtmp.substr(found+2,(strtmp.length()-found-2));
			//LOGD("-------get string sub %s \n",tm.c_str());	
			port = atoi(tm.c_str());
			LOGD("=============get port =%d \n",port);
			
		}
	}

		m_recvqueue.init_queue(Recv_Queue_size,port,m_strDstIP,m_iPort,fpLog,m_MediaInfofp);

		uint8_t *buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUF_SIZE);

		
		//pb = avio_alloc_context(buf, BUF_SIZE, 0, this, TSDecoder_Instance::read_data, NULL, NULL);
		pb = avio_alloc_context(buf, BUF_SIZE, 0, this, read_data, NULL, NULL);
		if (!pb) {
			LOGD("avio alloc failed!\n");
			return -1;
		}

		m_pb = pb;
		pFmt = avformat_alloc_context();

		//pVideoCodecCtx = avcodec_alloc_context();
		//pAudioCodecCtx = avcodec_alloc_context();

		pVideoCodecCtx = NULL;
		pAudioCodecCtx = NULL;
		m_pFmt = pFmt;
		m_pVideoCodecCtx = pVideoCodecCtx;
		m_pAudioCodecCtx = pAudioCodecCtx;

		//分辨率 码率 GOP值  AVCodecContext

	pthread_mutex_init(&m_locker,NULL);
	pthread_mutex_init(&m_audioLocker,NULL);
	
	initQueue(Queue_size);

	return 0;
}

int TSDecoder_Instance::init_SDL(SDLParam sdlParam)
{
	//init befor init decoder
	//render
	m_pRender = new BBCVRender();
	if(NULL== m_pRender)
	{
		LOGD("new sdl render instance error ");
		return -1;
	}
	return m_pRender->InitAll(sdlParam);
	
/*	
	m_pRender->Init();
	AudioParam pm;
	memset(&pm,0,sizeof(pm));

	pm.audioFreq = 48000;
	pm.audioFormat = 4;// s16
	pm.audioChannels = 2;
	pm.audiosilence = 0;
	pm.audioSamples = 2048;
	m_pRender->InitAudioPlayer(pm);	
*/
	return 0;
}

int TSDecoder_Instance::init_TS_Decoder(const char* cfilename,DecoderControlParam dcParam)
{
	memset(&m_dcControlParam,0,sizeof(DecodeParam));
	memcpy(&m_dcControlParam,&dcParam,sizeof(DecodeParam));

	strcpy(m_cfilename,cfilename);
	strcpy(m_strDstIP,dcParam.strDstIP);
	m_iPort = dcParam.iport;
	LOGE("----filename=%s dstip=%s  port=%d \n",m_cfilename,m_strDstIP,m_iPort);
	m_vCodetype=dcParam.vCodetype;
	m_aCodeType=dcParam.aCodetype;
	m_bDecoderFlag = dcParam.bAutoMatch;
	m_bSmoothPlay = dcParam.bSmooth;
	m_bNeedControlPlay = dcParam.bNeedControlPlay;
	int ret = -1;
	m_yuvBuff[0] = new unsigned char[1920*1080*3/2];
	m_yuvBuff[1] = new unsigned char[1920*1080*3/2];
	m_yuvBuff[2] = new unsigned char[1920*1080*3/2];
	for(int i=0;i<RenderBufferSize;++i){
		pthread_mutex_init(&m_yuvmutex[i],NULL);
	}
	m_iCurrentOutBuf = 0;

	m_audioBuff = new unsigned char[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	
        #if 1
//#ifdef H264_ONLY
        if(!m_bDecoderFlag)//false 指定播放
        {
                ret = initdecoder();
                if(ret < 0)
                {
                        LOGE("yyd info initdecoder failed  \n");
                        return NULL;
                }
                if(open_inputdata())
                {
                        m_recvqueue.m_bInitDecoder = true;
                        //清空缓冲区 存在一开始就叠加情况

                        //this0->m_recvqueue.read_ptr = 0;
                        //this0->m_recvqueue.write_ptr = 0;
                //      Sleep(1);
                //      this0->m_recvqueue.clean_RecvQue();
                //      this0->Clean_Video_audioQue();
                        LOGE("yyd info -----init open inputdata selfdefine format successs\n");
                }
        //#else
        }
        else    //true 自动匹配
        {
                ret = init_open_input();
                if(ret < 0)
                {
                        LOGE("initdecoder failed\n");
                        return NULL;
                }
                LOGE("------init_open_input in main thread auto match input successs\n");
        }
	
	memset(&sdlParam,0,sizeof(SDLParam));
        sdlParam.iScreenHeight =  m_pVideoCodecCtx->height;
        sdlParam.iScreenWidth = m_pVideoCodecCtx->width;
 //       sdlParam.iScreenHeight =  480;
 //       sdlParam.iScreenWidth = 640;
        sdlParam.iTextureHeight =  m_pVideoCodecCtx->height;
        sdlParam.iTextureWidth = m_pVideoCodecCtx->width;
        sdlParam.audioParam.audioChannels =  2;
        sdlParam.audioParam.audioFormat = 4; //s16
        sdlParam.audioParam.audioFreq = 48000;
        sdlParam.audioParam.audioSamples = 512;
        sdlParam.audioParam.audiosilence = 0;
	
	init_audioFilter();
	//init filter video
	#ifdef ANDROID_PLAMT
	// 靠靠
	m_avfilter_XH.init_filters(m_pVideoCodecCtx,"movie=/sdcard/my_logo.png[wm];[in][wm]overlay=5:5[out]");
	#else
	m_avfilter_XH.init_filters(m_pVideoCodecCtx,"movie=my_logo.png[wm];[in][wm]overlay=5:5[out]");
        #endif
	m_avfilter_XH.set_push_func(this);    	
        int iret = init_SDL(sdlParam);
#endif 


//yyd no audio
/*
	SYSTEMTIME systm;
	memset(&systm,0,sizeof(systm));
	GetLocalTime(&systm);
	fLOGD(fpLog,"%2d:%2d:%3d 流地址：%s  初始化开始。。。\n",systm.wMinute,systm.wSecond,systm.wMilliseconds,m_cfilename);
	fflush(fpLog);
*/
	//兼容rtsp
	//URLTYPE m_ifileType= tsUDP;
/*	char strURL[1024]={0};
	strcpy(strURL,m_cfilename);
	int port = 0;
	char cUDPPort[8] = {0};

	char pURL[1024] = {0};
	strcpy(pURL,strURL);
	char *pFind1 = strstr(pURL,"udp:");
	char *pFind2 = strstr(pURL,"rtsp:");
*/
	//  udp://@:14000 rtsp://192.168.20.131:8554/sd.ts
//	if(pFind1 != NULL)
	{
		//ts 流
		m_iURLType = tsUDP;
		pthread_t Decoder_thread;
			pthread_create(&Decoder_thread, NULL, TSDecoder_Instance::decoder_threadFun, this);
			pthread_detach(Decoder_thread);
		//启动数据回调线程
		/*
		HANDLE ThreadHandle;
		ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, decoder_threadFun, (LPVOID)this, 0, &m_iThreadid);
		CloseHandle (ThreadHandle);
		*/
	}
#if 0
	else if(pFind2 != NULL)
	{
		//rtsp 流
		m_iURLType = tsRTSP;

		char strRtspIP[256] ={0};
		//获取 ip 端口 资源
		char* pfindport1 = strstr(pURL,"//");
		if(pfindport1 != NULL)
		{

			char* pfindport2 = strstr(pfindport1,":");
			if(pfindport2)
			{
				char strPorttmp[1024] ={0};
				strcpy(strPorttmp,pfindport2);
				

				int ilen = pfindport2 - pfindport1;
				*(pfindport1+ilen) = '\0';
				strcpy(strRtspIP,pfindport1+2);
				char* pfingport3 = strstr(strPorttmp,"/");
				if(pfingport3)
				{
					*pfingport3 = '\0';

					port = atoi(strPorttmp+1);
				}
			}
		}
		//获取rtsp server的ip Port，
		memset(m_strRtspServerIP,0,sizeof(m_strRtspServerIP));
		strcpy(m_strRtspServerIP,strRtspIP);
		m_iRtspPort = port;

		//创建Myrtsp对象

		//开启rtsp连接线程

		//开启rtsp客户端

	}

#endif	
//	m_pAudioPlay = new AudioPlay();

	int iSampleSec = 48000;
//	m_pAudioPlay->InitDSound(hwnd,iSampleSec);
//	Set_tsDecoder_Volume(0);

//	m_pAudioPlay->StartPlay();

	return true;
}

bool TSDecoder_Instance::initQueue(int isize)
{
	for(int i=0;i<isize;++i)
	{
		VideoData* tmpbuff = new VideoData;
		tmpbuff->data = new unsigned char[Video_Buff_size];
		m_freeQueue.push_back(tmpbuff);
	}
	
	//音频多点缓存
	for(int i=0;i<isize*5/2;++i)
	{
		AudioData* audiobuff = new AudioData;
		audiobuff->data = new unsigned char[Audio_Buff_size];
		m_audioFreeQueue.push_back(audiobuff);
	}
	
	
	return true;
}

bool TSDecoder_Instance::freeQueue()
{

	//EnterCriticalSection(&m_locker);
	pthread_mutex_lock(&m_locker);
	while(m_usedQueue.size() > 0)
	{
		VideoData* tmpbuff = m_usedQueue.front();
		m_usedQueue.pop_front();
		delete tmpbuff->data;
		tmpbuff->data = NULL;
		delete tmpbuff;
	}
	while(m_freeQueue.size() > 0)
	{
		VideoData* tmpbuff = m_freeQueue.front();
		m_freeQueue.pop_front();
		delete tmpbuff->data;
		delete tmpbuff;
		tmpbuff = NULL;
	}

	pthread_mutex_unlock(&m_locker);

	pthread_mutex_lock(&m_audioLocker);
	while(m_audioFreeQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioFreeQueue.front();
		m_audioFreeQueue.pop_front();
		delete audiobuff->data;
		delete audiobuff;
		audiobuff = NULL;
	}
	while(m_audioUsedQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioUsedQueue.front();
		m_audioUsedQueue.pop_front();
		delete audiobuff->data;
		delete audiobuff;
		audiobuff = NULL;
	}

	pthread_mutex_unlock(&m_audioLocker);
	return true;
}


FILE* fpYUV = NULL;
void TSDecoder_Instance::Push_Video_Data(int iWidth, int iHight, AVFrame *pAVfram,unsigned long ulPTS)
{
	pthread_mutex_lock(&m_locker);
	if(m_freeQueue.size() > 0)
	{
		VideoData* tempbuff = m_freeQueue.front();
		tempbuff->iHeight = iHight;
		tempbuff->iWidth = iWidth;
		tempbuff->pts = ulPTS;

		unsigned char *buff = tempbuff->data;
		for (int i = 0; i < iHight; i++)
		{
			memcpy(buff+i*iWidth, pAVfram->data[0]+i*pAVfram->linesize[0], iWidth);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth+i*iWidth/2, pAVfram->data[1]+i*pAVfram->linesize[1], iWidth/2);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth*5/4+i*iWidth/2, pAVfram->data[2]+i*pAVfram->linesize[2], iWidth/2);
		}
		/*
		if(fpYUV == NULL)
		{
			fpYUV = fopen("/home/ky/rsm-yyd/DecoderTs/jhxyuv.dat", "w+b");
		}
		fwrite(buff, 1, iWidth*iHight*3/2, fpYUV);
		*/
		m_freeQueue.pop_front();
		m_usedQueue.push_back(tempbuff);
	}
	else if(m_usedQueue.size()>0)
	{
		VideoData* tempbuff = m_usedQueue.front();
		tempbuff->iHeight = iHight;
		tempbuff->iWidth = iWidth;
		tempbuff->pts = ulPTS;

		unsigned char *buff = tempbuff->data;
		for (int i = 0; i < iHight; i++)
		{
			memcpy(buff+i*iWidth, pAVfram->data[0]+i*pAVfram->linesize[0], iWidth);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth+i*iWidth/2, pAVfram->data[1]+i*pAVfram->linesize[1], iWidth/2);
		}
		for (int i=0; i < iHight/2; i++)
		{
			memcpy(buff+iHight*iWidth*5/4+i*iWidth/2, pAVfram->data[2]+i*pAVfram->linesize[1], iWidth/2);
		}
		/*
		if(fpYUV == NULL)
		{
			fpYUV = fopen("/home/ky/rsm-yyd/DecoderTs/jhxyuv.dat", "w+b");
		}
		fwrite(buff, 1, iWidth*iHight*3/2, fpYUV);
		*/
		m_usedQueue.pop_front();
		m_usedQueue.push_back(tempbuff);
		LOGD("========full used front to back\n");
	}
	pthread_mutex_unlock(&m_locker);
}

int TSDecoder_Instance::get_video_param(int *iwidth,int *iheight)
{
	*iwidth = m_iWidht;
	*iheight = m_iHeight;
	return true;
}

int TSDecoder_Instance::get_Video_data(unsigned char *output_video_yuv420,int *output_video_size,
	int* iwidth,int* iHeight,unsigned long* video_pts)
{
	//fLOGD(stderr,"get video data\n");
/*	if(!m_bDecoderFlag && !m_recvqueue.m_bDelayFrame)
	{
		Clean_Video_audioQue();
		return -1;
	}*/
	pthread_mutex_lock(&m_locker);
/*	if(!m_bDecoderFlag && m_recvqueue.m_bDelayFrame)
	{
		//LOGD("-----delay frame time 1111\n");
		if(m_usedQueue.size() > 0 )
		{
			m_iDelayFrame++;
			LOGD("-----delay frame time 2222\n");
			VideoData* tempbuff = m_usedQueue.front();
			
			unsigned char *buff = tempbuff->data;
			*iwidth = tempbuff->iWidth;
			*iHeight = tempbuff->iHeight;
			if(video_pts)
				*video_pts = tempbuff->pts;
			
			int iyuvsize = tempbuff->iWidth * tempbuff->iHeight*3/2;
			if(*output_video_size < iyuvsize)
			{
				fLOGD(stderr,"output_video_size is too small \n");
				pthread_mutex_unlock(&m_locker);
				return -1;
			}
			*output_video_size = iyuvsize;
			memcpy(output_video_yuv420,buff,iyuvsize);
			tempbuff->iWidth = 0;
			tempbuff->iHeight = 0;
			m_usedQueue.pop_front();
			m_freeQueue.push_back(tempbuff);
			
			pthread_mutex_unlock(&m_locker);

			struct timeval tm;

			gettimeofday(&tm,NULL);
			LOGD("-----video que size =%d ,get Video Time =%ld\n",m_usedQueue.size(),tm.tv_sec*1000+tm.tv_usec/1000);
			//LOGD("pts=%ld,w=%d video queue used size =%d \n",m_ulvideo_pts,*iwidth,m_usedQueue.size());
			return 0;
		}
		else if(m_usedQueue.size() > 0)
		{
			m_iDelayFrame++;
			//m_recvqueue.m_bDelayFrame = false;
			LOGD("-----clean que begin\n");
			while(m_usedQueue.size() >0)
			{
				VideoData* tempbuff = m_usedQueue.front();
				tempbuff->iWidth = 0;
				tempbuff->iHeight = 0;
				m_usedQueue.pop_front();
				m_freeQueue.push_back(tempbuff);
			
			}
			pthread_mutex_unlock(&m_locker);
			LOGD("----clean video que \n");

			pthread_mutex_lock(&m_audioLocker);
			while(m_audioUsedQueue.size() > 0)
			{
				AudioData* audiobuff = m_audioUsedQueue.front();
				m_audioUsedQueue.pop_front();
				m_audioFreeQueue.push_back(audiobuff);
			}
			LOGD("----clean audio que \n");
			pthread_mutex_unlock(&m_audioLocker);
			
			return -1;
		}
		else
		{
			m_iDelayFrame++;
			pthread_mutex_unlock(&m_locker);
			return -1;
		}

	}
*/	
	if(m_usedQueue.size() >0)
	{
		VideoData* tempbuff = m_usedQueue.front();
		
		unsigned char *buff = tempbuff->data;
		*iwidth = tempbuff->iWidth;
		*iHeight = tempbuff->iHeight;
		if(video_pts)
			*video_pts = tempbuff->pts;
		//m_ulvideo_pts = tempbuff->pts; //控制音视频同步
		
		int iyuvsize = tempbuff->iWidth * tempbuff->iHeight*3/2;
		if(*output_video_size < iyuvsize)
		{
			LOGD("output_video_size is too small \n");
			pthread_mutex_unlock(&m_locker);
			return -1;
		}
		*output_video_size = iyuvsize;
		memcpy(output_video_yuv420,buff,iyuvsize);
		tempbuff->iWidth = 0;
		tempbuff->iHeight = 0;
		m_usedQueue.pop_front();
		m_freeQueue.push_back(tempbuff);
		
		pthread_mutex_unlock(&m_locker);



	}
	else
	{
		pthread_mutex_unlock(&m_locker);
		return -1;
	}
	return 0;
}

void TSDecoder_Instance::Push_Audio_Data(unsigned char *sample,int isize,unsigned long ulPTS)
{
	pthread_mutex_lock(&m_audioLocker);
	if(m_audioFreeQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioFreeQueue.front();
		audiobuff->pts = ulPTS;
		audiobuff->size = isize;

		unsigned char *tmpbuf = audiobuff->data;
		memcpy(tmpbuf,sample,isize);

		m_audioFreeQueue.pop_front();
		m_audioUsedQueue.push_back(audiobuff);
	}
	else if(m_audioUsedQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioUsedQueue.front();
		audiobuff->pts = ulPTS;
		audiobuff->size = isize;

		unsigned char* tmpbuf = audiobuff->data;
		memcpy(tmpbuf,sample,isize);

		m_audioUsedQueue.pop_front();
		m_audioUsedQueue.push_back(audiobuff);
	}
	
	pthread_mutex_unlock(&m_audioLocker);
}

int TSDecoder_Instance::get_Audio_data(unsigned char *output_audio_data,int* input_audio_size,
	unsigned long* audio_pts)
{
	pthread_mutex_lock(&m_audioLocker);
	if(m_audioUsedQueue.size() >0)
	{

		AudioData* tempbuff = m_audioUsedQueue.front();
		
		unsigned char *buff = tempbuff->data;
	
		if(audio_pts)
			*audio_pts= tempbuff->pts;
		
		if(*input_audio_size < tempbuff->size)
		{
			LOGD("input_audio_size is too small \n");
			pthread_mutex_unlock(&m_audioLocker);
			return -1;
		}
		*input_audio_size= tempbuff->size;
		memcpy(output_audio_data,buff,tempbuff->size);
		tempbuff->size = 0;
		
		
		m_audioUsedQueue.pop_front();
		m_audioFreeQueue.push_back(tempbuff);
	
		pthread_mutex_unlock(&m_audioLocker);
	}
	else
	{
		pthread_mutex_unlock(&m_audioLocker);
		return -1;
	}
	return 0;
}

void TSDecoder_Instance::stopDecoder(bool bstop)
{
	m_bstop = bstop;
}
/*
void av_free_packet(AVPacket *pkt)
{
    if (pkt) {
        if (pkt->destruct)
            pkt->destruct(pkt);
        pkt->data            = NULL;
        pkt->size            = 0;
        pkt->side_data       = NULL;
        pkt->side_data_elems = 0;
    }
}

void av_free(void *ptr)
{

    free(ptr);
}


void av_freep(void *arg)
{
    void **ptr= (void**)arg;
    av_free(*ptr);
    *ptr = NULL;
}

static void free_packet_buffer(AVPacketList **pkt_buf, AVPacketList **pkt_buf_end)
{
    while (*pkt_buf) {
        AVPacketList *pktl = *pkt_buf;
        *pkt_buf = pktl->next;
        av_free_packet(&pktl->pkt);
        av_freep(&pktl);
    }
    *pkt_buf_end = NULL;
}

*/
bool TSDecoder_Instance::set_tsDecoder_stat(bool bstat)
{

	m_bDecoderFlag = bstat; //true 为自动匹配，FALSE为指定解码
	return true;

}

int TSDecoder_Instance::init_open_input()
{
	
	AVCodec *pVideoCodec, *pAudioCodec;
	AVCodecContext *pVideoCodecCtx = NULL;
	AVCodecContext *pAudioCodecCtx = NULL;
	AVIOContext * pb = NULL;
	AVInputFormat *piFmt = NULL;
	AVFormatContext *pFmt = NULL;


//	struct timeval tm;
/*
			SYSTEMTIME systm;

			GetLocalTime(&systm);
			fLOGD(fpLog,"%2d:%2d:%3d AutoMatch开始初始化ffmpeg\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);
*/
	av_register_all();
	avformat_network_init();
	avfilter_register_all();

	std::string strtmp=m_cfilename;
	std::size_t found = strtmp.find("udp://");
	if (found != std::string::npos)
		m_ifileType = internetfile;
	else
	{
		m_ifileType = localfile;
	}
	LOGD("-------------file Type=%d\n",m_ifileType);

	if(m_ifileType == localfile || Use_ffmpeg_recv)
	{
		pFmt = avformat_alloc_context();
		if (avformat_open_input(&pFmt, m_cfilename, NULL, NULL) < 0)
		{
			LOGD( "avformat open failed.\n");
			return -1;
		} 
		else
		{
			LOGD("open stream local file success!\n");
		}
		
	}
	else if(m_ifileType == internetfile)
	{
		//  udp://@:14000
		std::size_t found  = strtmp.find("@:");
		int port = 0;
		if (found != std::string::npos)
		{
			std::string tm = strtmp.substr(found+2,(strtmp.length()-found-2));
			//LOGD("-------get string sub %s \n",tm.c_str());	
			port = atoi(tm.c_str());
			LOGD("=============get port =%d \n",port);
			
		}
		m_recvqueue.init_queue(Recv_Queue_size,port,m_strDstIP,m_iPort,fpLog,m_MediaInfofp,m_bNeedControlPlay);
		
		uint8_t *buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUF_SIZE*2);
		//uint8_t *buf = (uint8_t*)malloc(sizeof(uint8_t)*BUF_SIZE);
		
		pb = avio_alloc_context(buf, BUF_SIZE, 0, this, TSDecoder_Instance::read_data, NULL, NULL);
		if (!pb) {
			LOGD( "avio alloc failed!\n");
			return -1;
		}

		
		// 开始探测码流
		/*
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 开始探测码流类型\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
		*/
		//gettimeofday(&tm,NULL);
	//	LOGD("-----init time 1 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
		int iloop = 5;
		do
		{
			if (av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 188*7*2) < 0) {
				LOGD("probe failed!\n");
				usleep(1000);
			} else {
				LOGD("probe success!\n");
				//LOGD("format: %s[%s]\n", piFmt->name, piFmt->long_name);
				break;
			}
			
		}while(iloop-- >=0);

/*
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
	//	fLOGD(fpLog,"%2d:%2d:%3d 探测码流成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);
		//gettimeofday(&tm,NULL);
		//LOGD("-----init time 2 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
*/
		pFmt = avformat_alloc_context();
		pFmt->pb = pb;
		if (avformat_open_input(&pFmt, "", piFmt, NULL) < 0) {
			//LOGD("avformat open failed.\n");
			/*
			memset(&systm,0,sizeof(systm));
			GetLocalTime(&systm);
			fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据失败\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);*/
			return -1;
		} else {/*
			memset(&systm,0,sizeof(systm));
			GetLocalTime(&systm);
			fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据成功\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);*/
		}

		
		//gettimeofday(&tm,NULL);
		//LOGD("-----init time 3 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
		
	}


	//LOGD("======max_analyze_duration=%d,probesize=%ld==== \n",pFmt->max_analyze_duration,pFmt->probesize);

	if(m_ifileType == internetfile)
	{
		pFmt->max_analyze_duration  = 800;
		pFmt->probesize = BUF_SIZE;//2048;

	//	pFmt->max_analyze_duration2  = 30*1000000;
	//	pFmt->probesize2 = 50000000;
		pFmt->max_analyze_duration  = 30*1000000;
		pFmt->probesize =  50000000;
	}

/*
	if (av_find_stream_info(pFmt) < 0)
	{
		fLOGD(stderr, "could not fine stream.\n");
		return -1;
	}*/
	if (avformat_find_stream_info(pFmt,NULL) < 0)
	{/*
		memset(&systm,0,sizeof(systm));
		GetLocalTime(&systm);
		fLOGD(fpLog,"%2d:%2d:%3d 打开网络数据信息\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
		fflush(fpLog);*/
		return -1;
	}

	
	//gettimeofday(&tm,NULL);
	//LOGD("-----init time 4 =%ld\n",tm.tv_sec*1000+tm.tv_usec/1000);
	//pFFAVFormat->ff_av_dump_format(pFmt,0,"",0);
	



	av_dump_format(pFmt, 0, "", 0);

	int videoindex = -1;
	int audioindex = -1;
	for (int i = 0; i < pFmt->nb_streams; i++) 
	{
		if ( (pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) &&
				(videoindex < 0) ) {
			videoindex = i;
		}
		if ( (pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) &&
				(audioindex < 0) ) {
			audioindex = i;
		}
	}


	if (videoindex < 0 || audioindex < 0) {
		LOGD("videoindex=%d, audioindex=%d\n", videoindex, audioindex);
		return -1;
	}

	AVStream *pVst,*pAst;
	pVst = pFmt->streams[videoindex];
	pAst = pFmt->streams[audioindex];

	
	pVideoCodecCtx = pVst->codec;
	pAudioCodecCtx = pAst->codec;

	pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
	if (!pVideoCodec) {
		LOGD("could not find video decoder!\n");
		return -1;
	}
	if (avcodec_open2(pVideoCodecCtx, pVideoCodec,NULL) < 0) {
		LOGD("could not open video codec!\n");
		return -1;
	}


	pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
	if (!pAudioCodec) {
		LOGD("could not find audio decoder!\n");
		return -1;
	}
	if (avcodec_open2(pAudioCodecCtx, pAudioCodec,NULL) < 0) {
		LOGD("could not open audio codec!\n");
		return -1;
	}
	
#if 0
	memset(&systm,0,sizeof(systm));
	GetLocalTime(&systm);
	// media info
	char txt[1024]={0};
	switch(pVideoCodecCtx->codec_id)
	{
		case CODEC_ID_H264:
			{
				memset(txt,0,sizeof(txt));
				sLOGD(txt,"Video Format H264\n");
				fLOGD(fpLog,"%2d:%2d:%3d 视频类型：H264\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
				fflush(fpLog);
				break;
			}
		case CODEC_ID_MPEG2VIDEO:
			{
				memset(txt,0,sizeof(txt));
				sLOGD(txt,"Video Format MPEG2VIDEO\n");
				fLOGD(fpLog,"%2d:%2d:%3d 视频类型：MPEG2VIDEO\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
				fflush(fpLog);
				break;
			}
		case CODEC_ID_MPEG4:
			{
				memset(txt,0,sizeof(txt));
				sLOGD(txt,"Video Format MPEG4\n");
				fLOGD(fpLog,"%2d:%2d:%3d 视频类型：MPEG4\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
				fflush(fpLog);
				break;
			}
		case CODEC_ID_H263:
			{
				memset(txt,0,sizeof(txt));
				sLOGD(txt,"Video Format H263\n");
				fLOGD(fpLog,"%2d:%2d:%3d 视频类型：H263\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
				fflush(fpLog);
				break;
			}
		default:
			{
				memset(txt,0,sizeof(txt));
				sLOGD(txt,"Video Format %d\n",pVideoCodecCtx->codec_id);
				fLOGD(fpLog,"%2d:%2d:%3d 视频类型：%d\n",systm.wMinute,systm.wSecond,systm.wMilliseconds,pVideoCodecCtx->codec_id);
				fflush(fpLog);
			}
			break;
	}
	//fwrite(txt,1,strlen(txt),m_MediaInfofp);
	//fflush(m_MediaInfofp);

	switch(pAudioCodecCtx->codec_id)
	{
	case CODEC_ID_MP2:
		{
			memset(txt,0,sizeof(txt));
			sLOGD(txt,"Audio Format MP2\n");
			fLOGD(fpLog,"%2d:%2d:%3d 音频类型：MP2\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);
			break;
		}
	case CODEC_ID_MP3:
		{
			memset(txt,0,sizeof(txt));
			sLOGD(txt,"Audio Format MP3\n");
			fLOGD(fpLog,"%2d:%2d:%3d 音频类型：MP3\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);
			break;
		}
	case CODEC_ID_AAC:
		{
			memset(txt,0,sizeof(txt));
			sLOGD(txt,"Audio Format AAC\n");
			fLOGD(fpLog,"%2d:%2d:%3d 音频类型：AAC\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);
			break;
		}
	case CODEC_ID_AC3:
		{
			memset(txt,0,sizeof(txt));
			sLOGD(txt,"Audio Format AC3\n");
			fLOGD(fpLog,"%2d:%2d:%3d 音频类型：AC3\n",systm.wMinute,systm.wSecond,systm.wMilliseconds);
			fflush(fpLog);
			break;
		}
	default:
		{
			memset(txt,0,sizeof(txt));
			sLOGD(txt,"Audio Format %d\n",pAudioCodecCtx->codec_id);
			fLOGD(fpLog,"%2d:%2d:%3d 音频类型：%d\n",systm.wMinute,systm.wSecond,systm.wMilliseconds,pAudioCodecCtx->codec_id);
			fflush(fpLog);
		}
		break;
	}
	//fwrite(txt,1,strlen(txt),m_MediaInfofp);
	//fflush(m_MediaInfofp);

#endif


	m_pVideoCodec = pVideoCodec;
	m_pAudioCodec = pAudioCodec;
	m_pFmt = pFmt;
	m_pVideoCodecCtx = pVideoCodecCtx;
	m_pAudioCodecCtx = pAudioCodecCtx;
	m_pframe = NULL;
	//m_iHeight = iHeight;
	//m_iWidht = iWidht;
	m_videoindex = videoindex;
	m_audioindex = audioindex;

	
	pthread_mutex_init(&m_locker,NULL);
	pthread_mutex_init(&m_audioLocker,NULL);

	
	//initQueue(Queue_size);

	return 0;
}

bool TSDecoder_Instance::Get_tsDecoder_sem(void **pSem)
{
	//*pSem = &(m_recvqueue.m_sem_send);
	LOGD("=====LOGD add \n");
//	LOGD("----sem add %0x",*pSem);
	return true;
}



int TSDecoder_Instance::uninit_TS_Decoder()
{
	
	//pthread_cancel(p_instanse->read_thread_id);
	//pthread_cancel(p_instanse->write_thread_id);
	//pthread_mutex_destroy(&p_instanse->m_mutex);

	freeQueue();
	
	if (m_pVideoCodecCtx) 
	{
        avcodec_close(m_pVideoCodecCtx);
    }
	if(m_pAudioCodecCtx)
	{
		avcodec_close(m_pAudioCodecCtx);
	}
	//if(p_instanse->m_pFmt)
	//{
	//	avcodec_close(p_instanse->m_pFmt);
	//}
		if (m_img_convert_ctx)  
    {  
        sws_freeContext(m_img_convert_ctx);  
        m_img_convert_ctx = NULL;  
    }  
	av_free(m_pframe);

    /* free the stream */
    av_free(m_pFmt);

//	if(m_avbuf)
//		pFFCodec->ff_av_freep(m_avbuf);
//	m_avbuf = NULL;
	return 0;
}

bool TSDecoder_Instance::Clean_Video_audioQue()
{
	pthread_mutex_lock(&m_locker);
	//m_recvqueue.m_bDelayFrame = false;
	//LOGD("-----clean video que begin\n");
	while(m_usedQueue.size() >0)
	{
		VideoData* tempbuff = m_usedQueue.front();
		tempbuff->iWidth = 0;
		tempbuff->iHeight = 0;
		m_usedQueue.pop_front();
		m_freeQueue.push_back(tempbuff);

	}
	pthread_mutex_unlock(&m_locker);
	//LOGD("----clean audio que \n");

	pthread_mutex_lock(&m_audioLocker);
	while(m_audioUsedQueue.size() > 0)
	{
		AudioData* audiobuff = m_audioUsedQueue.front();
		m_audioUsedQueue.pop_front();
		m_audioFreeQueue.push_back(audiobuff);
	}
	pthread_mutex_unlock(&m_audioLocker);

	return true;
}

bool TSDecoder_Instance::Set_tsDecoder_Volume(int iVolume)
{
/*
	double fbtmp = (double)iVolume/100;
	DWORD nVolume = fbtmp * 0xFFFF;
	if(!m_pAudioPlay)
		return false;
	return m_pAudioPlay->SetVolume(nVolume);
	*/
	return true;
}

bool TSDecoder_Instance::Get_tsDecoder_Volume(int &iVolume)
{
/*
	if(!m_pAudioPlay)
		return false;

	DWORD nVolume = 0;
	int ret = true;
	if(m_pAudioPlay)
		m_pAudioPlay->GetVolume(nVolume);
	double fbtmp = (double)nVolume /0xFFFF;
	 iVolume = fbtmp *100;
	 */
	return true;
}

//设置码率周期
bool TSDecoder_Instance::Set_tsRate_period(int iperiod)
{

	m_recvqueue.Set_tsRate_period(iperiod);
	return true;
}

//获取到码率
bool TSDecoder_Instance::Get_tsRate(int* iRate)
{
	m_recvqueue.Get_tsRate(iRate);
	return true;
}
//计算延时
bool TSDecoder_Instance::Set_tsTime_delay(int begintime,int* relsutTime)
{

	return true;
}

bool TSDecoder_Instance::Get_tsIFrame_size(int* iSize)
{
	//m_tsStreamPrase.Get_tsIFrame_size(iSize);
//	m_recvqueue.Get_tsIFrame_size(iSize);
	return true;
}

bool TSDecoder_Instance::Set_tsDecoder_SaveStream(bool bSaveStream)
{
	//m_tsStreamPrase.Set_tsDecoder_SaveStream(bSaveStream);
	m_bSaveTSStream = bSaveStream;
	return true;
}

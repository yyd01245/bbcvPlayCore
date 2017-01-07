#include"avfilterXiHa.h"
#include"Decoder.h"

int avFilter_XH::init_filters(AVCodecContext *pCodecCtx, const char *filters_descr)  
{  
	avfilter_register_all();
    char args[512];  
    int ret;  
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");   //
    AVFilter *buffersink = avfilter_get_by_name("buffersink");    // ("ffbuffersink");  ffmpeg3.0使用buffersink
    AVFilterInOut *outputs = avfilter_inout_alloc();  
    AVFilterInOut *inputs  = avfilter_inout_alloc();  
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };  
    AVBufferSinkParams *buffersink_params;  
  
    filter_graph = avfilter_graph_alloc();  
  
    /* buffer video source: the decoded frames from the decoder will be inserted here. */  
    snprintf(args, sizeof(args),  
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",  
            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,  
            pCodecCtx->time_base.num, pCodecCtx->time_base.den,  
            pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);  
 	
	LOGE("avfilter_XH : create filter args: %s \n",args); 
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",  
                                       args, NULL, filter_graph);  
    if (ret < 0) {  
        LOGE("Cannot create buffer source\n");  
        return ret;  
    }  
 	LOGE("avfilter_XH : create filter success \n"); 
    /* buffer video sink: to terminate the filter chain. */  
    buffersink_params = av_buffersink_params_alloc();  
    buffersink_params->pixel_fmts = pix_fmts;  
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",  
                                       NULL, buffersink_params, filter_graph);  
	
 	LOGD("avfilter_XH : create sink filter success \n"); 
    av_free(buffersink_params);  
    if (ret < 0) {  
        printf("Cannot create buffer sink error num:%d \n",AVERROR(ENOMEM));  
        return ret;  
    }  
  
    /* Endpoints for the filter graph. */  
    outputs->name       = av_strdup("in");  
    outputs->filter_ctx = buffersrc_ctx;  
    outputs->pad_idx    = 0;  
    outputs->next       = NULL;  
  
    inputs->name       = av_strdup("out");  
    inputs->filter_ctx = buffersink_ctx;  
    inputs->pad_idx    = 0;  
    inputs->next       = NULL;  
  
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,  
                                    &inputs, &outputs, NULL)) < 0)  
        return ret;  
  
	
 	LOGD("avfilter_XH : parse filter graph success \n"); 
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)  
        return ret; 

	filt_frame = av_frame_alloc();
	src_frame = av_frame_alloc(); 
	if(src_frame ==  NULL)
		{
			printf("init src_frame NULL\n");
			
		}
		if(filt_frame ==  NULL)
		{
			printf("init filter_frame NULL\n");
			
		}
		src_frame->format = AV_PIX_FMT_YUV420P;
		src_frame->width  = pCodecCtx->width; 
		src_frame->height = pCodecCtx->height;
		if(av_frame_get_buffer(src_frame, 32) != 0)
			printf("Can't allocate a buffer for input frame\n");

 	LOGE("avfilter_XH : init filter success over \n"); 
		
		//memset(src_frame,0,sizeof(AVFrame));
		//memset(filt_frame,0,sizeof(AVFrame));
	fprintf(stderr,"init filter success \n");
    return 0;  
} 





int avFilter_XH::filter_run(int iWidth, int iHight,AVFrame * pframe,unsigned long ulpts)
{
	if(NULL == filter_graph ){
		// no filter
		((TSDecoder_Instance*)m_funpush)->showVideoData(pframe,ulpts);
		return 0;
	}
	//((TSDecoder_Instance*)m_funpush)->Push_Video_Data_shm(iWidth,iHight,(AVPicture*)pframe,ulpts);
#if 1
	 pframe->pts = ulpts; //av_frame_get_best_effort_timestamp(pframe);  
     /* push the decoded frame into the filtergraph */
	 pframe->channel_layout = 0;
	 int ret = -1;
	ret = av_buffersrc_add_frame_flags(buffersrc_ctx, pframe, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		if(AVERROR(EINVAL)==ret)
			 av_log(NULL, AV_LOG_ERROR, "Error Number EINVAL while feeding the filtergraph\n");
		else if(AVERROR(ENOMEM)==ret)
			av_log(NULL, AV_LOG_ERROR, "Error Number ENOMEM while feeding the filtergraph\n");
		return -1;
    }
	/* pull filtered frames from the filtergraph */

	while (1) {
		ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		if (ret < 0)
			goto end;
		//display_frame(filt_frame, buffersink_ctx->inputs[0]->time_base);
		((TSDecoder_Instance*)m_funpush)->showVideoData(filt_frame,ulpts);
		av_frame_unref(filt_frame);
	}
	//av_frame_unref(pframe);
 	return 0;
 end:  
	 avfilter_graph_free(&filter_graph);  
   
	 filter_graph = NULL;
	 if (ret < 0 && ret != AVERROR_EOF) {  
		 char buf[1024];  
		 av_strerror(ret, buf, sizeof(buf));  
		 printf("Error occurred: %s\n", buf);  
		 return -1;  
	 }	
#endif

	 return 0;	
}

int avFilter_XH::set_push_func(void* pfunc)
{
	
	m_funpush = pfunc;
	return  0;
}



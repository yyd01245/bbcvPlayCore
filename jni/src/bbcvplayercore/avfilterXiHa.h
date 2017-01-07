#ifndef _AVFILTER_XIHA_H_
#define _AVFILTER_XIHA_H_

#include <deque>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>

#ifndef INT64_C
#define INT64_C(c) c##LL
#endif
#ifndef UINT64_C
#define UINT64_C(c) c##LL
#endif
extern "C" {

#include "libavutil/pixfmt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/samplefmt.h"

#include "libavutil/avstring.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"

#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/avfilter.h"

}

#include "DataHead.h"

#include<vector>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>

#include<stdio.h>
#include<deque>

//const char *filter_descr = "movie=my_logo.png[wm];[in][wm]overlay=5:5[out]";  


//typedef  void (*Push_Video_Data_shm_func)(int iWidth, int iHight, AVPicture *pAVfram,unsigned long ulPTS)

class avFilter_XH
{
public:
	
	int init_filters(AVCodecContext *pCodecCtx, const char *filters_descr);

	int set_push_func(void* pfunc);
	int filter_run(int iWidth, int iHight,AVFrame *pframe,unsigned long ulpts);

	void* m_funpush;
//private:
	AVFilterContext *buffersink_ctx;  
	AVFilterContext *buffersrc_ctx;  
	AVFilterGraph *filter_graph;  
	AVFrame *filt_frame;
	AVFrame* src_frame ; 

};




#endif

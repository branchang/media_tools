
#include "pen_word_into_pic.h"

#define MAX_SUBTITLE_PER_LEN  16
#define MAX_WORDS_LEN_PER_LINE  10
#define MAX_WORDS_LINE    5


CPenWordIntoPic::CPenWordIntoPic()
{
	m_filter_graph = NULL;
	m_codecContext = NULL;
	m_picture_buf_Src2YUV = NULL;
	m_srcFrame = NULL;
	m_ConverImg = NULL;
	avfilter_register_all();
}

CPenWordIntoPic::~CPenWordIntoPic()
{
	if (m_srcFrame)
	{
		av_frame_free(&m_srcFrame);
	}

	if (m_picture_buf_Src2YUV)
	{
		delete[] m_picture_buf_Src2YUV;
	}

	if (m_ConverImg)
	{
		delete m_ConverImg;
	}
}

bool CPenWordIntoPic::SetSubTitile(const char* subTitile, AVCodecContext * codecContext)
{
	m_codecContext = codecContext;
	if (!codecContext)
	{
		return false;
	}

	m_ConverImg = new CConvertImg(codecContext->width,
									codecContext->height,
									codecContext->width,
									codecContext->height,
									codecContext->pix_fmt,
									codecContext->pix_fmt);

	std::string tmpStr = subTitile;

	char tmpChar[128] = {0};
	sprintf(tmpChar, "fontsize=%d:x=0:y=0:text=", 100);
	std::string strFontAndPos = tmpChar;


	m_filters_descr ="drawtext=fontfile=/Users/Bran/Downloads/FreeSerif/FreeSerif.ttf:fontcolor=red:" + strFontAndPos + tmpStr;

	if(0 != InitFilter(codecContext))
		return false;

	return true;
}

AVFrame * CPenWordIntoPic::GetAFrameWithWord(AVFrame *src)
{
	int ret = -1;

	AVFrame *tmpFrame = src;
	if (NULL == src)
	{
		if (NULL == m_srcFrame)
		{
			int frame_size = avpicture_get_size(m_codecContext->pix_fmt, m_codecContext->width, m_codecContext->height);
			m_picture_buf_Src2YUV = new uint8_t[frame_size];
			m_srcFrame = av_frame_alloc();
			avpicture_fill((AVPicture *)m_srcFrame, m_picture_buf_Src2YUV, m_codecContext->pix_fmt,
				m_codecContext->width, m_codecContext->height);
			m_srcFrame->format = m_codecContext->pix_fmt;
			m_srcFrame->width = m_codecContext->width;
			m_srcFrame->height = m_codecContext->height;

			memset(m_srcFrame->data[0], 38.334, m_codecContext->width * m_codecContext->height);
			memset(m_srcFrame->data[1], 128, m_codecContext->width * m_codecContext->height / 4);
			memset(m_srcFrame->data[2], 128, m_codecContext->width * m_codecContext->height / 4);
			tmpFrame = m_srcFrame;
		}
	}

	ret = av_buffersrc_add_frame(m_buffersrc_ctx, tmpFrame);

	AVFrame *_pFrameOut;
	_pFrameOut = av_frame_alloc();
	_pFrameOut->width = m_codecContext->width;
	_pFrameOut->format = m_codecContext->pix_fmt;
	_pFrameOut->height = m_codecContext->height;
	ret = av_buffersink_get_frame_flags(m_buffersink_ctx, _pFrameOut, 0);

	AVFrame * tmpFrame0 = _pFrameOut;
	if (m_ConverImg)
	{
		tmpFrame0 = m_ConverImg->GetAConvertFrameData(_pFrameOut);
		av_frame_free(&_pFrameOut);
	}

	return tmpFrame0;
}

int CPenWordIntoPic::InitFilter(AVCodecContext * codecContext)
{
	char args[512];
	int ret = 0;

	const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P};

	m_filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !m_filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	sprintf(args,
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		codecContext->width, codecContext->height, codecContext->pix_fmt,
		codecContext->time_base.num, codecContext->time_base.den,
		codecContext->sample_aspect_ratio.num, codecContext->sample_aspect_ratio.den);

	ret = avfilter_graph_create_filter(&m_buffersrc_ctx, buffersrc, "in",
		args, NULL, m_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&m_buffersink_ctx, buffersink, "out",
		NULL, NULL, m_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	ret = av_opt_set_int_list(m_buffersink_ctx, "pix_fmts", pix_fmts,
		AV_PIX_FMT_YUV420P, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = m_buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name       = av_strdup("out");
	inputs->filter_ctx = m_buffersink_ctx;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;
	if ((ret = avfilter_graph_parse_ptr(m_filter_graph, m_filters_descr.c_str(),
		&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(m_filter_graph, NULL)) < 0)
		goto end;
	return ret;
end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}


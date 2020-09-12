#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
};
#endif
#include <string>
#include "convert_img.h"

class CPenWordIntoPic{

    public:
        CPenWordIntoPic();
        ~CPenWordIntoPic();
        bool SetSubTitile(const char* sub_title, AVCodecContext* codecContext);

        AVFrame *GetAFrameWithWord(AVFrame *src = nullptr);

    protected:
        int InitFilter(AVCodecContext * codecContext);

        AVFilterGraph* m_filter_graph;

        AVFilterContext* m_buffersrc_ctx;
        AVFilterContext* m_buffersink_ctx;

        AVCodecContext *m_codecContext;

        AVFrame *m_srcFrame;
        uint8_t* m_picture_buf_Src2YUV;

        CConvertImg *m_ConverImg;
        std::string m_filters_descr;
};

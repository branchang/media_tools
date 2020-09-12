#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif


class CConvertImg
{


public:
    CConvertImg(int w0, int h0, int w1, int h1, AVPixelFormat srcFmt,
                AVPixelFormat desFmt, int rotate=0, bool bKeepRatio=true);
    ~CConvertImg();

    AVFrame* GetAConvertFrameData(AVFrame* src, bool bWrite=false);

    void GetSrcSize(int &w, int &h);
    void GetDesSize(int &w, int &h);
    AVPixelFormat GetSrcFmt(){
        return m_srcFmt;
    }

    AVPixelFormat GetDesFmt(){
        return m_desFmt;
    }

protected:

    SwsContext *m_img_convert_ctx_src_YUV;
    AVFrame *m_Src2YUV;
    uint8_t *m_picture_buf_Src2YUV;

	AVFrame *m_FrameConverRotate;
	uint8_t *m_picture_buf_Rotate;

	SwsContext *m_img_convert_ctx_scale_pixel_YUV;
	AVFrame *m_Frame_scale_YUV;
	uint8_t *m_picture_scale_yuv_buf;

	AVFrame *m_FrameAddBlackSide_yuv;
	uint8_t *m_picture_buf_AddBlackSide_yuv;

	SwsContext *m_img_convert_ctx_out;
	AVFrame *m_FrameOut;
	uint8_t *m_picture_buf_out;

	AVPixelFormat m_srcFmt;
	AVPixelFormat m_desFmt;
    int m_srcW;
    int m_srcH;
	int m_desW;
	int m_desH;

	int m_desRatioW;
	int m_desRatioH;

	int m_rotate;

};


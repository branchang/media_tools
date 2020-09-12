#include "convert_img.h"



CConvertImg::CConvertImg(int w0, int h0, int w1, int h1, AVPixelFormat srcFmt, AVPixelFormat desFmt, int rotate, bool bKeepRatio)
{
    if (!(rotate == 0 || rotate == 90 || rotate == 180 || rotate == 270))
	{
		rotate = 0;
	}

	m_rotate = rotate;

	m_img_convert_ctx_src_YUV = NULL;
	m_Src2YUV = NULL;
	m_picture_buf_Src2YUV = NULL;

	m_FrameConverRotate = NULL;
	m_picture_buf_Rotate = NULL;

	m_img_convert_ctx_scale_pixel_YUV = NULL;
	m_Frame_scale_YUV = NULL;
	m_picture_scale_yuv_buf = NULL;

	m_FrameAddBlackSide_yuv = NULL;
	m_picture_buf_AddBlackSide_yuv = NULL;

	m_img_convert_ctx_out = NULL;
	m_FrameOut = NULL;
	m_picture_buf_out = NULL;

	m_srcW = w0;
	m_srcH = h0;
	m_desW = w1;
	m_desH = h1;
	m_srcFmt = srcFmt;
	m_desFmt = desFmt;

	m_desRatioW = m_desW;
	m_desRatioH = m_desH;
    //求得保持比例的长宽
	//if (bKeepRatio)
	{
		if (0 == rotate || 180 == rotate)
		{
			float srcRatio = ((float)m_srcW) / ((float)m_srcH);
			float desRatio = ((float)m_desW) / ((float)m_desH);

			if (!(abs(srcRatio - desRatio) < 0.01))
			{
				if (srcRatio > desRatio)
				{
					m_desRatioH = m_desW / srcRatio;
				}
				else
				{
					m_desRatioW = m_desH * srcRatio;
				}
			}

			m_img_convert_ctx_scale_pixel_YUV = sws_getContext(m_srcW, m_srcH, AV_PIX_FMT_YUV420P, m_desRatioW, m_desRatioH, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		}
		else if (90 == rotate || 270 == rotate)
		{
			float srcRatio = ((float)m_srcH) / ((float)m_srcW);
			float desRatio = ((float)m_desW) / ((float)m_desH);

			if (!(abs(srcRatio - desRatio) < 0.01))
			{
				if (srcRatio > desRatio)
				{
					m_desRatioH = m_desW / srcRatio;
				}
				else
				{
					m_desRatioW = m_desH * srcRatio;
				}
			}

			m_img_convert_ctx_scale_pixel_YUV = sws_getContext(m_srcH, m_srcW, AV_PIX_FMT_YUV420P, m_desRatioW, m_desRatioH, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		}
	}

    {
		if (1)
		{
			m_img_convert_ctx_src_YUV = sws_getContext(m_srcW, m_srcH, srcFmt, m_srcW, m_srcH, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

			m_Src2YUV = av_frame_alloc();
			// int tmpSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_srcW, m_srcH);
			int tmpSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_srcW, m_srcH, 1);
			m_picture_buf_Src2YUV = new uint8_t[tmpSize];
			// avpicture_fill((AVPicture *)m_Src2YUV, m_picture_buf_Src2YUV, AV_PIX_FMT_YUV420P, m_srcW, m_srcH);
			av_image_fill_arrays(((AVPicture *)m_Src2YUV)->data, ((AVPicture *)m_Src2YUV)->linesize, m_picture_buf_Src2YUV, AV_PIX_FMT_YUV420P, m_srcW, m_srcH, 1);

			m_Src2YUV->format = AV_PIX_FMT_YUV420P;
			m_Src2YUV->width = m_srcW;
			m_Src2YUV->height = m_srcH;
		}

		m_Frame_scale_YUV = av_frame_alloc();
		int size = avpicture_get_size(AV_PIX_FMT_YUV420P, m_desRatioW, m_desRatioH);
		m_picture_scale_yuv_buf = new uint8_t[size];
		avpicture_fill((AVPicture *)m_Frame_scale_YUV, m_picture_scale_yuv_buf, AV_PIX_FMT_YUV420P, m_desRatioW, m_desRatioH);
		m_Frame_scale_YUV->format = AV_PIX_FMT_YUV420P;
		m_Frame_scale_YUV->width = m_desRatioW;
		m_Frame_scale_YUV->height = m_desRatioH;

		if (90 == rotate || 270 == rotate)
		{
			m_FrameConverRotate = av_frame_alloc();
			int tmpSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_srcH, m_srcW);
			m_picture_buf_Rotate = new uint8_t[tmpSize];
			avpicture_fill((AVPicture *)m_FrameConverRotate, m_picture_buf_Rotate, AV_PIX_FMT_YUV420P, m_srcH, m_srcW);
			m_FrameConverRotate->format = AV_PIX_FMT_YUV420P;
			m_FrameConverRotate->width = m_srcH;
			m_FrameConverRotate->height = m_srcW;
		}

		m_FrameAddBlackSide_yuv = av_frame_alloc();
		size = avpicture_get_size(AV_PIX_FMT_YUV420P, m_desW, m_desH);
		m_picture_buf_AddBlackSide_yuv = new uint8_t[size];
		avpicture_fill((AVPicture *)m_FrameAddBlackSide_yuv, m_picture_buf_AddBlackSide_yuv, AV_PIX_FMT_YUV420P, m_desW, m_desH);
		m_FrameAddBlackSide_yuv->format = AV_PIX_FMT_YUV420P;
		m_FrameAddBlackSide_yuv->width = m_desW;
		m_FrameAddBlackSide_yuv->height = m_desH;


		m_img_convert_ctx_out = sws_getContext(m_desW, m_desH, AV_PIX_FMT_YUV420P, m_desW, m_desH, m_desFmt, SWS_BICUBIC, NULL, NULL, NULL);
		m_FrameOut = av_frame_alloc();
		size = avpicture_get_size(m_desFmt, m_desW, m_desH);
		m_picture_buf_out = new uint8_t[size];
		avpicture_fill((AVPicture *)m_FrameOut, m_picture_buf_out, desFmt, m_desW, m_desH);
		m_FrameOut->format = desFmt;
		m_FrameOut->width = m_desW;
		m_FrameOut->height = m_desH;
	}
}

CConvertImg::~CConvertImg()
{

    if (m_img_convert_ctx_src_YUV)
	{
		sws_freeContext(m_img_convert_ctx_src_YUV);
		m_img_convert_ctx_src_YUV = NULL;
	}

	if (m_Src2YUV)
	{
		av_frame_free(&m_Src2YUV);
		m_Src2YUV = NULL;
	}

	if (m_picture_buf_Src2YUV)
	{
		delete[] m_picture_buf_Src2YUV;
		m_picture_buf_Src2YUV = NULL;
	}
	/////////////////////

	if (m_FrameConverRotate)
	{
		av_frame_free(&m_FrameConverRotate);
		m_FrameConverRotate = NULL;
	}

	if (m_picture_buf_Rotate)
	{
		delete[] m_picture_buf_Rotate;
		m_picture_buf_Rotate = NULL;
	}
	///////////////////////////////

	if (m_img_convert_ctx_scale_pixel_YUV)
	{
		sws_freeContext(m_img_convert_ctx_scale_pixel_YUV);
		m_img_convert_ctx_scale_pixel_YUV = NULL;
	}

	if (m_Frame_scale_YUV)
	{
		av_frame_free(&m_Frame_scale_YUV);
		m_Frame_scale_YUV = NULL;
	}

	if (m_picture_scale_yuv_buf)
	{
		delete[] m_picture_scale_yuv_buf;
		m_picture_scale_yuv_buf = NULL;
	}
	/////////////////////////////////

	if (m_FrameAddBlackSide_yuv)
	{
		av_frame_free(&m_FrameAddBlackSide_yuv);
		m_FrameAddBlackSide_yuv = NULL;
	}
	if (m_picture_buf_AddBlackSide_yuv)
	{
		delete[] m_picture_buf_AddBlackSide_yuv;
		m_picture_buf_AddBlackSide_yuv = NULL;
	}
	/////////////////////////////////////////////

	if (m_img_convert_ctx_out)
	{
		sws_freeContext(m_img_convert_ctx_out);
		m_img_convert_ctx_out = NULL;
	}

	if (m_FrameOut)
	{
		av_frame_free(&m_FrameOut);
		m_FrameOut = NULL;
	}

	if (m_picture_buf_out)
	{
		delete[] m_picture_buf_out;
		m_picture_buf_out = NULL;
	}
}



AVFrame* CConvertImg::GetAConvertFrameData(AVFrame* src, bool bWrite)
{
	// 	Y = 0.299 R + 0.587 G + 0.114 B
	// 	U = - 0.1687 R - 0.3313 G + 0.5 B + 128
	// 	V = 0.5 R - 0.4187 G - 0.0813 B + 128
	//RGB(0.0, 0.0, 0.0) -> YUV(0.0, 128, 128)

	//RGB(272727)
	float bY = 49.501;   //38.334  (1a1a1a)
	float bU = 128;
	float bV = 128;

	AVFrame* tmpFrameSrcYUV = src;
	//src 2 yuv
	if (m_img_convert_ctx_src_YUV)
	{
		sws_scale(m_img_convert_ctx_src_YUV, (const uint8_t* const*)src->data, src->linesize, 0,
			/*m_desH*/m_srcH, m_Src2YUV->data, m_Src2YUV->linesize);
		tmpFrameSrcYUV = m_Src2YUV;
	}

	AVFrame* tmpRotateFrame = tmpFrameSrcYUV;
	//yuv rotate
	if (90 == m_rotate)
	{
		for (int i = 0; i < m_srcW; i++)
		{
			for (int j = 0; j < m_srcH; j++)
			{
				m_FrameConverRotate->data[0][i * m_FrameConverRotate->linesize[0] + j] =
					tmpFrameSrcYUV->data[0][(m_srcH - j - 1) * tmpFrameSrcYUV->linesize[0] + i];
			}
		}

		for (int i = 0; i < m_srcW / 2; i++)
		{
			for (int j = 0; j < m_srcH / 2; j++)
			{
				m_FrameConverRotate->data[1][i * m_FrameConverRotate->linesize[1] + j] =
					tmpFrameSrcYUV->data[1][(m_srcH / 2 - j - 1) * tmpFrameSrcYUV->linesize[1] + i];

				m_FrameConverRotate->data[2][i * m_FrameConverRotate->linesize[2] + j] =
					tmpFrameSrcYUV->data[2][(m_srcH / 2 - j - 1) * tmpFrameSrcYUV->linesize[2] + i];
			}
		}
		tmpRotateFrame = m_FrameConverRotate;
	}

	if (180 == m_rotate)
	{
		tmpRotateFrame = m_FrameConverRotate;
	}

	if (270 == m_rotate)
	{
		for (int i = 0; i < m_srcW; i++)
		{
			for (int j = 0; j < m_srcH; j++)
			{
				m_FrameConverRotate->data[0][i * m_FrameConverRotate->linesize[0] + j] =
					tmpFrameSrcYUV->data[0][j * tmpFrameSrcYUV->linesize[0] + (tmpFrameSrcYUV->linesize[0] - i - 1)];
			}
		}

		for (int i = 0; i < m_srcW / 2; i++)
		{
			for (int j = 0; j < m_srcH / 2; j++)
			{
				m_FrameConverRotate->data[1][i * m_FrameConverRotate->linesize[1] + j] =
					tmpFrameSrcYUV->data[1][j * tmpFrameSrcYUV->linesize[1] + (tmpFrameSrcYUV->linesize[1] - i - 1)];

				m_FrameConverRotate->data[2][i * m_FrameConverRotate->linesize[2] + j] =
					tmpFrameSrcYUV->data[2][j * tmpFrameSrcYUV->linesize[2] + (tmpFrameSrcYUV->linesize[2] - i - 1)];
			}
		}
		tmpRotateFrame = m_FrameConverRotate;
	}

	//rotate yuv 2 yuv dst scale pixel
	{
		if (90 == m_rotate || 270 == m_rotate)
		{
			sws_scale(m_img_convert_ctx_scale_pixel_YUV, (const uint8_t* const*)tmpRotateFrame->data, tmpRotateFrame->linesize, 0,
				/*m_desH*/m_srcW, m_Frame_scale_YUV->data, m_Frame_scale_YUV->linesize);
		}
		else
		{
			sws_scale(m_img_convert_ctx_scale_pixel_YUV, (const uint8_t* const*)tmpRotateFrame->data, tmpRotateFrame->linesize, 0,
				/*m_desH*/m_srcH, m_Frame_scale_YUV->data, m_Frame_scale_YUV->linesize);
		}
	}


	if ((m_desW == m_desRatioW && m_desH == m_desRatioH) && AV_PIX_FMT_YUV420P == m_desFmt)
	{
		return m_Frame_scale_YUV;
	}
	else//按比例把两边变黑边
	{
		//Y
		for (int i = 0; i < m_desH; i++)
		{
			if (m_desH != m_desRatioH)
			{
				int difH = m_desH - m_desRatioH;
				difH = difH / 2;
				if (i < difH || (m_desH - difH) < i)
				{
					memset(&m_FrameAddBlackSide_yuv->data[0][i * m_FrameAddBlackSide_yuv->linesize[0]], bY, m_Frame_scale_YUV->linesize[0]);
				}
				else
				{
					memcpy(&m_FrameAddBlackSide_yuv->data[0][i * m_FrameAddBlackSide_yuv->linesize[0]], &m_Frame_scale_YUV->data[0][(i - difH) * m_Frame_scale_YUV->linesize[0]], m_Frame_scale_YUV->linesize[0]);
				}
			}
			else
			{
				for (int j = 0; j < m_desW; j++)
				{
					int difW = m_desW - m_desRatioW;
					difW = difW / 2;
					if (j < difW || (m_desW - difW) < j)
					{
						m_FrameAddBlackSide_yuv->data[0][i * m_FrameAddBlackSide_yuv->linesize[0] + j] = bY;
					}
					else
					{
						m_FrameAddBlackSide_yuv->data[0][i * m_FrameAddBlackSide_yuv->linesize[0] + j] = m_Frame_scale_YUV->data[0][i * m_Frame_scale_YUV->linesize[0] + j - difW];
					}
				}
			}
		}

		//UV
		for (int i = 0; i < m_desH / 2; i++)
		{
			if (m_desH != m_desRatioH)
			{
				int difH = m_desH - m_desRatioH;
				difH = difH / 4;
				if (i < difH || (m_desH / 2 - difH) < i)
				{
					memset(&m_FrameAddBlackSide_yuv->data[1][i * m_FrameAddBlackSide_yuv->linesize[1]], bU, m_Frame_scale_YUV->linesize[1]);
					memset(&m_FrameAddBlackSide_yuv->data[2][i * m_FrameAddBlackSide_yuv->linesize[2]], bV, m_Frame_scale_YUV->linesize[2]);
				}
				else
				{
					memcpy(&m_FrameAddBlackSide_yuv->data[1][i * m_FrameAddBlackSide_yuv->linesize[1]], &m_Frame_scale_YUV->data[1][(i - difH) * m_Frame_scale_YUV->linesize[1]], m_Frame_scale_YUV->linesize[1]);
					memcpy(&m_FrameAddBlackSide_yuv->data[2][i * m_FrameAddBlackSide_yuv->linesize[2]], &m_Frame_scale_YUV->data[2][(i - difH) * m_Frame_scale_YUV->linesize[2]], m_Frame_scale_YUV->linesize[2]);
				}
			}
			else
			{
				for (int j = 0; j < m_desW / 2; j++)
				{
					int difW = m_desW - m_desRatioW;
					difW = difW / 4;
					if (j < difW || (m_desW / 2 - difW) < j)
					{
						m_FrameAddBlackSide_yuv->data[1][i * (m_FrameAddBlackSide_yuv->linesize[1]) + j] = bU;
						m_FrameAddBlackSide_yuv->data[2][i * (m_FrameAddBlackSide_yuv->linesize[2]) + j] = bV;
					}
					else
					{
						m_FrameAddBlackSide_yuv->data[1][i * (m_FrameAddBlackSide_yuv->linesize[1]) + j] = m_Frame_scale_YUV->data[1][i * (m_Frame_scale_YUV->linesize[1]) + j - difW];
						m_FrameAddBlackSide_yuv->data[2][i * (m_FrameAddBlackSide_yuv->linesize[2]) + j] = m_Frame_scale_YUV->data[2][i * (m_Frame_scale_YUV->linesize[2]) + j - difW];
					}
				}
			}
		}


		if (m_img_convert_ctx_out)
		{
			sws_scale(m_img_convert_ctx_out, (const uint8_t* const*)m_FrameAddBlackSide_yuv->data, m_FrameAddBlackSide_yuv->linesize, 0,
				/*m_desH*/m_desH, m_FrameOut->data, m_FrameOut->linesize);
		}

		return m_FrameOut;
	}
}

void CConvertImg::GetSrcSize(int &w, int &h)
{
	w = m_srcW;
	h = m_srcH;
}

void CConvertImg::GetDesSize(int &w, int &h)
{
	w = m_desW;
	h = m_desH;
}

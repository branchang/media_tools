
#include <unistd.h>
#include <pthread.h>
#include "pen_word_into_pic.h"
#include "convert_img.h"

#define ERRLOG printf


#define ERR_OK 0						//成功
#define ERR_OPEN_INPUT -1				//打开输入错误
#define ERR_NO_VIDEO_INPUT -2			//打开视频输入错误
#define ERR_NO_AUDIO_INPUT -3			//打开音频输入错误
#define ERR_OPEN_VIDEO_OUTPUT -4		//打开视频输出错误
#define ERR_OPEN_FILE_HANLDE -5			//打开输出文件错误
#define ERR_WRITE_HEAD -6				//输出文件写头错误
#define ERR_OCTX_ALLOC -7				//无法初始化输出格式错误
#define ERR_MARK_NOPIC -8				//水印图片错误
#define ERR_ADD_SUBTITLE -9             //添加字幕出错


void* DecodeWork(void* lpParam );
void CodecWork();

int flush_video_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index);
	void free_output_fmtCtx(AVFormatContext **fmtCtx);

int NewVideoStream(AVFormatContext *pOutContext, AVCodecContext *pVideoCodecContext, int &outVedioStramIndex, int w, int h, AVCodecID codecID, int frameRate, AVStream* inPutStream)
{
	int ret = 0;
	if (pVideoCodecContext && (!(w && h && 0 != codecID)))
	{
		outVedioStramIndex = pOutContext->nb_streams;

		AVStream* out_video_stream = avformat_new_stream(pOutContext, NULL);
		if (!out_video_stream)
		{
			ERRLOG("Fail to allocating output video stream!");
			return -1;
		}
		if ((ret = avcodec_copy_context(out_video_stream->codec, pVideoCodecContext)) < 0)
		{
			ERRLOG("can not copy the video codec context! ret=%d", ret);
			return ret;
		}

		if (inPutStream)
		{
			AVDictionaryEntry *tag = NULL;
			tag = av_dict_get(inPutStream->metadata, "rotate", tag, 0);

			if (tag != NULL)
			{
				av_dict_set(&out_video_stream->metadata, "rotate", tag->value, 0);
			}
		}

		AVRational time_base={1, frameRate};

		out_video_stream->time_base = time_base;

		out_video_stream->codec->codec_tag = 0;
		if(pOutContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			out_video_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}
	else if (w && h && 0 != codecID)
	{
		//to be add byMK
		AVCodecContext *videoCodecCtx;
		outVedioStramIndex = pOutContext->nb_streams;
		AVStream* pVideoStream = avformat_new_stream(pOutContext, NULL);
		if (!pVideoStream)
		{
			ERRLOG("Fail to allocating output video stream!");
			return -1;
		}

		AVDictionaryEntry *tag = NULL;
		if (inPutStream)
		{
			tag = av_dict_get(inPutStream->metadata, "rotate", tag, 0);

			if (tag != NULL)
			{
				//av_dict_set(&pVideoStream->metadata, "rotate", tag->value, 0);
				int tmpRotate = atoi(tag->value);
				if (tmpRotate == 90 || tmpRotate == 270)
				{
					w = w + h;
					h = w - h;
					w = w - h;
				}
			}
		}

		//set codec context param
		pVideoStream->codec->codec = avcodec_find_encoder(codecID);
		pVideoStream->codec->height = h;
		pVideoStream->codec->width = w;
		pVideoStream->codec->codec_id = codecID;

		pVideoStream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
		pVideoStream->codec->qmin = 30;
		pVideoStream->codec->qmax = 51;

		AVRational time_base={1, frameRate};
		pVideoStream->codec->time_base = time_base;
		pVideoStream->time_base = time_base;
		// take first format from list of supported formats
		AVRational sample_aspect_ratio={0, 1};
		pVideoStream->codec->sample_aspect_ratio = sample_aspect_ratio;
		pVideoStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;//pOutContext->streams[outVedioStramIndex]->codec->codec->pix_fmts[0];
		//pVideoStream->codec->bit_rate = (w * h);
		pVideoStream->codec->gop_size = frameRate;

		pVideoStream->metadata;

		//open encoder
		if (!pVideoStream->codec->codec)
		{
			ERRLOG("fail to find the encoder for video stream codedID:%d", codecID);
			return -1;
		}

		if (pOutContext->oformat->flags & AVFMT_GLOBALHEADER)
			pVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		AVDictionary *param = NULL;
		if (pVideoStream->codec->codec_id == AV_CODEC_ID_H264)
		{
			av_dict_set(&param, "preset", "slow", 0);
			av_dict_set(&param, "tune", "film", 0);
		}

		if ((ret = avcodec_open2(pVideoStream->codec, pVideoStream->codec->codec, &param)) < 0)
		{
			ERRLOG("fail to open the encoder for video stream, codecID:%d ret:%d", codecID, ret);
			return ret;
		}
		ret = 0;   //将ret的值重新置为0
	}
	else
	{
		return -1;
	}

	return ret;
}


int OpenInput(const char * file_name, AVFormatContext **in_fmtctx, int &videoIndex, int &audioIndex)
{
	int ret = -1;
	if ((ret = avformat_open_input(in_fmtctx, file_name, NULL, NULL)) < 0)
	{
		ERRLOG("can not open the input file:%s!", file_name);
		return ret;
	}
	if ((ret = avformat_find_stream_info((*in_fmtctx), NULL)) < 0)
	{
		ERRLOG("can not find the input stream for file:%s!", file_name);
		avformat_close_input(in_fmtctx);
		return ret;
	}
	AVFormatContext *pTempInputCtx = (*in_fmtctx);
	for (int i = 0; i < pTempInputCtx->nb_streams; i++)
	{
		if (pTempInputCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (-1 == videoIndex)
			{
				videoIndex = i;
			}

			ret = avcodec_open2(pTempInputCtx->streams[i]->codec, avcodec_find_decoder(pTempInputCtx->streams[i]->codec->codec_id), NULL);
			if(0 > ret)
			{
				ERRLOG("can not find or open video decoder for file:%s!", file_name);
				avformat_close_input(in_fmtctx);
				return ret;
			}
		}
		else if (pTempInputCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (-1 == audioIndex)
			{
				audioIndex = i;
			}

			ret = avcodec_open2(pTempInputCtx->streams[i]->codec, avcodec_find_decoder(pTempInputCtx->streams[i]->codec->codec_id), NULL);
			if(0 > ret)
			{
				ERRLOG("can not find or open audio decoder for file:%s!", file_name);
				avformat_close_input(in_fmtctx);
				return ret;
			}
		}
	}

	return 0;
}


// const char *VideoFileName = "SourceVideo.MOV";
// const char *VideoFileName = "SampleVideo_1280x720_20mb.mkv";
const char* outVideoFileName = "TestAddSubTitle.mp4";
// const char *SubtitleWords = "ADD SUBTITLE!!!";

AVFormatContext *pInputContext = NULL;

int inputVideoIndex = -1;
int inputAudioIndex = -1;


//output
AVFormatContext *pOutContext = NULL;

int outPutVideoIndex = -1;

AVFifoBuffer	*pFifoVideo;

CPenWordIntoPic *pPenWordIntoPic = NULL;

bool bIsFileEnd = false;
bool bWork = true;

int add_subtitle_main(const char *VideoFileName, const char *SubtitleWords)
{
	av_register_all();
	if ((NULL == VideoFileName || NULL == outVideoFileName))
	{
		return -1;
	}

	int ret_code = 0;
	int m_x = 0;
	int m_y = 0;
	int srcW = 0;
	int srcH = 0;



	OpenInput(VideoFileName, &pInputContext, inputVideoIndex, inputAudioIndex);
	if (-1 == inputVideoIndex)
	{
		printf("open input error\n");
		return ERR_OPEN_INPUT;
	}

	srcW = pInputContext->streams[inputVideoIndex]->codec->width;
	srcH = pInputContext->streams[inputVideoIndex]->codec->height;


	char outFileName[128];
	sprintf(outFileName, "%s", outVideoFileName);
	avformat_alloc_output_context2(&pOutContext, NULL, NULL, outFileName);
	if (!pOutContext)
	{
		printf("can not alloc output context!\n");
		ret_code = ERR_OCTX_ALLOC;
		return ret_code;
	}
	int tmpRet = 0;

	AVDictionaryEntry *tag = NULL;

	//new out put video stream
	int frameRate = pInputContext->streams[inputVideoIndex]->r_frame_rate.num / pInputContext->streams[inputVideoIndex]->r_frame_rate.den;
	{
		tmpRet = NewVideoStream(pOutContext, pInputContext->streams[inputVideoIndex]->codec, outPutVideoIndex, pInputContext->streams[inputVideoIndex]->codec->width,
			pInputContext->streams[inputVideoIndex]->codec->height,  AV_CODEC_ID_H264, frameRate, pInputContext->streams[inputVideoIndex]);

		if (0 != tmpRet)
		{
			printf("new out put vedio stream error\n");
			//			return ERR_OPEN_VEDIO_OUTPUT;
		}
	}
	pPenWordIntoPic = new CPenWordIntoPic();
	pPenWordIntoPic->SetSubTitile(SubtitleWords, pOutContext->streams[outPutVideoIndex]->codec);


	if (!(pOutContext->oformat->flags & AVFMT_NOFILE))
	{
		if(avio_open(&pOutContext->pb, outFileName, AVIO_FLAG_WRITE) < 0)
		{
			printf("can not open output file handle!\n");
			ret_code = ERR_OPEN_FILE_HANLDE;
			return ret_code;
		}
	}

	if(avformat_write_header(pOutContext, NULL) < 0)
	{
		printf("can not write the header of the output file!\n");
		ret_code = ERR_WRITE_HEAD;
		return ret_code;
	}

	int src_frame_size = avpicture_get_size(pOutContext->streams[outPutVideoIndex]->codec->pix_fmt,
		pOutContext->streams[outPutVideoIndex]->codec->width,
		pOutContext->streams[outPutVideoIndex]->codec->height);

	if (NULL == pFifoVideo)
	{
		pFifoVideo = av_fifo_alloc(30 * (src_frame_size + sizeof(int64_t)));
	}

	// CreateThread( NULL, 0, DecodeWork, 0, 0, NULL);
	pthread_t ntid;
	pthread_create(&ntid, NULL, DecodeWork, NULL);

	CodecWork();
}

void* DecodeWork(void* lpParam )
{
	AVFrame* pFrameBuf = av_frame_alloc();
	int w = pOutContext->streams[outPutVideoIndex]->codec->width;
	int h = pOutContext->streams[outPutVideoIndex]->codec->height;
	AVPixelFormat tmpPixFmt = pOutContext->streams[outPutVideoIndex]->codec->pix_fmt;
	int frame_size = avpicture_get_size(tmpPixFmt, w, h);
	int y_size = w * h;

	FILE* pVideoFile = NULL;
	//pVideoFile = fopen("test.yuv", "w");
	AVDictionaryEntry *tag = NULL;
	tag = av_dict_get(pInputContext->streams[inputVideoIndex]->metadata,
		"rotate", tag, 0);

	int rotate = 0;
	if (tag != NULL)
	{
		rotate = atoi(tag->value);
	}

	CConvertImg *_pPicConvertOne = new CConvertImg(
		pInputContext->streams[inputVideoIndex]->codec->width,
		pInputContext->streams[inputVideoIndex]->codec->height,
		w,
		h,
		pInputContext->streams[inputVideoIndex]->codec->pix_fmt,
		tmpPixFmt, rotate);

	while(bWork)
	{
		if (av_fifo_space(pFifoVideo) < (frame_size + sizeof(int64_t)))
		{
			sleep(1);
			continue;
		}

		AVPacket* packet = new AVPacket;
		av_init_packet(packet);

		packet->data = NULL;
		packet->size = 0;

		if (av_read_frame(pInputContext, packet) < 0)
		{
			bIsFileEnd = true;
			break;
		}

		//if a video packet
		if (packet->stream_index == inputVideoIndex)
		{
			int getFrame = 0;
			if (avcodec_decode_video2(pInputContext->streams[inputVideoIndex]->codec, pFrameBuf, &getFrame, packet) < 0)
			{
				av_packet_unref(packet);
				delete packet;
				continue;
			}

			//write fifo
			if (getFrame)
			{
				if (av_fifo_space(pFifoVideo) >= (frame_size + sizeof(int64_t)))
				{
					AVFrame* tmpCvFrame = _pPicConvertOne->GetAConvertFrameData(pFrameBuf);

					if (pVideoFile)
					{
						for(int j=0; j<h; j++)
							fwrite(tmpCvFrame->data[0] + j * tmpCvFrame->linesize[0], 1, w, pVideoFile);
						for(int j=0; j<h/2; j++)
							fwrite(tmpCvFrame->data[1] + j * tmpCvFrame->linesize[1], 1, w/2, pVideoFile);
						for(int j=0; j<h/2; j++)
							fwrite(tmpCvFrame->data[2] + j * tmpCvFrame->linesize[2], 1, w/2, pVideoFile);
					}

					{
						int64_t tmpPts = pFrameBuf->pkt_pts;
						AVFrame *tmpFrame = pPenWordIntoPic->GetAFrameWithWord(tmpCvFrame);
						if (tmpFrame && tmpFrame->data && tmpFrame->data[0])
						{
							if (pVideoFile)
							{
								for(int j=0; j<h; j++)
									fwrite(tmpFrame->data[0] + j * tmpFrame->linesize[0], 1, w, pVideoFile);
								for(int j=0; j<h/2; j++)
									fwrite(tmpFrame->data[1] + j * tmpFrame->linesize[1], 1, w/2, pVideoFile);
								for(int j=0; j<h/2; j++)
									fwrite(tmpFrame->data[2] + j * tmpFrame->linesize[2], 1, w/2, pVideoFile);
							}

							av_fifo_generic_write(pFifoVideo, tmpFrame->data[0], y_size, NULL);
							av_fifo_generic_write(pFifoVideo, tmpFrame->data[1], y_size / 4, NULL);
							av_fifo_generic_write(pFifoVideo, tmpFrame->data[2], y_size / 4, NULL);
							av_fifo_generic_write(pFifoVideo, &tmpPts, sizeof(int64_t), NULL);

						}
					}
				}
			}

			av_packet_unref(packet);
			delete packet;
		}
		else
		{
			av_packet_unref(packet);
			delete packet;
		}
	}

	if (pVideoFile)
	{
		fclose(pVideoFile);
	}

	av_frame_free(&pFrameBuf);

	return ((void *)0);
}


void CodecWork()
{
	int64_t cur_pts_v=0,cur_pts_a=0;

	int w = pOutContext->streams[outPutVideoIndex]->codec->width;
	int h = pOutContext->streams[outPutVideoIndex]->codec->height;
	AVPixelFormat tmpPixFmt = pOutContext->streams[outPutVideoIndex]->codec->pix_fmt;
	int frame_size = avpicture_get_size(tmpPixFmt, w, h);

	uint8_t *tmpPicture_buf = NULL;
	if (NULL == tmpPicture_buf)
		tmpPicture_buf = new uint8_t[frame_size];

	int audioFrameIndex = 0;
	int videoFrameIndex = 0;
	int videoFrameEncodeIndex = 0;

	AVPacket pkt_out;

	FILE* pVideoFile = NULL;
	//pVideoFile = fopen("testCodec.yuv", "w");

	while(bWork)
	{
		if (bIsFileEnd && av_fifo_size(pFifoVideo) < (frame_size + sizeof(int64_t)))
		{
			break;
		}

		{
			if (av_fifo_size(pFifoVideo) < (frame_size + sizeof(int64_t)))
			{
				if (bIsFileEnd)
				{
					cur_pts_v = 0x7fffffffffffffff;
					continue;
				}
				sleep(1);
				continue;
			}
			int64_t tmpPts = 0;

			av_fifo_generic_read(pFifoVideo, tmpPicture_buf, frame_size, NULL);
			av_fifo_generic_read(pFifoVideo, &tmpPts, sizeof(int64_t), NULL);

			AVFrame *pFrame = NULL;
			pFrame = av_frame_alloc();

			avpicture_fill((AVPicture *)pFrame, tmpPicture_buf,
				pOutContext->streams[outPutVideoIndex]->codec->pix_fmt,
				pOutContext->streams[outPutVideoIndex]->codec->width,
				pOutContext->streams[outPutVideoIndex]->codec->height);

			if (pVideoFile)
			{
				for(int j=0; j<h; j++)
					fwrite(pFrame->data[0] + j * pFrame->linesize[0], 1, w, pVideoFile);
				for(int j=0; j<h/2; j++)
					fwrite(pFrame->data[1] + j * pFrame->linesize[1], 1, w/2, pVideoFile);
				for(int j=0; j<h/2; j++)
					fwrite(pFrame->data[2] + j * pFrame->linesize[2], 1, w/2, pVideoFile);
			}

			pFrame->pts = av_rescale_q_rnd(tmpPts,
				pInputContext->streams[inputVideoIndex]->time_base,
				pOutContext->streams[outPutVideoIndex]->time_base,
				(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

			int got_picture = 0;


			av_init_packet(&pkt_out);
			pkt_out.data = NULL;
			pkt_out.size = 0;

			pFrame->format = pOutContext->streams[outPutVideoIndex]->codec->pix_fmt;
			pFrame->width = pOutContext->streams[outPutVideoIndex]->codec->width;
			pFrame->height = pOutContext->streams[outPutVideoIndex]->codec->height;

			int ret = avcodec_encode_video2(pOutContext->streams[outPutVideoIndex]->codec,
				&pkt_out, pFrame, &got_picture);

			if (ret < 0)
			{
				continue;
			}
			videoFrameIndex++;
			if (got_picture == 1)
			{
				pkt_out.stream_index = outPutVideoIndex;

				pkt_out.duration = ((pOutContext->streams[outPutVideoIndex]->time_base.den /
					pOutContext->streams[outPutVideoIndex]->time_base.num) / 30);
				cur_pts_v = pkt_out.pts;
				ret = av_interleaved_write_frame(pOutContext, &pkt_out);
				videoFrameEncodeIndex++;
				av_packet_unref(&pkt_out);
			}
			av_packet_unref(&pkt_out);
			av_frame_free(&pFrame);
		}
	}

	flush_video_encoder(pOutContext, outPutVideoIndex);

	if (pVideoFile)
	{
		fclose(pVideoFile);
		pVideoFile = NULL;
	}

	av_write_trailer(pOutContext);
	free_output_fmtCtx(&pOutContext);

	if (tmpPicture_buf)
	{
		delete[] tmpPicture_buf;
		tmpPicture_buf = NULL;
	}

	printf("CConcatVideoAndAudio work done!!\n");
}


void free_output_fmtCtx(AVFormatContext **fmtCtx)
{
	if (*fmtCtx)
	{
		for (int i = 0; i < (*fmtCtx)->nb_streams; i++)
		{
			if ((*fmtCtx)->streams[i]->codec)
			{
				avcodec_close((*fmtCtx)->streams[i]->codec);
			}
		}

		if ((*fmtCtx)->pb)
		{
			avio_close((*fmtCtx)->pb);
		}

		avformat_free_context((*fmtCtx));
	}
}

int flush_video_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket pkt_out;
	int frame_num = 0;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
	{
		return 0;
	}

	while (1)
	{
		pkt_out.data = NULL;
		pkt_out.size = 0;
		av_init_packet(&pkt_out);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &pkt_out, NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
		{
			break;
		}

		if (!got_frame)
		{
			ret=0;
			break;
		}

		pkt_out.stream_index = stream_index;

		/* mux encoded frame */
		ret = av_interleaved_write_frame(fmt_ctx, &pkt_out);
		frame_num++;
		if (ret < 0)
		{
			break;
		}
	}

	return ret;
}

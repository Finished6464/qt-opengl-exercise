
#include "videodecoder.h"
#include <stdio.h>
#include <unistd.h>

VideoDecoder::VideoDecoder()
{
    code_ctx_  = nullptr;
    fmt_ctx_ = nullptr;
    packet_ = nullptr;
    frame_ = nullptr;
    frame_number_ = 0;
    is_loaded_ = false;
}

VideoDecoder::~VideoDecoder()
{
    Unload();
}

void VideoDecoder::GetErrorMessage(int err, char *txt, int size) const
{
    av_strerror(err, txt, size);
}

int VideoDecoder::LoadFromFile(const char* filename)
{
    int err = 0;
    AVCodec *codec = nullptr;
    AVCodecParameters *origin_par = nullptr;

    err = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);
    if (err < 0) {
        fprintf(stderr, "avformat_open_input failed(%d).\n", err);
        return err;
    }

    err = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (err < 0) {
        fprintf(stderr, "avformat_find_stream_info failed(%d).\n", err);
        return err;
    }

    video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index_ < 0) {
        err = video_stream_index_;
        fprintf(stderr, "av_find_best_stream failed(%d).\n", err);
        return err;
    }

    origin_par = fmt_ctx_->streams[video_stream_index_]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        err = AVERROR_DECODER_NOT_FOUND;
        fprintf(stderr, "avcodec_find_decoder failed(%d).\n", err);
        return err;
    }

//    if (codec->id == AV_CODEC_ID_H264) {
//        AVCodec* h264_mmal = avcodec_find_decoder_by_name("h264_mmal");
//        if (h264_mmal) {
//            codec = h264_mmal;
//        }
//    }

    code_ctx_ = avcodec_alloc_context3(codec);
    if (!code_ctx_) {
        err = AVERROR(ENOMEM);
        fprintf(stderr, "avcodec_alloc_context3 failed(%d).\n", err);
        return err;
    }

    err = avcodec_parameters_to_context(code_ctx_, origin_par);
    if (err < 0) {
        fprintf(stderr, "avcodec_parameters_to_context failed(%d).\n", err);
        return err;
    }

    //enable multithread 0 or more than 1
    // 0: 由ffmpeg决定
    // >1: 用户指定
    code_ctx_->thread_count = 0;
//    code_ctx_->thread_type = FF_THREAD_SLICE;
//    code_ctx_->rc_max_rate = 5000 * 1000;
//    code_ctx_->rc_buffer_size = 20 * 1000;
//    code_ctx_->flags2 |= AV_CODEC_FLAG2_FAST;
    err = avcodec_open2(code_ctx_, codec, nullptr);
    if (err < 0) {
        fprintf(stderr, "avcodec_open2 failed(%d).\n", err);
        return err;
    }

    packet_ = av_packet_alloc();
    if (!packet_) {
        err = AVERROR(ENOMEM);
        fprintf(stderr, "av_packet_alloc failed(%d).\n", err);
        return err;
    }

    frame_ = av_frame_alloc();
    if (!frame_) {
        err = AVERROR(ENOMEM);
        fprintf(stderr, "av_frame_alloc failed(%d).\n", err);
        return err;
    }

//    buff_size_ = av_image_get_buffer_size(code_ctx_->pix_fmt, code_ctx_->width, code_ctx_->height, BUFF_ALIGN);
//    buff_ = (unsigned char *)av_malloc(buff_size_);
//    if (!buff_) {
//        qCritical() << "av_malloc failed";
//        goto failed;
//    }

    is_loaded_ = true;
    return 0;
}

void VideoDecoder::Unload()
{
    if (code_ctx_) {
        avcodec_free_context(&code_ctx_);
        code_ctx_ = nullptr;
    }

    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }

    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }

    frame_number_ = 0;
    is_loaded_ = false;
}

double VideoDecoder::GetDuration() const
{
    if (!fmt_ctx_ || fmt_ctx_->streams[video_stream_index_]->duration < 0)
        return 0;
    return fmt_ctx_->streams[video_stream_index_]->duration * av_q2d(fmt_ctx_->streams[video_stream_index_]->time_base);
}

double VideoDecoder::GetFrameTime() const
{
    if (!frame_ || frame_->pts < 0)
        return 0;
    return frame_->pts * av_q2d(fmt_ctx_->streams[video_stream_index_]->time_base);
}

int VideoDecoder::SeekFrame(double t)
{
    int64_t timestamp = (int64_t)(t / av_q2d(fmt_ctx_->streams[video_stream_index_]->time_base));
    int err = av_seek_frame(fmt_ctx_, video_stream_index_, timestamp, AVSEEK_FLAG_BACKWARD);
    if (err < 0) {
        fprintf(stderr, "avcodec_send_packet failed(%d).\n", err);
        return err;
    }

    avcodec_flush_buffers(code_ctx_);
    return ReadFrameIntenal();
}

int VideoDecoder::ReadFrame(const unsigned char* const **data, const int **linesize, int *num)
{
    int err = ReadFrameIntenal();
    if (err == 0) {
        *num = sizeof(frame_->linesize) / sizeof(frame_->linesize[0]);
        *data = (const unsigned char* const *)frame_->data;
        *linesize = frame_->linesize;
    }

    return err;
}

int VideoDecoder::ReadFrameIntenal()
{
    int err;
    const AVRational& time_base = fmt_ctx_->streams[video_stream_index_]->time_base;

    do {
        av_packet_unref(packet_);
        av_init_packet(packet_);
        err = av_read_frame(fmt_ctx_, packet_);
        if (err < 0 && err != AVERROR_EOF) {
            fprintf(stderr, "av_read_frame failed(%d).\n", err);
            return err;
        }

        if (err == AVERROR_EOF || packet_->stream_index == video_stream_index_) {
again:
            err = avcodec_send_packet(code_ctx_, err != AVERROR_EOF ? packet_ : nullptr);
            if (err < 0) {
                if (err == AVERROR(EAGAIN)) {
                    fprintf(stderr, "avcodec_send_packet return EAGAIN.\n");
                    usleep(100);
                    goto again;
                }

                if (err != AVERROR_EOF) {
                    fprintf(stderr, "avcodec_send_packet failed(%d).\n", err);
                }

                return err;
            }

            err = avcodec_receive_frame(code_ctx_, frame_);
            if (err < 0) {
                if (err == AVERROR(EAGAIN)) {
                    fprintf(stderr, "avcodec_receive_frame return EAGAIN.\n");
                    usleep(5000);
                    goto again;
                }

                if (err != AVERROR_EOF) {
                    fprintf(stderr, "avcodec_receive_frame failed(%d).\n", err);
                }

                return err;
            }

            frame_number_++;
            return 0;
        }
    } while (1);
}




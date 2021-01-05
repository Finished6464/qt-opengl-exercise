#ifndef VIDEODECODER_H
#define VIDEODECODER_H


#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif


class VideoDecoder
{
public:
    VideoDecoder();
    virtual ~VideoDecoder();

    int LoadFromFile(const char* filename);
    void Unload();

    double GetDuration() const;
    double GetFrameTime() const;
    int ReadFrame(const unsigned char* const **data, const int **linesize, int *num);
    int SeekFrame(double t);
    int GetFrameCount() const { return frame_number_; }
    int GetFrameWidth() const { return code_ctx_ ? code_ctx_->width : 0; }
    int GetFrameHeight() const { return code_ctx_ ? code_ctx_->height : 0; }

    void GetErrorMessage(int err, char *txt, int size) const;

private:
    int ReadFrameIntenal();

private:
    AVCodecContext* code_ctx_;
    AVFormatContext* fmt_ctx_;
    AVPacket* packet_;
    AVFrame *frame_;
    int video_stream_index_;

    int frame_number_;
};

#endif // VIDEODECODER_H

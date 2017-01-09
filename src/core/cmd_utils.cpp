#include "core/cmd_utils.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/eval.h>
#include <libavutil/display.h>
}

int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec) {
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0)
    av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
  return ret;
}

AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec) {
  AVDictionary* ret = NULL;
  AVDictionaryEntry* t = NULL;
  int flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char prefix = 0;
  const AVClass* cc = avcodec_get_class();

  if (!codec)
    codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);

  switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      prefix = 'v';
      flags |= AV_OPT_FLAG_VIDEO_PARAM;
      break;
    case AVMEDIA_TYPE_AUDIO:
      prefix = 'a';
      flags |= AV_OPT_FLAG_AUDIO_PARAM;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      prefix = 's';
      flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
      break;
  }

  while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
    char* p = strchr(t->key, ':');

    /* check stream specification in opt name */
    if (p)
      switch (check_stream_specifier(s, st, p + 1)) {
        case 1:
          *p = 0;
          break;
        case 0:
          continue;
        default:
          exit_program(1);
      }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec ||
        (codec->priv_class &&
         av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)))
      av_dict_set(&ret, t->key, t->value, 0);
    else if (t->key[0] == prefix &&
             av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))
      av_dict_set(&ret, t->key + 1, t->value, 0);

    if (p)
      *p = ':';
  }
  return ret;
}

AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts) {
  if (!s->nb_streams) {
    return NULL;
  }

  AVDictionary** opts = static_cast<AVDictionary**>(av_mallocz_array(s->nb_streams, sizeof(*opts)));
  if (!opts) {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
    return NULL;
  }

  for (unsigned int i = 0; i < s->nb_streams; i++) {
    opts[i] =
        filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
  }
  return opts;
}

double get_rotation(AVStream* st) {
  AVDictionaryEntry* rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
  uint8_t* displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
  double theta = 0;

  if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
    char* tail;
    theta = av_strtod(rotate_tag->value, &tail);
    if (*tail)
      theta = 0;
  }
  if (displaymatrix && !theta)
    theta = -av_display_rotation_get((int32_t*)displaymatrix);

  theta -= 360 * floor(theta / 360 + 0.9 / 360);

  if (fabs(theta - 90 * round(theta / 90)) > 2)
    av_log(NULL, AV_LOG_WARNING,
           "Odd rotation angle.\n"
           "If you want to help, upload a sample "
           "of this file to ftp://upload.ffmpeg.org/incoming/ "
           "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

  return theta;
}

void exit_program(int ret) {
  exit(ret);
}

void* grow_array(void* array, int elem_size, int* size, int new_size) {
  if (new_size >= INT_MAX / elem_size) {
    av_log(NULL, AV_LOG_ERROR, "Array too big.\n");
    exit_program(1);
  }
  if (*size < new_size) {
    uint8_t* tmp = static_cast<uint8_t*>(av_realloc_array(array, new_size, elem_size));
    if (!tmp) {
      av_log(NULL, AV_LOG_ERROR, "Could not alloc buffer.\n");
      exit_program(1);
    }
    memset(tmp + *size * elem_size, 0, (new_size - *size) * elem_size);
    *size = new_size;
    return tmp;
  }
  return array;
}

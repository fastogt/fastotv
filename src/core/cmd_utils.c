#include "core/cmd_utils.h"

#include <libavutil/opt.h>

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
  int i;
  AVDictionary** opts;

  if (!s->nb_streams)
    return NULL;
  opts = av_mallocz_array(s->nb_streams, sizeof(*opts));
  if (!opts) {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
    return NULL;
  }
  for (i = 0; i < s->nb_streams; i++)
    opts[i] =
        filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
  return opts;
}


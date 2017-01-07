#ifndef CMD_UTILS_H
#define CMD_UTILS_H

#include "ffmpeg_config.h"

#include <libavutil/dict.h>
#include <libavformat/avformat.h>

/**
 * Filter out options for given codec.
 *
 * Create a new options dictionary containing only the options from
 * opts which apply to the codec with ID codec_id.
 *
 * @param opts     dictionary to place options in
 * @param codec_id ID of the codec that should be filtered for
 * @param s Corresponding format context.
 * @param st A stream from s for which the options should be filtered.
 * @param codec The particular codec for which the options should be filtered.
 *              If null, the default one is looked up according to the codec id.
 * @return a pointer to the created dictionary
 */
AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec);

/**
 * Setup AVCodecContext options for avformat_find_stream_info().
 *
 * Create an array of dictionaries, one dictionary for each stream
 * contained in s.
 * Each dictionary will contain the options from codec_opts which can
 * be applied to the corresponding stream codec context.
 *
 * @return pointer to the created array of dictionaries, NULL if it
 * cannot be created
 */
AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts);

#endif /* CMDUTILS_H */

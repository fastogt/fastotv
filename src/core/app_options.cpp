#include "core/app_options.h"

#include <stddef.h>

AppOptions::AppOptions()
    : autorotate(0), exit_on_keydown(0), exit_on_mousedown(0), sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {}

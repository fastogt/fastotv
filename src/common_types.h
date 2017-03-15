#pragma once

#include <stdint.h>

#include <limits>

namespace fasto {
namespace fastotv {

typedef uint64_t stream_id;  // must be unique
static const stream_id invalid_stream_id = std::numeric_limits<stream_id>::max();
}
}

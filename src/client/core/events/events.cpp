#include "client/core/events/events.h"

#include <common/time.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

TimeInfo::TimeInfo() : time_millisecond(common::time::current_mstime()) {}

PreExecInfo::PreExecInfo(int code) : code(code) {}

PostExecInfo::PostExecInfo(int code) : code(code) {}
}
}
}
}
}

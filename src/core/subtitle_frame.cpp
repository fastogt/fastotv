#include "core/subtitle_frame.h"

namespace core {

SubtitleFrame::SubtitleFrame() : sub(), serial(0), pts(0), width(0), height(0), uploaded(0) {}

SubtitleFrame::~SubtitleFrame() {
  ClearFrame();
}

void SubtitleFrame::ClearFrame() {
  avsubtitle_free(&sub);
}

}  // namespace core

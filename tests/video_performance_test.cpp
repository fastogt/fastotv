#include <common/application/application.h>
#include <common/threads/types.h>  // for condition_variable, mutex

extern "C" {
#include <libavdevice/avdevice.h>  // for avdevice_register_all
}

#include "client/core/events/events.h"
#include "client/core/sdl_utils.h"
#include "client/core/video_state.h"
#include "client/core/video_state_handler.h"

using namespace fasto::fastotv;
using namespace fasto::fastotv::client;
using namespace fasto::fastotv::client::core;

namespace {
struct DictionaryOptions {
  DictionaryOptions() : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {
    av_dict_set(&sws_dict, "flags", "bicubic", 0);
  }

  ~DictionaryOptions() {
    av_dict_free(&swr_opts);
    av_dict_free(&sws_dict);
    av_dict_free(&format_opts);
    av_dict_free(&codec_opts);
  }
  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;

 private:
  DISALLOW_COPY_AND_ASSIGN(DictionaryOptions);
};
}  // namespace

class FakeHandler : public VideoStateHandler {
  virtual ~FakeHandler() {}

  // audio
  virtual bool HandleRequestAudio(VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params,
                                  int* audio_buff_size) override {
    UNUSED(stream);
    UNUSED(wanted_channel_layout);
    UNUSED(wanted_nb_channels);
    UNUSED(wanted_sample_rate);
    UNUSED(audio_buff_size);

    core::AudioParams laudio_hw_params;
    if (!core::init_audio_params(3, 48000, 2, &laudio_hw_params)) {
      return false;
    }

    *audio_hw_params = laudio_hw_params;
    return true;
  }
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override {
    UNUSED(audio_stream_ptr);
    UNUSED(src);
    UNUSED(len);
    UNUSED(volume);
  }

  // video
  virtual bool HandleRequestVideo(VideoState* stream, int width, int height, int format, AVRational sar) override {
    UNUSED(stream);
    UNUSED(width);
    UNUSED(height);
    UNUSED(format);
    UNUSED(sar);
    return true;
  }

  virtual void HandleFrameResize(VideoState* stream, int width, int height, int format, AVRational sar) override {
    UNUSED(stream);
    UNUSED(width);
    UNUSED(height);
    UNUSED(format);
    UNUSED(sar);
  }

  virtual void HandleQuitStream(VideoState* stream, int exit_code, common::Error err) override {
    UNUSED(stream);
    UNUSED(exit_code);
    UNUSED(err);
  }
};

class FakeApplication : public common::application::IApplicationImpl {
 public:
  FakeApplication(int argc, char** argv) : common::application::IApplicationImpl(argc, argv), stop_(false) {}

  virtual int PreExec() override { /* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    return EXIT_SUCCESS;
  }
  virtual int Exec() override {
    const stream_id id = "unique";
    // wget http://download.blender.org/peach/bigbuckbunny_movies/big_buck_bunny_1080p_h264.mov
    const common::uri::Uri uri = common::uri::Uri("file://" PROJECT_TEST_SOURCES_DIR "/big_buck_bunny_1080p_h264.mov");
    core::AppOptions opt;
    DictionaryOptions* dict = new DictionaryOptions;
    const core::ComplexOptions copt(dict->swr_opts, dict->sws_dict, dict->format_opts, dict->codec_opts);
    VideoStateHandler* handler = new FakeHandler;
    VideoState* vs = new VideoState(id, uri, opt, copt, handler);
    int res = vs->Exec();
    if (res != EXIT_SUCCESS) {
      delete handler;
      delete dict;
      return res;
    }

    std::thread audio([this, vs]() {
      while (!stop_) {
        uint8_t stream_buff[8192];
        vs->UpdateAudioBuffer(stream_buff, sizeof(stream_buff), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
      }
    });

    lock_t lock(stop_mutex_);
    while (!stop_) {
      std::cv_status interrupt_status = stop_cond_.wait_for(lock, std::chrono::milliseconds(20));
      if (interrupt_status == std::cv_status::no_timeout) {  // if notify
      } else {
        vs->TryToGetVideoFrame();
      }
    }

    vs->Abort();
    audio.join();
    delete vs;
    delete handler;
    delete dict;
    return EXIT_SUCCESS;
  }
  virtual int PostExec() override { return EXIT_SUCCESS; }

  virtual void PostEvent(event_t* event) override {
    events::Event* fevent = static_cast<events::Event*>(event);
    if (fevent->GetEventType() == core::events::RequestVideoEvent::EventType) {
      core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
      core::events::FrameInfo fr = avent->info();
      bool res = fr.stream_->RequestVideo(fr.width, fr.height, fr.format, fr.sar);
      if (!res) {
        fApp->Exit(EXIT_FAILURE);
      }
    } else if (fevent->GetEventType() == core::events::QuitStreamEvent::EventType) {
      // events::QuitStreamEvent* qevent = static_cast<core::events::QuitStreamEvent*>(event);
      fApp->Exit(EXIT_SUCCESS);
    } else {
      NOTREACHED();
    }
  }
  virtual void SendEvent(event_t* event) override {
    UNUSED(event);
    NOTREACHED();
  }

  virtual void Subscribe(listener_t* listener, common::events_size_t id) override {
    UNUSED(listener);
    UNUSED(id);
  }
  virtual void UnSubscribe(listener_t* listener, common::events_size_t id) override {
    UNUSED(listener);
    UNUSED(id);
  }
  virtual void UnSubscribe(listener_t* listener) override { UNUSED(listener); }

  virtual void ShowCursor() override {}
  virtual void HideCursor() override {}

  virtual void Exit(int result) override {
    UNUSED(result);
    lock_t lock(stop_mutex_);
    stop_ = true;
    stop_cond_.notify_one();
  }

 private:
  typedef common::unique_lock<common::mutex> lock_t;
  common::condition_variable stop_cond_;
  common::mutex stop_mutex_;
  bool stop_;
};

common::application::IApplicationImpl* CreateApplicationImpl(int argc, char** argv) {
  return new FakeApplication(argc, argv);
}

int main(int argc, char** argv) {
#if defined(NDEBUG)
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#else
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("~/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif
  common::application::Application app(argc, argv, &CreateApplicationImpl);
  return app.Exec();
}

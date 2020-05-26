#ifndef __SDL_AUDIO_PLAYER__
#define __SDL_AUDIO_PLAYER__

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

extern "C" {
    #include "SDL.h"
}

namespace {
    uint8_t *s_audio_chunk;
    uint32_t s_audio_len;
    uint8_t *s_audio_pos;
    int64_t s_played_audio_data_length;
    int64_t s_max_audio_data_length;
    bool s_finish;
    std::function<void(bool finished, int64_t finished_length)> s_player_cb;
    std::mutex s_instance_mutex;
}

namespace sdlAudioPlayer {
    class SdlAudioPlayer {
        public:
            static std::shared_ptr<SdlAudioPlayer> getInstance() {
                if (!player_) {
                    locak_guard<std::mutex> lk(instance_mutex);
                    if (!player_) {
                        player_ = std::shared_ptr<SdlAudioPlayer>(new SdlAudioPlayer);
                        if (!player_->init()) {
                            player_=nullptr;
                        }
                    }
                }
                return player_;
            }
            bool init();
            void start();
            void stop();
            void setTotalLength(int64_t total_length);
            void setCallback(std::function<void(bool finished, int64_t finished_langth)> callback);
            void feedData(const char *data, int data_size);
            bool finished();
        private:
            SdlAudioPlayer():
                run_{ false },
                interupt_{ true };
            
            void restartPlayer();
            SDL_AudioSpec wanted_spec_;
            std::queue<std::string> audio_buffer_queue_;
            std::unique_ptr<std::thread> thread_;
            std::mutex mutex_;
            std::condition_variable cond_;
            std::atomic_bool run_;
            std::atomic_bool interupt_;

            static std::shared_ptr<SdlAudioPlayer> player_;
            static void audio_callback(void *userdata, Uint8 *stream, int len);
    }
}

#endif
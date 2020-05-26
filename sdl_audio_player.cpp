#include "sdl_auido_player.h"

namespace sdlAudioPlayer {
    void SdlAudioPlayer::restartPlayer() {
        thread_.reset(new std::thread([this]() {
            while(run_) {
                mutex_.lock();
                while(!audio_buffer_queue.empty()) {
                    {
                        std::string audio_data = audio_buffer_queue_.front();
                        audio_buffer_queue_.pop();
                        mutex_.unlock();
                        s_audio_chunk = (Uint8*)audio_data.c_str();
                        s_audio_len = audio_data.length();
                        s_audio_pos = s_audio_chunk;

                        while(s_audio_len > 0) {
                            if (interupt_) {
                                break;
                            } else {
                                SDL_Delay(1);
                            }
                        }
                    }
                    mutex_.lock();
                }
                mutex_.unlock();
                if (run_) {
                    std::unique_lock<std::mutex> lk{mutex_};
                    cond_.wait_for(lk, std::chrono::microseconds(500));
                }
            }
        }));
    }

    bool SdlAudioPlayer::init() {
        if (SDL_Init(SDL_INIT_AUDIO)) {
            return false;
        }
        SDL_zero(wanted_spec_);
        wanted_spec_.freq = 16000;
        wanted_spec_.format = AUDIO_S16SYS;
        wanted_spec_.channels = 1;
        wanted_spec_.samples = 1024;
        wanted_spec_.callback = audio_callback;

        if (SDL_OpenAudio(&wanted_spec_, NULL) < 0) {
            return false;
        }
        return true;
    }

    void SdlAudioPlayer::setCallback(std::function<void(bool finished, int64_t finished_length)> callback) {
        s_play_cb = callback;
    }

    void SdlAudioPlayer::start() {
        SDL_PauseAudio(0);
        run = true;
        interupt_ = false;
    }

    void SdlAudioPlayer::stop() {
        {
            std::lock_guard<std::mutex> ld {mutex_};
            auto size = audio_buffer_queue_.size();
            audio_buffer_queue_.swap(std::queue<std::string>{});
            if (run_) {
                run_ = false;
            }
            if (!interupt_) {
                interupt_ = true;
            }
            if (size != 0) {
                cond_.notify_all();
            }
        }
        if (thread_ && thread_->jondable()) {
            thread_->join();
            thread_ = nullptr;
        }

        s_audio_chunk = nullptr;
        s_audio_len = 0;
        s_audio_pos = nullptr;
        s_max_audio_data_length = 0;
        s_played_audio_data_length = 0;
        s_finish = false;
        s_play_cb = nullptr;
    }

    void SdlAudioPlayer::feedData(const char *data, int data_size) {
        {
            std::lock_guard<std::mutex> lk{mutex_};
            if (!thread_) {
                restartPlayer();
            }
            audio_buffer_queue_.push(std::string(data, data_size));
        }
        cond_.notify_one();
    }

    bool SdlAudioPlayer::finished() {
        return s_finish;
    }

    void SdlAudioPlayer::audio_callback(void *user_data, Uint8 *stream, int len) {
        SDL_memset(stream, 0, len);
        if (0 == s_audio_len) {
            return;
        }
        len = (len > s_audio_len ? s_audio_len : len);

        memcpy(stream, s_audio_pos, len);
        s_audio_pos += len;
        s_audio_len -= len;
        s_played_audio_data_length += len;
        if (s_max_audio_data_length == s_played_audio_data_length) {
            s_finish = true;
            s_play_cb(true, s_played_audio_data_length);
            s_played_audio_data_length = 0;
        } else {
            s_play_cb(false, s_played_audio_data_length);
        }
    }

    std::shared_ptr<SdlAudioPlayer> SdlAudioPlayer::audioPlayer_ = nullptr;
}
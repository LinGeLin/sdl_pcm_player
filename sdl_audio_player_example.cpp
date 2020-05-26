#include <fstream>
#include <iostream>
#include "sdl_audio_player.h"
using namespace std;
using namespace sdlAudioPlayer;
int main() {
    string fileName;
    cin >> fileName;
    ifstream fin(fileName, ios::binary);
    if (fin.good()) {
        cout << "Open file Ok!" << endl;
    } else {
        cout << "Open file Error!" << endl;
        exit(-1);
    }
    fin.seek(0, fin.end);
    int total_length = fin.tellg();
    SdlAudioPlayer::getInstance()->setTotalLength(total_length);
    SdlAudioPlayer::getInstance()->setCallbace([](bool finised, int64_t finished_length) {
        cout << "played audio length: " << finished_length << endl;
        if (finished) {
            cout << "finished !" << endl;
        }
    });
    SdlAudioPlayer::getInstance()->start();
    fin.seek(0, fin.beg);
    char buffer[2048] = {0};
    while(!fin.eof()) {
        fin.read(buffer, 2048);
        SdlAudioPlayer::getInstance()->feedData(buffer, fin.gcount());
    }
    return 0;
}
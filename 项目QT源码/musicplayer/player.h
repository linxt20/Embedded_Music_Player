#ifndef PLAYER_H
#define PLAYER_H
#include <alsa/asoundlib.h>
#include <QString>

class player
{
public:
    player();
    void init_pcm();
    void free_pcm();
    void init_volume();
    void song_play(QString filepath);
    void song_pause();
    void song_continue();
    void song_stop();
    void set_volume(int s_volume);
    void set_speed(double s_times);
    void play_front();
    void play_back();
    int decode_mp3_file(char *input_file_name,char *output_file_name);
    void open_music_file(const char *path_name);
    void clean_up();

public:
    snd_mixer_t *mixer = NULL;
    snd_mixer_elem_t *elem = NULL;
    snd_mixer_selem_id_t *mixer_selem_id;

    const char *card = "hw:0";
    const char *selem_name = "Playback";
    unsigned int raw_rate = 0;
    double times = 1.0;
    
    pthread_mutex_t Playing_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t isPlaying_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t fp_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t pcm_handle_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t notification = PTHREAD_COND_INITIALIZER;
    
    bool isPlaying = false;
    bool Playing = false;
    bool adjustSpeed = false;
    
    int32_t volume = 50;

    int second = 48;
    
};

#define TMP_FILE_NAME ".temp.wav"

#endif // PLAYER_H

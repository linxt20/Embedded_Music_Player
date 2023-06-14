#define _CRT_SECURE_NO_WARNINGS
#include "player.h"
#include "const.h"
#include <alsa/asoundlib.h>
#include <QFileInfo>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include<QDebug>
#include <QMessageBox>

int debug_msg(int result, const char *str)
{
    if (result < 0)
    {
        fprintf(stderr, "err: %s 失败!, result = %d, err_info = %s \n", str, result, snd_strerror(result));
        exit(1);
    }
    return 0;
}

player::player()
{

}

void player::init_pcm(){
    // 在堆栈上分配snd_pcm_hw_params_t结构体的空间，参数是配置pcm硬件的指针,返回0成功
    debug_msg(snd_pcm_hw_params_malloc(&hw_params), "分配snd_pcm_hw_params_t结构体");

    // 打开PCM设备 返回0 则成功，其他失败
    // 函数的最后一个参数是配置模式，如果设置为0,则使用标准模式
    // 其他值位SND_PCM_NONBLOCL和SND_PCM_ASYNC 如果使用NONBLOCL 则读/写访问, 如果是后一个就会发出SIGIO
    pcm_name = strdup("default");
    debug_msg(snd_pcm_open(&pcm_handle, pcm_name, stream, 0), "打开PCM设备");

    // 在我们将PCM数据写入声卡之前，我们必须指定访问类型，样本长度，采样率，通道数，周期数和周期大小。
    // 首先，我们使用声卡的完整配置空间之前初始化hwparams结构
    debug_msg(snd_pcm_hw_params_any(pcm_handle, hw_params), "配置空间初始化");

    // 设置交错模式（访问模式）
    // 常用的有 SND_PCM_ACCESS_RW_INTERLEAVED（交错模式） 和 SND_PCM_ACCESS_RW_NONINTERLEAVED （非交错模式）
    debug_msg(snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED), "设置交错模式（访问模式）");

    debug_msg(snd_pcm_hw_params_set_format(pcm_handle, hw_params, pcm_format), "设置样本长度(位数)");

    // 设置采样率为44.1KHz dir的范围(-1,0,1) 88.2
    // rate = wav_header.sample_rate;
    debug_msg(snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0), "设置采样率");

    // 设置通道数
    debug_msg(snd_pcm_hw_params_set_channels(pcm_handle, hw_params, wav_header.num_channels), "设置通道数");

    // 设置缓冲区 buffer_size = period_size * periods 一个缓冲区的大小可以这么算，我上面设定了周期为2，
    // 周期大小我们预先自己设定，那么要设置的缓存大小就是 周期大小 * 周期数 就是缓冲区大小了。
    buffer_size = period_size * periods;

    // 为buff分配buffer_size大小的内存空间
    buff = (unsigned char *)malloc(buffer_size);
    memset(buff, 0, buffer_size);

    if (pcm_format == SND_PCM_FORMAT_S16_LE || pcm_format == SND_PCM_FORMAT_S16_BE)
    {
        frames = buffer_size / (2 * wav_header.num_channels);
        debug_msg(snd_pcm_hw_params_set_buffer_size(pcm_handle, hw_params, frames), "设置S16_LE OR S16_BE缓冲区");
    }
    else if (pcm_format == SND_PCM_FORMAT_S24_3LE || pcm_format == SND_PCM_FORMAT_S24_3BE)
    {
        frames = buffer_size / (3 * wav_header.num_channels);
        /*
            当位数为24时，就需要除以6了，因为是24bit * 2 / 8 = 6
        */
        debug_msg(snd_pcm_hw_params_set_buffer_size(pcm_handle, hw_params, frames), "设置S24_3LE OR S24_3BE的缓冲区");
    }
    else if (pcm_format == SND_PCM_FORMAT_S32_LE || pcm_format == SND_PCM_FORMAT_S32_BE || pcm_format == SND_PCM_FORMAT_S24_LE || pcm_format == SND_PCM_FORMAT_S24_BE)
    {
        frames = buffer_size / (4 * wav_header.num_channels);
        /*
            当位数为32时，就需要除以8了，因为是32bit * 2 / 8 = 8
        */
        debug_msg(snd_pcm_hw_params_set_buffer_size(pcm_handle, hw_params, frames), "设置S32_LE OR S32_BE OR S24_LE OR S24_BE缓冲区");
    }
    // 设置的硬件配置参数，加载，并且会自动调用snd_pcm_prepare()将stream状态置为SND_PCM_STATE_PREPARED
    debug_msg(snd_pcm_hw_params(pcm_handle, hw_params), "设置的硬件配置参数");
    init_volume();
}

void player::free_pcm(){
    if (buff){
        free(buff);
        buff = NULL;
    }
    if (mixer) {
        snd_mixer_close(mixer);
        mixer = NULL;
        elem = NULL;
    }
    if (pcm_handle) {
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}

void player::init_volume(){
    debug_msg(snd_mixer_open(&mixer, 0), "snd_mixer_open");
    debug_msg(snd_mixer_attach(mixer, card), "snd_mixer_attach");
    debug_msg(snd_mixer_selem_register(mixer, NULL, NULL), "snd_mixer_selem_register");
    debug_msg(snd_mixer_load(mixer), "snd_mixer_load");

    snd_mixer_selem_id_alloca(&mixer_selem_id);
    snd_mixer_selem_id_set_index(mixer_selem_id, 0);
    snd_mixer_selem_id_set_name(mixer_selem_id, selem_name);
    elem = snd_mixer_find_selem(mixer, mixer_selem_id);
}

void player::song_play(QString filepath){
    QFileInfo fileinfo;
    fileinfo = QFileInfo(filepath);
    QByteArray filepath_T = filepath.toLocal8Bit();
    char* filename = filepath_T.data();
    if (fileinfo.suffix().endsWith("mp3", Qt::CaseInsensitive)) {
        if(decode_mp3_file(filename,TMP_FILE_NAME)<0){
            pthread_exit(NULL);
        }
        open_music_file(TMP_FILE_NAME);
    }
    else{
        open_music_file(filename);
    }
    init_pcm();

    int ret = 0;
    snd_pcm_prepare(pcm_handle);

    pthread_mutex_lock(&isPlaying_mutex);
    isPlaying = true;
    pthread_mutex_unlock(&isPlaying_mutex);

    pthread_mutex_lock(&Playing_mutex);
    Playing = true;
    pthread_mutex_unlock(&Playing_mutex);

    set_volume(volume);
    while (1)
    {
        if (!Playing)
        {
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&pcm_handle_mutex);
        if (!isPlaying)
        {
            memset(buff, 0, buffer_size);
        }
        else
        {
            // 读取文件数据放到缓存中
            pthread_mutex_lock(&fp_mutex);
            if(times == 2.0f){
                int tempbuff_size = buffer_size * times;
                unsigned char* tempbuff;
                tempbuff =  (unsigned char *)malloc(tempbuff_size);
                ret = fread(tempbuff, 1, tempbuff_size, fp);
                std::copy(tempbuff, tempbuff + buffer_size, buff);
                delete[] tempbuff;
            }
            else if(times == 1.5f){
                int tempbuff_size = buffer_size * times;
                unsigned char* tempbuff;
                tempbuff =  (unsigned char *)malloc(tempbuff_size);
                ret = fread(tempbuff, 1, tempbuff_size, fp);
                std::copy(tempbuff, tempbuff + buffer_size, buff);
                delete[] tempbuff;
            }
            else if(times == 1.0f){
                ret = fread(buff, 1, buffer_size, fp);
            }
            else if(times == 0.5f ){
                int tempbuff_size = buffer_size * times;
                unsigned char* tempbuff;
                tempbuff =  (unsigned char *)malloc(tempbuff_size);
                ret = fread(tempbuff, 1, tempbuff_size, fp);
                std::copy(tempbuff, tempbuff + tempbuff_size, buff);
                std::copy(tempbuff, tempbuff + tempbuff_size, buff + tempbuff_size);
                delete[] tempbuff;
            }
            pthread_mutex_unlock(&fp_mutex);
            if (ret == 0)
            {
                pthread_mutex_lock(&Playing_mutex);
                Playing = false;
                pthread_mutex_unlock(&Playing_mutex);
                clean_up();
                pthread_mutex_unlock(&pcm_handle_mutex);
                pthread_exit(NULL);
            }

            if (ret < 0)
            {
                fprintf(stderr, "read pcm from file!; exit thread \n");
                pthread_mutex_lock(&Playing_mutex);
                Playing = false;
                pthread_mutex_unlock(&Playing_mutex);
                clean_up();
                pthread_mutex_unlock(&pcm_handle_mutex);
                pthread_exit(NULL);
            }
        }
        // 向PCM设备写入数据, snd_pcm_writei()函数第三个是帧单位，
        while ((ret = snd_pcm_writei(pcm_handle, buff, frames)) < 0)
        {
            if (ret == -EPIPE)
            {
                //完成硬件参数设置，使设备准备好
                snd_pcm_prepare(pcm_handle);
            }
            else if (ret < 0)
            {
                fprintf(stderr, "ret value is : %d \n", ret);
                debug_msg(-1, "write to audio interface failed");
            }
        }
        pthread_mutex_unlock(&pcm_handle_mutex);
    }
}

void player::song_pause(){
    pthread_mutex_lock(&isPlaying_mutex);
    isPlaying = false;
    pthread_mutex_unlock(&isPlaying_mutex);
}

void player::song_continue(){
    pthread_mutex_lock(&isPlaying_mutex);
    isPlaying = true;
    pthread_mutex_unlock(&isPlaying_mutex);
}

void player::song_stop(){
    pthread_mutex_lock(&Playing_mutex);
    Playing = false;
    pthread_mutex_unlock(&Playing_mutex);
    pthread_mutex_lock(&pcm_handle_mutex);
    clean_up();
    pthread_mutex_unlock(&pcm_handle_mutex);
}

void player::set_volume(int s_volume){
    long min, max;
    if (s_volume < 0)
        s_volume = 0;
    if (s_volume > 100)
        s_volume = 100;
    volume = s_volume;
    if (Playing)
    {
        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        snd_mixer_selem_set_playback_volume_all(elem, (volume+100) * max / 200);
    }
}

void player::set_speed(double s_times){
    if (s_times > 2)
        s_times = 2;
    if (s_times < 0.5)
        s_times = 0.5;
    times = s_times;
}

void player::play_front(){
    if (!Playing)
    {
        return;
    }
    pthread_mutex_lock(&fp_mutex);
    long front_size = 10 * raw_rate * wav_header.num_channels * wav_header.bits_per_sample / 8;
    if (front_size > fsize - ftell(fp) - 1)
    {
        front_size = fsize - ftell(fp) - 1;
    }
    fseek(fp, front_size, SEEK_CUR);
    pthread_mutex_unlock(&fp_mutex);
}

void player::play_back(){
    if (!Playing)
    {
        return;
    }
    pthread_mutex_lock(&fp_mutex);
    long back_size = 10 * raw_rate * wav_header.num_channels * wav_header.bits_per_sample / 8;
    if (back_size > ftell(fp))
    {
        back_size = ftell(fp);
    }
    fseek(fp, -back_size, SEEK_CUR);
    pthread_mutex_unlock(&fp_mutex);
}

void player::open_music_file(const char *path_name)
{
    // 通过fopen函数打开音乐文件
    fp = fopen(path_name, "rb");
    // 判断文件是否为空
    if (fp == NULL)
    {
        QMessageBox message(QMessageBox::Warning, "No music", "music file is NOT find.", QMessageBox::Yes);
        message.exec();
        fprintf(stderr, "music file is NULL \n");
        pthread_exit(NULL);
    }
    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    // 把文件指针定位到文件的开头处
    fseek(fp, 0, SEEK_SET);

    // 读取文件，并解析文件头获取有用信息
    wav_header_size = fread(&wav_header, 1, sizeof(wav_header), fp);
    raw_rate = wav_header.sample_rate;
    rate = raw_rate;
    second = fsize/(raw_rate * wav_header.num_channels * wav_header.bits_per_sample / 8);
    if (wav_header.bits_per_sample == 16)
    {
        pcm_format = SND_PCM_FORMAT_S16_LE;
    }
    else if (wav_header.bits_per_sample == 24)
    {
        pcm_format = SND_PCM_FORMAT_S24_LE;
    }
    else
    {
        pcm_format = SND_PCM_FORMAT_S32_LE;
    }
}

void player::clean_up()
{
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
    fsize = 0;
    free_pcm();
}

void wavWrite_int16(char* filename, int16_t* buffer, uint32_t sampleRate, uint32_t totalSampleCount, unsigned int channels = 1) {
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("文件打开失败.\n");
        return;
    }

    int nbit = 16;
    int nbyte = nbit / 8;
    char text[4] = { 'R', 'I', 'F', 'F' };
    uint32_t long_number = 36 + totalSampleCount * sizeof(int16_t) * channels;

    fwrite(text, 1, 4, fp);
    fwrite(&long_number, 4, 1, fp);
    fwrite("WAVE", 1, 4, fp);
    fwrite("fmt ", 1, 4, fp);
    long_number = 16;
    fwrite(&long_number, 4, 1, fp);
    int16_t short_number = 1;
    fwrite(&short_number, 2, 1, fp);
    short_number = channels;
    fwrite(&short_number, 2, 1, fp);
    fwrite(&sampleRate, 4, 1, fp);
    long_number = sampleRate * nbyte;
    fwrite(&long_number, 4, 1, fp);
    short_number = nbyte;
    fwrite(&short_number, 2, 1, fp);
    short_number = nbit;
    fwrite(&short_number, 2, 1, fp);
    fwrite("data", 1, 4, fp);
    fwrite(&totalSampleCount, 4, 1, fp);
    fwrite(buffer, sizeof(int16_t) * channels, totalSampleCount, fp);
    fclose(fp);
}

int16_t* DecodeMp3ToBuffer(char* filename, uint32_t *sampleRate, uint32_t *totalSampleCount, unsigned int *channels) {
    FILE* fd = fopen(filename, "rb");
    if (fd == NULL)
        return NULL;

    fseek(fd, 0, SEEK_END);
    long music_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    unsigned char* file_buf = (unsigned char*)malloc(music_size);
    if (file_buf == NULL) {
        fclose(fd);
        return NULL;
    }

    if (fread(file_buf, music_size, 1, fd) < 1) {
        fclose(fd);
        free(file_buf);
        return NULL;
    }

    fclose(fd);

    int alloc_samples = 1024 * 1024;
    int num_samples = 0;
    int16_t* music_buf = (int16_t*)malloc(alloc_samples * sizeof(int16_t) * 2);

    mp3dec_t dec;
    mp3dec_init(&dec);

    unsigned char* buf = file_buf;
    mp3dec_frame_info_t info;

    for (;;) {
        int16_t frame_buf[2 * 1152];
        int samples = mp3dec_decode_frame(&dec, buf, music_size, frame_buf, &info);

        if (alloc_samples < (num_samples + samples)) {
            alloc_samples *= 2;
            int16_t* tmp = (int16_t*)realloc(music_buf, alloc_samples * sizeof(int16_t) * 2);
            if (tmp)
                music_buf = tmp;
        }

        if (music_buf)
            memcpy(music_buf + num_samples * info.channels, frame_buf, samples * info.channels * sizeof(int16_t));

        num_samples += samples;

        if (info.frame_bytes <= 0 || music_size <= (info.frame_bytes + 4))
            break;

        buf += info.frame_bytes;
        music_size -= info.frame_bytes;
    }

    if (alloc_samples > num_samples) {
        int16_t* tmp = (int16_t*)realloc(music_buf, num_samples * sizeof(int16_t) * 2);
        if (tmp)
            music_buf = tmp;
    }

    if (sampleRate)
        *sampleRate = info.hz;

    if (channels)
        *channels = info.channels;

    if (totalSampleCount)
        *totalSampleCount = num_samples;

    free(file_buf);

    return music_buf;
}

int player::decode_mp3_file(char* mp3_file,char* wav_file){
    uint32_t totalSampleCount = 0;
    uint32_t sampleRate = 0;
    unsigned int channels = 0;
    int16_t* wavBuffer = DecodeMp3ToBuffer(mp3_file, &sampleRate, &totalSampleCount, &channels);
    wavWrite_int16(wav_file, wavBuffer, sampleRate, totalSampleCount, channels);
    free(wavBuffer);
    return 0;
}

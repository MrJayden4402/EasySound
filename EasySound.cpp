#include "EasySound.h"

namespace EasySound
{
    IXAudio2 *pXAudio = nullptr;
    IXAudio2MasteringVoice *pMasteringVoice = nullptr;
    std::vector<std::pair<std::string, EasyAudio *>> easyAudioLoadList;
    bool EasySoundStartFlag = false;
};

bool __Easy_LoadWaveFileXA(const char *filename, __Easy_AudioData &out)
{
    HMMIO hFile = mmioOpenA((LPSTR)filename, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (!hFile)
        return false;

    MMCKINFO riffChunk = {};
    riffChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(hFile, &riffChunk, NULL, MMIO_FINDRIFF))
    {
        mmioClose(hFile, 0);
        return false;
    }

    // fmt
    MMCKINFO fmtChunk = {};
    fmtChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hFile, &fmtChunk, &riffChunk, MMIO_FINDCHUNK))
    {
        mmioClose(hFile, 0);
        return false;
    }

    // 按 ckSize 分配并读取完整 fmt 内容
    BYTE *fmtBuf = new BYTE[fmtChunk.cksize];
    LONG rb = mmioRead(hFile, (HPSTR)fmtBuf, fmtChunk.cksize);
    if (rb != (LONG)fmtChunk.cksize)
    {
        delete[] fmtBuf;
        mmioClose(hFile, 0);
        return false;
    }
    mmioAscend(hFile, &fmtChunk, 0);

    // 解释格式
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)fmtBuf;
    if (wfx->wFormatTag == WAVE_FORMAT_PCM || wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        // PCM 或 Float，确保结构完整：如果只有 16 字节，补齐 cbSize=0
        // 直接用读取的缓冲作为 WAVEFORMATEX
        out.pFormat = wfx; // 直接持有该缓冲
    }
    else if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        // 扩展格式，XAudio2 需要完整的 WAVEFORMATEXTENSIBLE
        // 注意：我们已经完整读入 fmtBuf（大小为 ckSize），可直接传
        out.pFormat = wfx; // 指针指向 WAVEFORMATEXTENSIBLE 起始，同样有效
    }
    else
    {
        // 非支持格式（如 ADPCM、mu-law 等）
        delete[] fmtBuf;
        mmioClose(hFile, 0);
        return false;
    }

    // data
    MMCKINFO dataChunk = {};
    dataChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hFile, &dataChunk, &riffChunk, MMIO_FINDCHUNK))
    {
        delete[] fmtBuf;
        mmioClose(hFile, 0);
        return false;
    }

    out.dataSize = dataChunk.cksize;
    out.pPCM = new BYTE[out.dataSize];
    rb = mmioRead(hFile, (HPSTR)out.pPCM, out.dataSize);
    mmioClose(hFile, 0);
    if (rb != (LONG)out.dataSize)
    {
        delete[] out.pPCM;
        out.pPCM = nullptr;
        delete[] fmtBuf;
        out.pFormat = nullptr;
        return false;
    }
    return true;
}

void EasySoundStart()
{
    using namespace EasySound;

    if (EasySoundStartFlag)
        return;
    EasySoundStartFlag = true;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    XAudio2Create(&pXAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);

    pXAudio->CreateMasteringVoice(&pMasteringVoice);

    for (int i = 0; i < EasySound::easyAudioLoadList.size(); i++)
        EasySound::easyAudioLoadList[i].second->Create(EasySound::easyAudioLoadList[i].first);
}

EasyAudio::EasyAudio(std::string filename)
{
    this->Create(filename);
}

void EasyAudio::Create(std::string filename)
{
    using namespace EasySound;

    if (!EasySoundStartFlag)
    {
        EasySound::easyAudioLoadList.push_back({filename, this});
        return;
    }

    if (!__Easy_LoadWaveFileXA(filename.c_str(), audioData))
    {
        MessageBox(NULL, "Error loading audio file", "EasySound Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    // 打印格式信息（便于确认）
    wfx = audioData.pFormat;

    // 直接传入我们从文件读到的格式缓冲（可能是 WAVEFORMATEX 或 WAVEFORMATEXTENSIBLE）
    pXAudio->CreateSourceVoice(&pSourceVoice, wfx);

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = audioData.dataSize;
    buf.pAudioData = audioData.pPCM;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    pSourceVoice->SubmitSourceBuffer(&buf);
}

void EasyAudio::Release()
{
    if (pSourceVoice)
        pSourceVoice->DestroyVoice();
    delete[] audioData.pPCM;
    delete[] (BYTE *)audioData.pFormat;
}

bool EasyAudio::IsFinished()
{
    XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState(&state);
    return state.BuffersQueued == 0;
}

int EasyAudio::GetPlayTime()
{
    XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState(&state);

    if (wfx->nSamplesPerSec == 0)
        return 0;

    int relativeTime = (static_cast<double>(state.SamplesPlayed) / wfx->nSamplesPerSec) * 1000.0;

    if (relativeTime == 0)
        return 0;

    return relativeTime + skipTime;
}

void EasyAudio::Play()
{
    if (IsFinished())
        this->Skip(0);

    pSourceVoice->Start(0);
}

void EasyAudio::Stop()
{
    pSourceVoice->Stop(0);
}

void EasyAudio::Skip(int ms)
{
    pSourceVoice->Stop(0);

    // 基本字节数
    double bytes = (ms / 1000.0) * wfx->nAvgBytesPerSec;

    // 按块对齐
    DWORD aligned = (DWORD)(bytes / wfx->nBlockAlign) * wfx->nBlockAlign;

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = audioData.dataSize - aligned;
    buf.pAudioData = audioData.pPCM + aligned;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    pSourceVoice->FlushSourceBuffers();

    pSourceVoice->SubmitSourceBuffer(&buf);

    skipTime = ms;
}

int EasyAudio::GetTotalTime()
{
    double totalMs = (static_cast<double>(audioData.dataSize) / wfx->nAvgBytesPerSec) * 1000.0;
    return static_cast<int>(totalMs);
}

void EasyAudio::SetSpeed(double speed)
{
    speed = std::max(speed, XAUDIO2_MIN_FREQ_RATIO);
    speed = std::min(speed, XAUDIO2_MAX_FREQ_RATIO);

    pSourceVoice->SetFrequencyRatio(speed);
}

void EasyAudio::SetVolume(double volume)
{
    pSourceVoice->SetVolume(volume);
}

#include <windows.h>
#include <string>
#include <vector>
#include <mmsystem.h>
#include <mmreg.h>
#include <xaudio2.h>

class EasyAudio;

namespace EasySound
{
    extern IXAudio2 *pXAudio;
    extern IXAudio2MasteringVoice *pMasteringVoice;
    extern std::vector<std::pair<std::string, EasyAudio *>> easyAudioLoadList;
};

struct __Easy_AudioData
{
    WAVEFORMATEX *pFormat = nullptr;
    BYTE *pPCM = nullptr;
    DWORD dataSize = 0;
};

void EasySoundStart();

class EasyAudio
{
public:
    __Easy_AudioData audioData;
    IXAudio2SourceVoice *pSourceVoice = nullptr;
    WAVEFORMATEX *wfx;

    int skipTime;

    EasyAudio() = default;

    EasyAudio(std::string filename);

    void Create(std::string filename);

    void Release();

    bool IsFinished();

    int GetPlayTime();

    int GetTotalTime();

    void Play();

    void Stop();

    void Skip(int ms);

    void SetSpeed(double speed);

    void SetVolume(double volume);
};

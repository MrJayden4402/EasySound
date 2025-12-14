# EasySound 4.0 代码文档

## 1. 库概述

该库是基于XAudio2实现的音频播放库，提供了一些简化接口，可以进行简单的音频播放功能。

声明包含在 EasySound.h 中。\
实现包含在 EasySound.cpp 中。

实现中包含一些以__Easy_开头的函数，这些函数是库的内部函数，除非你知道你在做什么，否则你不应该直接调用它们。

使用这个库需要链接一系列lib，具体为加入以下参数：

- -lwinmm
- -lxaudio2_9
- -ole32

## 2. 库函数/类

### 2.1 EasySoundStart函数

该函数用于初始化整个库。

函数头部:
```cpp
void EasySoundStart();
```

### 2.2 EasyAudio类

该类代表一个音频文件，可以播放、暂停等。

#### 2.2.1 构造

调用Create函数，头部如下：

```cpp
void Create(std::string filename);
```

从文件中读取并创建一个音频对象。

注意目前只支持wav文件。

还可以用构造函数进行构造，参数和Create函数相同。

注意在EasySoundStart之前构造的，它们会在EasySoundStart时全部统一加载。

#### 2.2.2 播放

调用Play函数，头部如下：

```cpp
void Play();
```

播放音频。

#### 2.2.3 暂停

调用Stop函数，头部如下：

```cpp
void Stop();
```

暂停音频。

#### 2.2.4 播放状态

检测播放是否结束，使用IsFinished函数，头部如下：

```cpp
bool IsFinished();
```

如果播放结束，返回true，否则返回false。

#### 2.2.5 播放时间

获取当前播放时间，使用GetTime函数，头部如下：

```cpp
int GetPlayTime();
```

返回当前播放时间，单位为毫秒，注意这个播放时间是相对于整个音频的。


#### 2.2.6 音频长度

获取音频长度，使用GetTotalTime函数，头部如下：

```cpp
int GetTotalTime();
```

返回音频长度，单位为毫秒。

#### 2.2.7 调节音量
使用SetVolume函数，头部如下：

```cpp
void SetVolume(float volume);
```

调节音量，1为原声音。

#### 2.2.8 调节播放速度
使用SetSpeed函数，头部如下：

```cpp
void SetSpeed(float speed);
```

调节播放速度，1为原速度。

#### 2.2.9 调节播放位置
使用Skip函数，头部如下：

```cpp
void Skip(int ms);
```

调节播放位置，单位为毫秒。

#### 2.2.10 释放

使用Release函数释放资源，头部如下：

```cpp
void Release();
```

## 3. 历史

1.0 / 2.0 - 2023.4.16\
使用mciapi实现基础功能，但是播放有很多问题。

3.0 - 2024.2.25\
采用多进程播放，但是还是有问题。

4.0 - 2025.11.25\
使用XAudio2实现。

By MrJayden.
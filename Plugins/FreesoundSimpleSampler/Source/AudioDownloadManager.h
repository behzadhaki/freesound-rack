#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"

class AudioDownloadManager : public juce::Thread,
                           public juce::Timer
{
public:
    struct DownloadProgress
    {
        int totalFiles = 0;
        int completedFiles = 0;
        int64 currentFileDownloaded = 0;
        int64 currentFileTotal = 0;
        float overallProgress = 0.0f;
        juce::String currentFileName;
    };

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void downloadProgressChanged(const DownloadProgress& progress) = 0;
        virtual void downloadCompleted(bool success) = 0;
    };

    AudioDownloadManager();
    ~AudioDownloadManager() override;

    void startDownloads(const juce::Array<FSSound>& sounds, const juce::File& downloadDirectory);
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    void run() override;
    void timerCallback() override;
    void updateProgress();

    juce::Array<FSSound> soundsToDownload;
    juce::File targetDirectory;
    juce::ListenerList<Listener> listeners;
    
    DownloadProgress currentProgress;
    juce::CriticalSection progressLock;
    
    std::unique_ptr<juce::WebInputStream> currentStream;
    juce::File currentOutputFile;
    int currentDownloadIndex = 0;
};
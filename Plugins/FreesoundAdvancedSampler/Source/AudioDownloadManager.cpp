#include "AudioDownloadManager.h"

AudioDownloadManager::AudioDownloadManager() : juce::Thread("AudioDownloader")
{
}

AudioDownloadManager::~AudioDownloadManager()
{
    stopThread(2000);
}

void AudioDownloadManager::startDownloads(const juce::Array<FSSound>& sounds, const juce::File& downloadDirectory, const juce::String& searchQuery)
{
    if (isThreadRunning())
        stopThread(2000);

    soundsToDownload = sounds;
    targetDirectory = downloadDirectory;
    currentSearchQuery = searchQuery;
    currentDownloadIndex = 0;

    {
        juce::ScopedLock lock(progressLock);
        currentProgress.totalFiles = sounds.size();
        currentProgress.completedFiles = 0;
        currentProgress.currentFileDownloaded = 0;
        currentProgress.currentFileTotal = 0;
        currentProgress.overallProgress = 0.0f;
        currentProgress.currentFileName = "";
    }

    targetDirectory.createDirectory();
    startThread();
    startTimer(100); // Update UI every 100ms
}

void AudioDownloadManager::run()
{
    bool allSuccessful = true;
    juce::Array<DownloadedFileInfo> downloadedFiles;

    for (int i = 0; i < soundsToDownload.size() && !threadShouldExit(); ++i)
    {
        currentDownloadIndex = i;
        juce::URL url = soundsToDownload[i].getOGGPreviewURL();

        if (url.isEmpty())
            continue;

        // Create filename using just Freesound ID: FS_ID_XXXX.ogg
        juce::String fileName = "FS_ID_" + juce::String(soundsToDownload[i].id) + ".ogg";
        currentOutputFile = targetDirectory.getChildFile(fileName);

        {
            juce::ScopedLock lock(progressLock);
            currentProgress.currentFileName = fileName;
            currentProgress.currentFileDownloaded = 0;
            currentProgress.currentFileTotal = 0;
        }

        // Create web input stream
        juce::URL downloadUrl = url;
        currentStream = std::make_unique<juce::WebInputStream>(downloadUrl, false);

        if (currentStream->connect(nullptr))
        {
            // Get file size if available
            int64 totalSize = currentStream->getTotalLength();

            {
                juce::ScopedLock lock(progressLock);
                currentProgress.currentFileTotal = totalSize;
            }

            // Create output stream
            std::unique_ptr<juce::FileOutputStream> output = currentOutputFile.createOutputStream();

            if (output != nullptr)
            {
                const int bufferSize = 8192;
                juce::HeapBlock<char> buffer(bufferSize);
                int64 totalRead = 0;

                while (!currentStream->isExhausted() && !threadShouldExit())
                {
                    int bytesRead = currentStream->read(buffer, bufferSize);

                    if (bytesRead > 0)
                    {
                        output->write(buffer, bytesRead);
                        totalRead += bytesRead;

                        {
                            juce::ScopedLock lock(progressLock);
                            currentProgress.currentFileDownloaded = totalRead;
                        }
                    }
                    else if (bytesRead == 0)
                    {
                        break; // End of stream
                    }
                    else
                    {
                        allSuccessful = false;
                        break; // Error
                    }
                }

                output->flush();

                // Record successful download info
                if (currentOutputFile.existsAsFile())
                {
                    DownloadedFileInfo fileInfo;
                    fileInfo.fileName = fileName;
                    fileInfo.originalName = soundsToDownload[i].name; // Get original name from the sound object
                    fileInfo.freesoundId = soundsToDownload[i].id;
                    fileInfo.searchQuery = currentSearchQuery;
                    fileInfo.author = soundsToDownload[i].user;
                    fileInfo.license = soundsToDownload[i].license;
                    fileInfo.duration = soundsToDownload[i].duration;
                    fileInfo.fileSize = soundsToDownload[i].filesize;
                    fileInfo.downloadedAt = juce::Time::getCurrentTime().toString(true, true);
                    fileInfo.padIndex = i; // Store original download order
                    downloadedFiles.add(fileInfo);
                }
            }
            else
            {
                allSuccessful = false;
            }
        }
        else
        {
            allSuccessful = false;
        }

        currentStream.reset();

        {
            juce::ScopedLock lock(progressLock);
            currentProgress.completedFiles = i + 1;
        }
    }

    stopTimer();

    // Final progress update
    {
        juce::ScopedLock lock(progressLock);
        currentProgress.overallProgress = 1.0f;
        currentProgress.currentFileName = allSuccessful ? "All downloads completed!" : "Some downloads failed";
    }
    updateProgress();

    listeners.call([allSuccessful](Listener& l) { l.downloadCompleted(allSuccessful); });
}

void AudioDownloadManager::timerCallback()
{
    updateProgress();
}

void AudioDownloadManager::updateProgress()
{
    DownloadProgress progress;
    {
        juce::ScopedLock lock(progressLock);
        progress = currentProgress;
    }

    // Calculate overall progress
    if (progress.totalFiles > 0)
    {
        float filesProgress = (float)progress.completedFiles / (float)progress.totalFiles;
        float currentFileProgress = 0.0f;

        if (progress.currentFileTotal > 0)
        {
            currentFileProgress = (float)progress.currentFileDownloaded / (float)progress.currentFileTotal;
            currentFileProgress /= (float)progress.totalFiles; // Weight by total files
        }

        progress.overallProgress = filesProgress + currentFileProgress;
        progress.overallProgress = juce::jmin(1.0f, progress.overallProgress);
    }

    listeners.call([progress](Listener& l) { l.downloadProgressChanged(progress); });
}

void AudioDownloadManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void AudioDownloadManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}
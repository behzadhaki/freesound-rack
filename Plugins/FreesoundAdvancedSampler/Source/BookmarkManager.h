#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"

struct BookmarkInfo
{
    String freesoundId;
    String sampleName;
    String authorName;
    String licenseType;
    String searchQuery;
    String fileName;
    double duration;
    int fileSize;
    String bookmarkedAt;
    String freesoundUrl;
    
    BookmarkInfo() : duration(0.0), fileSize(0) {}
};

class BookmarkManager
{
public:
    BookmarkManager(const File& baseDirectory);
    ~BookmarkManager();
    
    // Bookmark operations
    bool addBookmark(const BookmarkInfo& bookmarkInfo);
    bool removeBookmark(const String& freesoundId);
    bool isBookmarked(const String& freesoundId) const;
    
    // Retrieval
    Array<BookmarkInfo> getAllBookmarks() const;
    BookmarkInfo getBookmark(const String& freesoundId) const;
    
    // File management
    File getBookmarksFile() const { return bookmarksFile; }
    void cleanupMissingFiles();
    
private:
    File baseDirectory;
    File bookmarksFile;
    
    void ensureFileExists();
    bool saveBookmarks(const Array<BookmarkInfo>& bookmarks);
    Array<BookmarkInfo> loadBookmarks() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkManager)
};
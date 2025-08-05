#include "BookmarkManager.h"

BookmarkManager::BookmarkManager(const File& baseDirectory)
    : baseDirectory(baseDirectory)
{
    bookmarksFile = baseDirectory.getChildFile("bookmarks.json");
    ensureFileExists();
}

BookmarkManager::~BookmarkManager()
{
}

void BookmarkManager::ensureFileExists()
{
    baseDirectory.createDirectory();
    
    if (!bookmarksFile.existsAsFile())
    {
        // Create initial JSON structure
        DynamicObject::Ptr rootObject = new DynamicObject();
        
        // File metadata
        DynamicObject::Ptr fileMetadata = new DynamicObject();
        fileMetadata->setProperty("version", "1.0");
        fileMetadata->setProperty("created_at", Time::getCurrentTime().toString(true, true));
        fileMetadata->setProperty("plugin_name", "Freesound Advanced Sampler");
        fileMetadata->setProperty("total_bookmarks", 0);
        rootObject->setProperty("file_metadata", var(fileMetadata.get()));
        
        // Empty bookmarks array
        rootObject->setProperty("bookmarks", Array<var>());
        
        String jsonString = JSON::toString(var(rootObject.get()), true);
        bookmarksFile.replaceWithText(jsonString);
    }
}

bool BookmarkManager::addBookmark(const BookmarkInfo& bookmarkInfo)
{
    if (bookmarkInfo.freesoundId.isEmpty())
        return false;
    
    Array<BookmarkInfo> bookmarks = loadBookmarks();
    
    // Check if already bookmarked
    for (const auto& bookmark : bookmarks)
    {
        if (bookmark.freesoundId == bookmarkInfo.freesoundId)
        {
            // Already bookmarked, don't add duplicate
            return true;
        }
    }
    
    // Add new bookmark
    bookmarks.add(bookmarkInfo);
    return saveBookmarks(bookmarks);
}

bool BookmarkManager::removeBookmark(const String& freesoundId)
{
    if (freesoundId.isEmpty())
        return false;
    
    Array<BookmarkInfo> bookmarks = loadBookmarks();
    
    for (int i = 0; i < bookmarks.size(); ++i)
    {
        if (bookmarks[i].freesoundId == freesoundId)
        {
            bookmarks.remove(i);
            return saveBookmarks(bookmarks);
        }
    }
    
    return false; // Not found
}

bool BookmarkManager::isBookmarked(const String& freesoundId) const
{
    if (freesoundId.isEmpty())
        return false;
    
    Array<BookmarkInfo> bookmarks = loadBookmarks();
    
    for (const auto& bookmark : bookmarks)
    {
        if (bookmark.freesoundId == freesoundId)
            return true;
    }
    
    return false;
}

Array<BookmarkInfo> BookmarkManager::getAllBookmarks() const
{
    return loadBookmarks();
}

BookmarkInfo BookmarkManager::getBookmark(const String& freesoundId) const
{
    Array<BookmarkInfo> bookmarks = loadBookmarks();
    
    for (const auto& bookmark : bookmarks)
    {
        if (bookmark.freesoundId == freesoundId)
            return bookmark;
    }
    
    return BookmarkInfo(); // Return empty if not found
}

Array<BookmarkInfo> BookmarkManager::loadBookmarks() const
{
    Array<BookmarkInfo> bookmarks;
    
    if (!bookmarksFile.existsAsFile())
        return bookmarks;
    
    String jsonText = bookmarksFile.loadFileAsString();
    var parsedJson = JSON::parse(jsonText);
    
    if (!parsedJson.isObject())
        return bookmarks;
    
    var bookmarksArray = parsedJson.getProperty("bookmarks", var());
    if (!bookmarksArray.isArray())
        return bookmarks;
    
    Array<var>* bookmarksVarArray = bookmarksArray.getArray();
    if (!bookmarksVarArray)
        return bookmarks;
    
    for (const auto& bookmarkVar : *bookmarksVarArray)
    {
        if (!bookmarkVar.isObject())
            continue;
        
        BookmarkInfo bookmark;
        bookmark.freesoundId = bookmarkVar.getProperty("freesound_id", "");
        bookmark.sampleName = bookmarkVar.getProperty("sample_name", "");
        bookmark.authorName = bookmarkVar.getProperty("author_name", "");
        bookmark.licenseType = bookmarkVar.getProperty("license_type", "");
        bookmark.searchQuery = bookmarkVar.getProperty("search_query", "");
        bookmark.fileName = bookmarkVar.getProperty("file_name", "");
        bookmark.duration = bookmarkVar.getProperty("duration", 0.0);
        bookmark.fileSize = bookmarkVar.getProperty("file_size", 0);
        bookmark.bookmarkedAt = bookmarkVar.getProperty("bookmarked_at", "");
        bookmark.freesoundUrl = bookmarkVar.getProperty("freesound_url", "");
        bookmark.tags = bookmarkVar.getProperty("tags", "");
        bookmark.description = bookmarkVar.getProperty("description", "");
        DBG("loaded bookmar tags: " + bookmark.tags);
        bookmarks.add(bookmark);
    }
    
    return bookmarks;
}

bool BookmarkManager::saveBookmarks(const Array<BookmarkInfo>& bookmarks)
{
    // Read existing JSON to preserve metadata
    String jsonText = bookmarksFile.loadFileAsString();
    var parsedJson = JSON::parse(jsonText);
    
    if (!parsedJson.isObject())
    {
        // Create new structure if parsing failed
        DynamicObject::Ptr rootObject = new DynamicObject();
        parsedJson = var(rootObject.get());
    }
    
    DynamicObject::Ptr rootObj = parsedJson.getDynamicObject();
    if (!rootObj)
        return false;
    
    // Update file metadata
    var fileMetadata = parsedJson.getProperty("file_metadata", var());
    DynamicObject::Ptr fileMetadataObj;
    
    if (!fileMetadata.isObject())
    {
        fileMetadataObj = new DynamicObject();
        fileMetadata = var(fileMetadataObj.get());
    }
    else
    {
        fileMetadataObj = fileMetadata.getDynamicObject();
    }
    
    if (fileMetadataObj)
    {
        fileMetadataObj->setProperty("last_modified", Time::getCurrentTime().toString(true, true));
        fileMetadataObj->setProperty("total_bookmarks", bookmarks.size());
        rootObj->setProperty("file_metadata", fileMetadata);
    }
    
    // Create bookmarks array
    Array<var> bookmarksArray;
    
    for (const auto& bookmark : bookmarks)
    {
        DynamicObject::Ptr bookmarkObj = new DynamicObject();
        bookmarkObj->setProperty("freesound_id", bookmark.freesoundId);
        bookmarkObj->setProperty("sample_name", bookmark.sampleName);
        bookmarkObj->setProperty("author_name", bookmark.authorName);
        bookmarkObj->setProperty("license_type", bookmark.licenseType);
        bookmarkObj->setProperty("search_query", bookmark.searchQuery);
        bookmarkObj->setProperty("file_name", bookmark.fileName);
        bookmarkObj->setProperty("duration", bookmark.duration);
        bookmarkObj->setProperty("file_size", bookmark.fileSize);
        bookmarkObj->setProperty("bookmarked_at", bookmark.bookmarkedAt);
        bookmarkObj->setProperty("freesound_url", bookmark.freesoundUrl);
        bookmarkObj->setProperty("tags", bookmark.tags);
        bookmarkObj->setProperty("description", bookmark.description);
        
        bookmarksArray.add(var(bookmarkObj.get()));
    }
    
    rootObj->setProperty("bookmarks", bookmarksArray);
    
    // Write to file
    String newJsonString = JSON::toString(parsedJson, true);
    return bookmarksFile.replaceWithText(newJsonString);
}

void BookmarkManager::cleanupMissingFiles()
{
    Array<BookmarkInfo> bookmarks = loadBookmarks();
    Array<BookmarkInfo> validBookmarks;
    
    for (const auto& bookmark : bookmarks)
    {
        File sampleFile = baseDirectory.getChildFile("samples").getChildFile(bookmark.fileName);
        if (sampleFile.existsAsFile())
        {
            validBookmarks.add(bookmark);
        }
    }
    
    if (validBookmarks.size() != bookmarks.size())
    {
        saveBookmarks(validBookmarks);
    }
}
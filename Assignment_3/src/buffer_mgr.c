#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>
#include <limits.h>

// Structure representing a page frame within the buffer pool.
typedef struct Page {
    SM_PageHandle data; // Data of the page.
    PageNumber pageNum; // Page number in the file.
    int dirtyBit; // Flag indicating if the page has been modified in the buffer, but not yet written back to disk.
    int fixCount; // Number of clients currently using this page.
    int hitNum; // Number of times the page has been referenced (for LRU and Clock replacement strategies).
    int refNum; // Number of times the page has been referenced (for LFU replacement strategy).
} PageFrame;

// Pointer for the least frequently used page replacement strategy.
int lfuPointer;

// Size of the buffer pool.
int bufferSize;

// Index of the last page in the buffer pool.
int rearIndex;

// Pointer for the clock page replacement strategy.
int clockPointer;

// Number of pages written back to disk.
int writeCount;

// Number of page hits in the buffer pool.
int hit;

// Function that writes a page frame back to disk.
void writeToDisk(BM_BufferPool *const bm, PageFrame *pageFrame);

// Function that replaces the data in a page frame with the data from another page.
void replacePageFrameData(PageFrame *pageFrame, PageFrame *page);

// Function that checks if a page frame can be replaced (not currently used by any client and not dirty).
bool isReplaceable(PageFrame *pageFrame);

// Function that gets the index of the next frame to be used in the buffer pool.
int getNextFrameIndex();

// Function that initializes an array of page frames.
PageFrame *initializePageFrames(const int numPages);

// Function that initializes auxiliary variables used in buffer pool management.
void initializeAuxiliaryVariables();

// Function that checks if there are any pinned (currently in use) pages in the buffer pool.
bool hasPinnedPages(PageFrame *pageFrames);

// Function that deallocates memory for an array of page frames.
void deallocatePageFrames(PageFrame **pageFrames);

// Function that checks if a page is dirty and not currently in use.
bool isPageDirtyAndUnfixed(PageFrame *pageFrame);

// Function that writes a page frame back to disk.
void writePageToDisk(SM_FileHandle *fh, PageFrame *pageFrame);

// This is a function to replace the data in one page frame with the data from another page frame.
void replacePageFrameData(PageFrame *pageFrame, PageFrame *page)
{
    // Assign page number from the input page to the existing page frame.
    pageFrame->pageNum = page->pageNum;

    // Assign data from the input page to the existing page frame.
    pageFrame->data = page->data;

    // Assign dirty bit from the input page to the existing page frame.
    pageFrame->dirtyBit = page->dirtyBit;

    // Assign fix count from the input page to the existing page frame.
    pageFrame->fixCount = page->fixCount;
}

// This function checks if a page frame can be replaced based on its hit number.
bool isReplaceable(PageFrame *pageFrame)
{
    // If the hit number of the page frame is zero, it means it's not recently used.
    // Hence, it can be replaced and the function returns true.
    if (pageFrame->hitNum == 0)
    {
        return true;
    }

    // If the hit number of the page frame is not zero, it means it's recently used.
    // In this case, we reset the hit number to zero (indicating it's not recently used anymore)
    // and return false as it cannot be replaced now.
    pageFrame->hitNum = 0;
    return false;
}

// This function calculates and returns the index of the next frame in a circular buffer.
int getNextFrameIndex()
{
    // The next frame index is calculated as the current position of the clock pointer (clockPointer)
    // incremented by one and then taken modulo the size of the buffer (bufferSize).
    // This ensures that the index wraps around to the start of the buffer once it reaches the end.
    return (clockPointer + 1) % bufferSize;
}

// This function initializes an array of PageFrames.
PageFrame *initializePageFrames(const int numPages)
{
    // Allocate memory for the page frames
    PageFrame *pageFrames = (PageFrame *)calloc(numPages, sizeof(PageFrame));

    // Initialize all pages in the buffer pool
    for (int i = 0; i < numPages; i++)
    {
        pageFrames[i].pageNum = -1; // Use -1 to represent an invalid page number
    }

    return pageFrames;
}

// This function initializes the auxiliary variables used in the program.
void initializeAuxiliaryVariables()
{
    // Reset writeCount to 0. This variable might be used to count the number of write operations
    // performed in your program, or to track a similar metric.
    writeCount = 0;

    // Reset clockPointer to 0. This variable is typically used in a clock replacement algorithm (a page
    // replacement algorithm), where it points to the next candidate frame for replacement.
    clockPointer = 0;

    // Reset lfuPointer to 0. This variable might be used in a Least Frequently Used (LFU) cache
    // algorithm, where it points to the next candidate frame for eviction based on the frequency of
    // access.
    lfuPointer = 0;
}

// This function checks if there are any pinned pages in the buffer pool.
bool hasPinnedPages(PageFrame *pageFrames)
{
    for (int i = 0; i < bufferSize; i++)
    {
        // Return true if a page frame is still pinned
        if (pageFrames[i].fixCount != 0)
        {
            return true;
        }
    }

    return false;
}

// This function deallocates memory allocated to an array of page frames.
void deallocatePageFrames(PageFrame **pageFrames)
{
    free(*pageFrames);  // Free the memory allocated to the pageFrames.
    *pageFrames = NULL; // Set the pointer to NULL to avoid dangling pointer issues after deallocation.
}

// This function checks if a page frame is dirty and unfixed.
bool isPageDirtyAndUnfixed(PageFrame *pageFrame)
{
    // Returns true if the page frame's fix count is zero (indicating it's unfixed)
    // and its dirty bit is set (indicating it's dirty), otherwise returns false.
    return pageFrame->fixCount == 0 && pageFrame->dirtyBit == 1;
}

// This function writes the data from a PageFrame to disk using the writeBlock function.
void writePageToDisk(SM_FileHandle *fh, PageFrame *pageFrame)
{
    // Write the data block to the page file on disk
    writeBlock(pageFrame->pageNum, fh, pageFrame->data);

    // Mark the page as not dirty
    pageFrame->dirtyBit = 0;

    // Increment the writeCount to record the disk write
    writeCount++;
}

// This function writes the data from a PageFrame to disk using the openPageFile and writeBlock functions.
void writeToDisk(BM_BufferPool *const bm, PageFrame *pageFrame)
{
    SM_FileHandle fh;
    openPageFile(bm->pageFile, &fh);
    writeBlock(pageFrame->pageNum, &fh, pageFrame->data);

    // Increment the writeCount to record the disk write
    writeCount++;
}

// This function implements a First In First Out (FIFO) page replacement algorithm for a buffer pool.
extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
    // Cast the buffer pool's management data to a PageFrame pointer
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Calculate the index of the front of the queue (the next frame to be replaced)
    int frontIndex = rearIndex % bufferSize;

    // Main loop to find a frame for replacement
    while (true)
    {
        // Frame is not currently being used
        if (pageFrame[frontIndex].fixCount == 0)
        {
            // Check if the frame has been modified and write it back to disk if necessary
            if (pageFrame[frontIndex].dirtyBit == 1)
            {
                writeToDisk(bm, &pageFrame[frontIndex]);
            }

            // Replace the content of the page frame with the new page's content
            replacePageFrameData(&pageFrame[frontIndex], page);

            // We have replaced the frame, so we can exit the loop now
            break;
        }

        // Move to the next frame if the current frame is being used
        frontIndex = (frontIndex + 1) % bufferSize;
    }
}

// It scans through all page frames to find the least recently used (LRU) frame based on the lowest hitNum value.
extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{
    // Get the size of the buffer
    int bufferSize = bm->numPages;
    // loading the pageFrame point with buffer pool's management data
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Initializing variables to hold the index and hit number of LRU
    int leastRecentIndex = -1;
    int leastRecentNum = INT_MAX;
    int highestRecentNum = INT_MIN;

    // Find the least recently used page frame and highest hitNum
    for (int i = 0; i < bufferSize; i++)
    {
        // If the current frame is not fixed and has a lower hit number than the current least recent page frame
        if (pageFrame[i].fixCount == 0 && pageFrame[i].hitNum <= leastRecentNum)
        {
            leastRecentNum = pageFrame[i].hitNum;
            leastRecentIndex = i;
        }
        if (pageFrame[i].hitNum > highestRecentNum)
        {
            highestRecentNum = pageFrame[i].hitNum;
        }
    }

    // If no frame is available, return early
    if (leastRecentIndex == -1)
    {
        return;
    }

    // If the page frame has been modified, write it to disk
    if (pageFrame[leastRecentIndex].dirtyBit == 1)
    {
        writeToDisk(bm, &pageFrame[leastRecentIndex]);
    }

    // Replace the least recently used page frame with the new page
    pageFrame[leastRecentIndex] = *page;

    // Update the hitNum of the new page frame to be the highest + 1
    pageFrame[leastRecentIndex].hitNum = highestRecentNum + 1;
}

// Defining CLOCK function
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page)
{
    // Cast the buffer pool's management data to a PageFrame pointer
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    // Continue until we find a replaceable frame
    while (true)
    {
        // If the current frame can be replaced, break the loop
        if (isReplaceable(&pageFrames[clockPointer]))
        {
            break;
        }

        // Move the clock pointer to the next frame
        clockPointer = getNextFrameIndex();
    }

    // If the frame is dirty, write it back to disk
    if (pageFrames[clockPointer].dirtyBit == 1)
    {
        writeToDisk(bm, &pageFrames[clockPointer]);
    }

    // Replace the current frame with the new page
    pageFrames[clockPointer] = *page;

    // Advance the clock pointer to the next frame
    clockPointer = getNextFrameIndex();
}

// This function initializes a buffer pool data structure and related state.
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    // Initialize auxiliary variables
    initializeAuxiliaryVariables();

    // Set the buffer size
    bufferSize = numPages;

    // Initialize buffer management properties
    bm->mgmtData = initializePageFrames(numPages);
    bm->pageFile = (char *)pageFileName;
    bm->strategy = strategy;
    bm->numPages = numPages;

    return RC_OK;
}

// It forces any dirty pages to flush back to disk before shutdown.
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    // Flush all dirty pages back to disk
    forceFlushPool(bm);

    // Return an error if there are any pinned pages in the buffer pool
    if (hasPinnedPages(pageFrames))
    {
        return RC_PINNED_PAGES_IN_BUFFER;
    }

    // Deallocate the memory for the page frames
    deallocatePageFrames(&pageFrames);

    // Reset the buffer pool's management data
    bm->mgmtData = NULL;

    return RC_OK;
}

// It forces any dirty pages still in memory to flush back to disk before shutdown.
extern RC forceFlushPool(BM_BufferPool *const bm)
{
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    // Open the page file on disk
    SM_FileHandle fh;
    openPageFile(bm->pageFile, &fh);

    for (int i = 0; i < bufferSize; i++)
    {
        // If the page is dirty and not fixed, write it back to disk
        if (isPageDirtyAndUnfixed(&pageFrames[i]))
        {
            writePageToDisk(&fh, &pageFrames[i]);
        }
    }

    // Close the file
    closePageFile(&fh);

    return RC_OK;
}

int findPageInBuffer(PageFrame *pageFrames, PageNumber pageNum);
//
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    int pageIndex = findPageInBuffer(pageFrames, page->pageNum);

    if (pageIndex != -1)
    {
        pageFrames[pageIndex].dirtyBit = 1;
        return RC_OK;
    }

    return RC_ERROR;
}

int findPageInBuffer(PageFrame *pageFrames, PageNumber pageNum)
{
    for (int i = 0; i < bufferSize; i++)
    {
        if (pageFrames[i].pageNum == pageNum)
        {
            return i;
        }
    }

    return -1; // If page not found in buffer
}

void unpinPageIfPinned(PageFrame *pageFrame);
// This function unpins a page in the buffer pool if it's pinnned.
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData; // Cast the management data of the buffer pool to an array of PageFrames.

    int pageIndex = findPageInBuffer(pageFrames, page->pageNum); // Find the index of the page in the buffer pool.

    if (pageIndex != -1) // If the page is found in the buffer pool.
    {
        unpinPageIfPinned(&pageFrames[pageIndex]); // Unpin the page if it's pinned.
        return RC_OK;                              // Return success status.
    }

    return RC_ERROR; // Return error status if the page is not found in the buffer pool.
}

// This function decrements the fix count of a page frame if it's pinned.
void unpinPageIfPinned(PageFrame *pageFrame)
{
    if (pageFrame->fixCount > 0) // If the page frame is pinned (indicated by a fix count greater than zero).
    {
        pageFrame->fixCount--; // Decrement the fix count.
    }
}

// This function writes the contents of the modified pages back to the page file on disk
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    int pageIndex = findPageInBuffer(pageFrames, page->pageNum);

    if (pageIndex == -1)
    {
        // Page not found in the buffer pool
        return RC_ERROR;
    }

    // Open the page file on disk
    SM_FileHandle fh;
    if (openPageFile(bm->pageFile, &fh) != RC_OK)
    {
        // Failed to open page file
        return RC_FILE_NOT_FOUND;
    }

    // Force the page to disk
    if (writeBlock(pageFrames[pageIndex].pageNum, &fh, pageFrames[pageIndex].data) != RC_OK)
    {
        // Failed to write page to disk
        return RC_WRITE_FAILED;
    }

    // Mark the page as clean and increment the write count
    pageFrames[pageIndex].dirtyBit = 0;
    writeCount++;

    // Close the page file
    closePageFile(&fh);

    return RC_OK;
}

// This function handles the scenario of reading the first page into the buffer pool.
extern RC handleFirstPage(BM_BufferPool *const bm, BM_PageHandle *const page,
                          const PageNumber pageNum, PageFrame *pageFrame)
{
    // Create a pointer to the first page frame
    PageFrame *firstPageFrame = &pageFrame[0];

    // Open the file
    SM_FileHandle fh;
    openPageFile(bm->pageFile, &fh);

    // Allocate memory for the first page frame's data
    firstPageFrame->data = (SM_PageHandle)malloc(PAGE_SIZE);

    // Ensure the file has enough pages
    ensureCapacity(pageNum, &fh);

    // Read the specified block into the first page frame's data
    readBlock(pageNum, &fh, firstPageFrame->data);

    // Set the properties of the first page frame
    firstPageFrame->pageNum = pageNum;
    firstPageFrame->fixCount++;
    rearIndex = hit = 0;
    firstPageFrame->hitNum = hit;
    firstPageFrame->refNum = 0;

    // Set the properties of the page handle
    page->pageNum = pageNum;
    page->data = firstPageFrame->data;

    return RC_OK;
}

// This function handles the scenario of a page already being in memory (buffer pool) and being referenced again.
extern RC handlePageInMemory(BM_BufferPool *const bm, BM_PageHandle *const page,
                             const PageNumber pageNum, PageFrame *pageFrame, int frameIndex)
{

    // Increment fix count and move clock pointer
    pageFrame[frameIndex].fixCount++;
    clockPointer++;

    // Update hit number or reference number based on replacement strategy
    switch (bm->strategy)
    {
    case RS_LRU:
        hit++;
        pageFrame[frameIndex].hitNum = hit;
        break;
    case RS_CLOCK:
        pageFrame[frameIndex].hitNum = 1;
        break;
    case RS_LFU:
        pageFrame[frameIndex].refNum++;
        break;
    default:
        return RC_ERROR;
    }

    // Update page handle
    page->pageNum = pageNum;
    page->data = pageFrame[frameIndex].data;

    return RC_OK;
}

// This function is called when the buffer pool is full and a new page needs to be loaded into memory.
extern RC handleBufferFull(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum, PageFrame *pageFrame, int i)
{
    // Initialize a new page frame object
    PageFrame *newPageFrame = &pageFrame[i];

    // Open the file and read the page into the new page frame's data
    SM_FileHandle fh;

    newPageFrame->data = (SM_PageHandle)malloc(PAGE_SIZE);
    openPageFile(bm->pageFile, &fh);
    readBlock(pageNum, &fh, newPageFrame->data);

    // Set the other properties of the new page frame
    newPageFrame->fixCount = 1;
    newPageFrame->refNum = 0;
    newPageFrame->pageNum = pageNum;

    // Increase index and hit
    hit++;
    rearIndex++;

    // Set the hit number based on the replacement strategy
    newPageFrame->hitNum = (bm->strategy == RS_LRU) ? hit : ((bm->strategy == RS_CLOCK) ? 1 : 0);

    // Set the properties of the page handle
    page->pageNum = pageNum;
    page->data = newPageFrame->data;
    return RC_OK;
}

// It checks if the requested page is already in the buffer pool and handles it if present.
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{

    int x;
    // loading pageFrame with bufferpool data
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    x = pageFrame[0].pageNum;

    // pinning the first page only if the buffer pool is empty
    if (x == -1)
    {
        handleFirstPage(bm, page, pageNum, pageFrame);
        return RC_OK;
    }

    bool isBufferFull = true;
    for (int i = 0; i < bufferSize; i++)
    {
        // If page is in memory
        if (pageFrame[i].pageNum == pageNum)
        {
            handlePageInMemory(bm, page, pageNum, pageFrame, i);
            isBufferFull = false;
            break;
        }
        // If the buffer has an empty slot
        else if (pageFrame[i].pageNum == -1)
        {
            handleBufferFull(bm, page, pageNum, pageFrame, i);
            isBufferFull = false;
            break;
        }
    }

    if (isBufferFull)
    {
        // Allocate and initialize a new page frame
        PageFrame *newPage = calloc(1, sizeof(PageFrame));

        // Open the page file
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);

        // Allocate and read data into the new page frame
        newPage->data = calloc(PAGE_SIZE, sizeof(char));
        readBlock(pageNum, &fh, newPage->data);

        // Initialize the new page frame properties
        newPage->dirtyBit = 0;
        newPage->pageNum = pageNum;
        newPage->refNum = 0;
        newPage->fixCount = 1;

        // Update index and hit count
        rearIndex++;
        hit++;

        // Set hit number based on buffer strategy
        newPage->hitNum = (bm->strategy == RS_LRU) ? hit : ((bm->strategy == RS_CLOCK) ? 1 : 0);

        // Update the page properties
        page->pageNum = pageNum;
        page->data = newPage->data;

        void (*strategyFunction)(BM_BufferPool *const, PageFrame *const) = NULL;
        switch (bm->strategy)
        {
        case RS_FIFO:
            strategyFunction = FIFO;
            break;
        case RS_LRU:
            strategyFunction = LRU;
            break;
        case RS_CLOCK:
            strategyFunction = CLOCK;
            break;
        // case RS_LFU: strategyFunction = LFU; break;
        default:
            printf("\nAlgorithm Not Implemented\n");
            break;
        }
        if (strategyFunction)
            strategyFunction(bm, newPage);
    }
    return RC_OK;
}

// This function returns an array of page numbers.
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    // Allocate memory for frameContents and initialize with NO_PAGE
    PageNumber *frameContents = (PageNumber *)calloc(bufferSize, sizeof(PageNumber));

    // Assign NO_PAGE for all elements
    for (int i = 0; i < bufferSize; i++)
        frameContents[i] = NO_PAGE;

    // Get the management data from buffer pool
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Update frameContents with the page numbers of the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++)
    {
        // If the page number is not -1, then the page is in the buffer pool
        if (pageFrame[i].pageNum != -1)
        {
            frameContents[i] = pageFrame[i].pageNum;
        }
    }

    return frameContents;
}

// This function returns an array of bools, each element represents the dirtyBit of the respective page.
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
    // Allocate memory for dirtyFlags
    bool *dirtyFlags = (bool *)malloc(bufferSize * sizeof(bool));

    // Get the management data from buffer pool
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Set dirtyFlags based on the dirtyBit of the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++)
    {
        dirtyFlags[i] = pageFrame[i].dirtyBit == 1;
    }

    return dirtyFlags;
}

// This function returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame.
extern int *getFixCounts(BM_BufferPool *const bm)
{
    // Allocate memory for fixCounts
    int *fixCounts = (int *)malloc(bufferSize * sizeof(int));

    // Get the management data from buffer pool
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Initialize fixCounts based on the fixCount of the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++)
    {
        // Assign 0 if fixCount is -1, otherwise assign fixCount
        fixCounts[i] = pageFrame[i].fixCount == -1 ? 0 : pageFrame[i].fixCount;
    }

    return fixCounts;
}

// Directly returns the number of read IOs based on the rearIndex
extern int getNumReadIO(BM_BufferPool *const bm)
{
    // Calculate the number of read IOs directly
    return rearIndex >= 0 ? rearIndex + 1 : 0;
}

// Directly returns the global writeCount variable
extern int getNumWriteIO(BM_BufferPool *const bm)
{
    // Return the number of write IOs directly
    return writeCount > 0 ? writeCount : 0;
}

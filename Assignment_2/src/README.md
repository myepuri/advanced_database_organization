HOW TO RUN THE SCRIPT
=========================

1) Go to Project root (Assignment_2) using Terminal.

2) Type "make" to compile all project files. 

3) Type "./test1" to run "test_assign2_1.c" file.

4) Type "./test2" to run "test_assign2_2.c" file.

5)Type "make clean" to delete old compiled .o files.


1. BUFFER POOL FUNCTIONS
===========================

initBufferPool(...)
- This function initializes a buffer pool data structure and related state.
- It sets the buffer size from the numPages parameter.
- It initializes the page frames data structure to manage page frames in memory. 
- It initializes the buffer pool properties like page file name, replacement strategy, and number of pages. 
- It also initializes any auxiliary variables needed for replacement algorithms.
- The function returns RC_OK if initialization is successful.
- This function prepares the buffer pool and related data structures before use for caching pages in memory.

shutdownBufferPool(...)
- It forces any dirty pages to flush back to disk before shutdown.
- It checks for any pinned pages still in use and returns error.
- It deallocates memory used for page frames and resets pool metadata.
- This ensures all data is persisted and resources are cleaned before pool destruction.

forceFlushPool(...)
- It forces any dirty pages still in memory to flush back to disk before shutdown. 
- It opens the page file, then iterates each page frame checking for dirty and unpinned pages.
- For any dirty and unpinned pages, it calls writePageToDisk to write the data to disk.
- It closes the file after all dirty pages have been flushed. This ensures data persistence before shutdown.


2. PAGE MANAGEMENT FUNCTIONS
==========================
pinPage(...)
- It checks if the requested page is already in the buffer pool and handles it if present.
- If an empty slot exists, it reads the page from disk into memory.
- If buffer is full, it allocates a new page frame, reads page and initializes its properties. 
- Based on strategy, it calls the corresponding replacement algorithm to select a victim frame.
- This function implements the core logic to retrieve a requested page, handling cache hits, empty slots or full pool requiring replacement.
- It either finds the page already cached, loads a new page or evicts one using the policy to pin the required page in memory.
- By calling different algorithms, this provides a unified way to implement various page replacement strategies.

unpinPage(...)
- Finds index of given page using findPageInBuffer
- Calls unpinPageIfPinned to decrement fix count if page is pinned
- Returns OK if found, ERROR otherwise

markDirty(...)
- Finds the index of given page in page frames array using page number. 
- If found, it sets the dirty bit for that page frame to mark it dirty.
- Returns OK, else returns ERROR if page not found in buffer pool.

forcePage(....)
- This function forces a specific page from the buffer pool out to disk. 
- It first finds the index of the given page in the page frames array using the page number.
- If found, it opens the page file, writes the page data block to disk at the corresponding page number location.
- After writing, it marks the page as clean in memory by resetting the dirty bit and increments the write count. This persists any modifications to that page.
- It returns an error code if the page is not found in the pool, if the file open or write operation fails for any reason.
- By forcing a particular page to disk, this function allows manually flushing a dirty page without replacing it from the buffer pool. This may be useful during certain checkpoint or sync operations.

3. STATISTICS FUNCTIONS
===========================

getFrameContents(...)
- This function returns the page numbers of all pages currently stored in the buffer pool.
- It first allocates an array to store the page numbers and initializes all values to NO_PAGE.
- It then loops through the page frames structure stored in the buffer pool and copies the page number from each valid frame (page number not -1) into the return array.
- The completed array containing all page numbers in the buffer pool is returned to the caller.
- This provides a way to easily retrieve all pages currently cached without needing to iterate the entire frame array structure. It offers a snapshot of the buffer pool contents.

getDirtyFlags(...)
- This function returns an array indicating which pages in the buffer pool are dirty.
- It allocates an array of bools and initializes all values to false by default.
- It then loops through each page frame and sets the corresponding bool in the return array to true if the frame's dirtyBit is set.
- This provides an easy way for clients to determine the dirty state of cached pages without additional lookups or flag checks.
- The completed array is returned, with a value of true corresponding to each dirty page currently in the buffer pool.
- Together with getFrameContents, this allows easily retrieving both page numbers and dirty states with a single call for status/reporting purposes.

getFixCounts(...) 
- Returns an array containing the fix count of each page in the buffer pool
- Allocates and initializes the array
- Loops through page frames, assigning 0 if fix count is -1, else assigns value

getNumReadIO(...)
- Directly returns the number of read IOs based on the rearIndex
- rearIndex tracks number of pages read in so far

getNumWriteIO(...)
- Directly returns the global writeCount variable
- writeCount tracks number of pages written out for replacement


4. PAGE REPLACEMENT ALGORITHM FUNCTIONS
=========================================

FIFO(...)
- This function implements a First In First Out (FIFO) page replacement algorithm for a buffer pool.
- It finds the next page frame to replace by tracking the "frontIndex" which points to the oldest page in the buffer pool.
- Before replacing a page frame, it first checks if the page is dirty by looking at the dirtyBit flag. If dirty, it writes the page back to disk.
- Once an unused frame is found, it replaces the page frame data with the new page passed in and exits the function. This completes one iteration of page replacement.

LRU(...)
- It scans through all page frames to find the least recently used (LRU) frame based on the lowest hitNum value.
- If a frame is dirty, it writes it back to disk before replacement.
- It replaces the LRU frame with the new page by copying over the page data.
- The hitNum of the new page is set to the highest existing hitNum + 1 to track future access. This implements the LRU policy by always choosing the page with the lowest hitNum for replacement.

CLOCK(...)
- It implements a circular queue (clock) approach to track page frames. 
- It checks each frame starting from the clock pointer to see if it can be replaced based on its hit number.
- If a frame is dirty, it writes it back to disk before replacement. 
- After replacing a frame, it increments the clock pointer and continues circulation through the queue. This ensures LRU approximation by progressively marking pages as unused.



5. HELPER FUNCTIONS
=====================

replacePageFrameData(...): 
- This function is used to copy the contents of one PageFrame into another. In detail:
- It updates the page number in the existing PageFrame with the page number from the input PageFrame.
- It updates the data in the existing PageFrame with the data from the input PageFrame.
- It updates the dirty bit in the existing PageFrame with the dirty bit from the input PageFrame.
- It updates the fix count in the existing PageFrame with the fix count from the input PageFrame.

isReplaceable(...): 
- This function checks if a PageFrame can be replaced based on its hit number.
- If the hit number is zero (meaning the page hasn't been recently used), it returns true indicating that the PageFrame can be replaced.
- If the hit number is not zero (meaning the page has been recently used), it resets the hit number to zero and returns false indicating that the PageFrame cannot be replaced at the moment.

writeToDisk(...): 
- This function writes the data from a PageFrame to disk using the openPageFile and writeBlock functions. 
- It then increments the writeCount.

getNextFrameIndex(...): 
- This function calculates the index of the next frame in a circular buffer. 
- It increments the current clock pointer by one and then takes the modulus with the buffer size. 
- This ensures that the index wraps around to the start of the buffer once it reaches the end.

initializePageFrames(...): 
- This function initializes an array of PageFrames.
- It first allocates memory for the PageFrame array.
- It then sets the page number of all pages in the array to -1, indicating an invalid page number.

initializeAuxiliaryVariables(...): 
- This function resets several auxiliary variables (writeCount, clockPointer, lfuPointer) to zero. 
- These variables are typically used for tracking metrics or pointers in page replacement algorithms.

hasPinnedPages(...): 
- This function checks if there are any pinned pages in the buffer pool. 
- It returns true if it finds a PageFrame with a non-zero fix count.

deallocatePageFrames(...): 
- This function deallocates the memory allocated to the PageFrame array and sets the pointer to NULL to avoid dangling pointer issues.

isPageDirtyAndUnfixed(...): 
- This function checks if a PageFrame is both dirty and unfixed. 
- It returns true if the PageFrame's fix count is zero and its dirty bit is set.

writePageToDisk(...): 
- This function writes the data from a PageFrame to disk using the writeBlock function. 
- It then sets the PageFrame's dirty bit to zero and increments the writeCount.

unpinPageIfPinned(..):
- This  is used to unpin a page frame if it is currently pinned in memory.
- It takes a pointer to a PageFrame struct as input.
- It checks if the page frame's fixCount is greater than 0, indicating it is pinned.
- If so, it decrements the fixCount by 1 to unpin it.
- This allows a pinned page to be unpinned and made eligible for replacement again.

handleFirstPage(...):
-This function handles the scenario of reading the first page into the buffer pool.
-It first creates a pointer to the first PageFrame and opens the file associated with the BM_BufferPool.
-It then allocates memory for the first PageFrame's data.
-It ensures that the file has enough pages using the ensureCapacity function.
-It reads the specified block from the file into the first PageFrame's data using the readBlock function.
-It sets the properties of the first PageFrame, including the page number, fix count, hit number, and reference number.
-It also sets the properties of the BM_PageHandle to reflect the page that was just read into the buffer pool.
-It finally returns RC_OK to indicate success.

handlePageInMemory(...):
-This function handles the scenario of a page already being in memory (buffer pool) and being referenced again.
-It increments the fix count of the PageFrame being referenced and moves the clock pointer.
-Depending on the replacement strategy (LRU, Clock, LFU), it updates the hit number or reference number of the PageFrame.
-It updates the BM_PageHandle to reflect the page that was just referenced.
-It finally returns RC_OK to indicate success.

handleBufferFull(...):
-This function is called when the buffer pool is full and a new page needs to be loaded into memory.
-It first initializes a new PageFrame at the index i in the pageFrame array.
-It then opens the file associated with the buffer pool and reads the specified page into the new PageFrame's data.
-It sets the fix count of the new PageFrame to 1 and the reference number to 0.
-It also sets the page number of the new PageFrame to the specified page number.
-It increments the hit and rearIndex variables, which are used for tracking the most recently used page and the last page in the buffer, respectively.
-Depending on the replacement strategy (LRU or Clock), it sets the hit number of the new PageFrame accordingly.
-It updates the BM_PageHandle to reflect the page that was just loaded into the buffer pool.
-Finally, it returns RC_OK to indicate success.



Bonus
===============
We have implemented and tested CLOCK page replacement successfully which can be seen when you run "./test2".
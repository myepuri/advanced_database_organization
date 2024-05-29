# RUNNING THE SCRIPT

- Go to Project root (assign1) using Terminal.
- Type "make" to compile all project files including "test_assign1_1.c" file
- Type "./test_assign1" to run "test_assign1_1.c" file.
- Type "make clean" to delete old compiled .o files.

# File related methods

## extern void initStorageManager (void)

• Initializes the storage manager by setting the page variable to NULL, indicating that no page is currently being managed.
• Prepares the storage manager to handle requests for page management, including creating new pages, managing existing pages, and dealing with page faults.

## extern RC createPageFile(char \*fileName)

-It takes a file name as input and attempts to open the file for writing in binary mode.
-It returns status codes to indicate the outcome: RC_OK for success, RC_FILE_NOT_FOUND if the file can't be opened, and RC_WRITE_FAILED if not enough data could be written.
-It writes PAGE_SIZE bytes to the file by repetitively calling fwrite() in a loop.
-The buffer used for writing is initialized to all zeros via memset().
-The file is closed before returning from the function regardless of success or failure in writing.

## extern RC openPageFile(char *fileName, SM_FileHandle *fHandle)

-It takes in a file name and a file handle pointer as parameters.
-It initializes the file handle by setting the file name, page pos, total pages, and mgmtInfo fields.
-It opens the file in read/write binary mode using fopen().
-It stores the FILE pointer returned by fopen() in the mgmtInfo field of the file handle.
-It uses fseek() and ftell() to get the file size and calculate the total number of pages based on a pre-defined PAGE_SIZE.
-It returns RC_OK on success, or RC_FILE_NOT_FOUND if the file can't be opened.
-The open file handle can then be passed to other functions to perform operations like reading/writing pages.

## extern RC closePageFile(SM_FileHandle \*fHandle)

-closePageFile() takes a pointer to a SM_FileHandle struct as input. This contains metadata about an open file.
-It first checks that the file handle is initialized by checking if the mgmtInfo field is NULL. If so, it returns RC_FILE_HANDLE_NOT_INIT error.
-It gets the FILE pointer from the mgmtInfo field of the handle struct. This is the standard C file pointer.
-It calls fclose() to close the FILE, closing the underlying OS file handle.
-It resets the fields of the SM_FileHandle struct to default values: NULL mgmtInfo, 0 totalNumPages and curPagePos. This invalidates the struct.
-Returns RC_OK if successful in closing the file. The handle is now invalid and cannot be used again.

## extern RC destroyPageFile(char fileName)

-destroyPageFile() takes a char\* fileName as input, which is the name of the file to delete.
-It calls the standard C remove() function, passing the filename.
-remove() attempts to delete the file from the filesystem.
-If remove() returns non-zero, it indicates an error occurred deleting the file.
-In case of error, it returns RC_FILE_NOT_FOUND to indicate the file could not be found/deleted.
-If remove() succeeds and returns 0, RC_OK is returned to indicate successful deletion.
-This function deletes the specified file from disk if it exists. The return code indicates whether the file was deleted or if any error occurred.

## extern RC readBlock (int pageNum, SM_FileHandle fHandle, SM_PageHandle memPage)

-readBlock() takes a page number, file handle, and memory page handle as inputs. It reads the given page from file into memory.
-It first validates that the file handle is initialized and the page number is valid.
-It calculates the file offset of the page using the page number and PAGE_SIZE constants.
-It seeks to the calculated offset in the file using fseek().
-It reads PAGE_SIZE bytes from the file into the memory page using fread().
-It updates the current page position in the file handle and returns RC_OK if successful.
-If any validation fails or the fread() has an error, it returns the appropriate error code.
-The function allows reading a specific page from a file into a memory buffer.

## extern int getBlockPos(SM_FileHandle \*fHandle)

-getBlockPos() takes a file handle pointer fHandle as a parameter.
-It first checks if the file handle is NULL or the mgmtInfo field inside the handle is NULL. If so, it returns RC_FILE_HANDLE_NOT_INIT error code indicating the handle is -invalid.
-To get the block position, it uses the curPagePos field from the file handle, which contains the current page position.
-It divides the curPagePos value by PAGE_SIZE to calculate block position. This assumes each block is of size PAGE_SIZE in bytes.
-The integer block position calculated is returned back to the caller.
-If the file handle is valid, it returns the integer block position based on the current page position.
-This allows finding the block position in the file corresponding to the currently loaded page. Handles invalid file handle error cases.

## extern RC readFirstBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)

- readFirstBlock() takes a file handle and memory page handle as parameters.
- It reads the first block of the file into the given memory page.
- The first block of a file is always at position 0.
- It calls readBlock(), passing 0 as the page number to read.
- readBlock() handles all the logic of reading a specific block from file to memory.
- The file handle's current page position is passed to readBlock() to update after reading.
- Returns any error code encountered while reading the block.

## extern RC readLastBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)

- readLastBlock() takes a file handle and memory page handle as parameters.
- It checks if the total number of pages is 0, if so returns error code RC_READ_NON_EXISTING_PAGE.
- Otherwise calls readBlock() with page number totalNumPages - 1 to read the last block/page into memory.
- Handles empty file case and leverages readBlock() to read last page into memory.
- Returns any error encountered while reading last block.

## extern RC readPreviousBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)

- Checks if current page pos is <= 0, returns error if true..
- Calls readBlock() with current page pos - 1 to read previous page into memory.

## extern RC readCurrentBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)

- Validates current page pos is within total pages.
- Calls readBlock() with current page pos to read current page.

## extern RC readNextBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)

- Checks if current page pos is at last page, returns error if true.
- Calls readBlock() with current page pos + 1 to read next page.

## extern RC writeBlock (int pageNum, SM_FileHandle \*fHandle, SM_PageHandle memPage)

- Takes a page number, file handle, and memory page as inputs to write a specific page from memory into the file at a given offset.
- Validates that the file handle is initialized and the page number is valid, ensuring that the method is called with valid parameters.
- Calculates the file offset using the page number and the size of each page (PAGE_SIZE).
- Seeks to the offset in the file using fseek(), ensuring that the write operation is performed at the correct location.
- Writes PAGE_SIZE bytes from the memory page to the file using fwrite(), ensuring that the data is written correctly and efficiently.
- Updates the current page position and extends the total pages if writing past the old total, ensuring that the file's capacity is maintained and the correct number of pages is used.
- Note: The writePage method handles invalid parameters, bounds checking, and writing page data, making it a convenient and reliable method for writing specific pages from memory to a file.

## extern RC writeCurrentBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)

- Validates that the file handle is initialized, ensuring that the method is called with a valid file handle.
- Validates that the current page number is within the total pages, ensuring that the method is called with a valid page number.
- Calculates the offset using the current page position and the size of each page (PAGE_SIZE).
- Seeks to the calculated offset in the file using fseek(), ensuring that the write operation is performed at the correct location.
- Writes PAGE_SIZE bytes from the memory page to the current position, ensuring that the data is written correctly and efficiently.
- Returns RC_OK if the write succeeded, or an error code if there is an issue with the file handle, page bounds, or write operation.

## extern RC appendEmptyBlock (SM_FileHandle \*fHandle)

- Validates that the file handle is initialized and ensures that it is not null.
- Calculates the offset of the new last page using the total number of pages and the size of each page (PAGE_SIZE).
- Seeks to the new last page offset in the file and writes PAGE_SIZE bytes of zeroes to create an empty block.
- Increments the total number of pages to account for the appended block, ensuring that the file capacity is sufficient for the new number of pages.
- Returns RC_OK if the appended block is successfully written, or RC_WRITE_FAILED if there is an issue with writing the empty block.

## extern RC ensureCapacity(int numberOfPages, SM_FileHandle \*fHandle)

- Validates that the file handle is initialized and ensures that it is not null.
- Calculates the required capacity in bytes based on the given number of pages, taking into account the size of each page.
- Checks if the current file size is less than or equal to the required capacity, and if not, calculates the number of pages that need to be appended to ensure the required capacity.
- Appends empty pages to the file by calling appendEmptyBlock() in a loop, ensuring that the file capacity is sufficient for the required number of pages.
- Returns RC_OK if the capacity is ensured successfully, or an error code if there is an issue with the file handle or appending the empty pages.

# Memeory Leak Checks

- Used Valgrind to check for potential memory leaks and debugging the memory leaks.
  command- valgrind --leak-check=full --show-leak-kinds=all ./test_assign1

## Valgrind output for Test Case 1

       HEAP SUMMARY:
       ==26618==     in use at exit: 472 bytes in 1 blocks
       ==26618==   total heap usage: 11 allocs, 10 frees, 23,864 bytes allocated
       ==26618==

       ==26618==
       ==26618== LEAK SUMMARY:
       ==26618==    definitely lost: 0 bytes in 0 blocks
       ==26618==    indirectly lost: 0 bytes in 0 blocks
       ==26618==      possibly lost: 0 bytes in 0 blocks
       ==26618==    still reachable: 472 bytes in 1 blocks
       ==26618==         suppressed: 0 bytes in 0 blocks
       ==26618==
       ==26618== For lists of detected and suppressed errors, rerun with: -s
       ==26618== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)

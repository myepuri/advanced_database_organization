#include<stdio.h>
#include<stdlib.h>
#include "storage_mgr.h"
#include<string.h>


FILE *page;

//initializing page handler 
extern void initStorageManager (void){
	page = NULL;
}


RC createPageFile(char *fileName) {

	// opening the file in write binary mode
    FILE *file = fopen(fileName, "wb");
    if (file == NULL) {
        return RC_FILE_NOT_FOUND;
    }
	
	// initialising buffer
    size_t writeSize = 0;
    char buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);

	// writing data from buffer to file
    while (writeSize < PAGE_SIZE) {
        size_t bytesWritten = fwrite(buffer, sizeof(char), PAGE_SIZE - writeSize, file);
        if (bytesWritten == 0) {
            break;
        }
        writeSize += bytesWritten;
    }

	// check if written bytes are less than page size
    if (writeSize < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    fclose(file);
    return RC_OK;
}


RC openPageFile(char *fileName, SM_FileHandle *fHandle) {

  // Initialising file handle
  fHandle->fileName = fileName;
  fHandle->totalNumPages = 0;
  fHandle->curPagePos = 0;
  fHandle->mgmtInfo = NULL;

  // Opening the file in read binary mode
  FILE* file = fopen(fileName, "rb+");
  if(!file) {
    return RC_FILE_NOT_FOUND;
  }

  // Storing the file in file handle
  fHandle->mgmtInfo = file;

  // Getting the file size
  fseek(file, 0L, SEEK_END);
  long size = ftell(file);

  // Calculating total no. of pages
  fHandle->totalNumPages = size / PAGE_SIZE;

  return RC_OK;

}


RC closePageFile(SM_FileHandle *fHandle) {
  
  if(!fHandle) 
    return RC_FILE_HANDLE_NOT_INIT;

  // Closing file
  FILE* file = fHandle->mgmtInfo;
  fclose(file);

  // Reset handle
  fHandle->mgmtInfo = NULL;
  fHandle->totalNumPages = 0;
  fHandle->curPagePos = 0;

  return RC_OK;

}

RC destroyPageFile(char *fileName) {

	// Removing file, returns error if file not found
    if (remove(fileName) != 0) {
        return RC_FILE_NOT_FOUND;
    }

    return RC_OK;  // Return successful deletion
}


RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

  // Validate file handle and return error code if not found
  if (!fHandle || !fHandle->mgmtInfo) 
    return RC_FILE_HANDLE_NOT_INIT;

  // Validating page number
  if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
    return RC_READ_NON_EXISTING_PAGE;

  // Calculating offset with page number and page size
  long offset = pageNum * PAGE_SIZE;

  // Seek to offset and read page data
  FILE* file = fHandle->mgmtInfo; 
  fseek(file, offset, SEEK_SET);
  size_t read = fread(memPage, PAGE_SIZE, 1, file);
  if (read != 1)
    return RC_READ_ERROR;

  // Updating current page position
  fHandle->curPagePos = pageNum;

  return RC_OK;
}

int getBlockPos(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        // Check if fHandle is null and return error code
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Calculating the block position based on the current page position and return it
    int blockPos = fHandle->curPagePos / PAGE_SIZE;

    return blockPos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    
    // Check if the file is empty
    if (fHandle->totalNumPages == 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Return the first block of data from the file and store it in the mempage buffer
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the file is empty
    if (fHandle->totalNumPages == 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Return the last block of data from the file
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the current page is the first page
    if (fHandle->curPagePos <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Return the previous block of data from the file
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the current page is valid
    if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Return the current block of data from the file
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the current page is the last page
    if (fHandle->curPagePos >= fHandle->totalNumPages - 1) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Return the next block of data from the file
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}



RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

  // Check for valid file handle
  if (!fHandle || !fHandle->mgmtInfo) 
    return RC_FILE_HANDLE_NOT_INIT;

  // Check for valid page number  
  if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
    return RC_WRITE_NON_EXISTING_PAGE;

  // Calculate file offset
  long offset = pageNum * PAGE_SIZE;

  // Seek to offset and write page data
  FILE* file = fHandle->mgmtInfo;
  fseek(file, offset, SEEK_SET);
  size_t written = fwrite(memPage, PAGE_SIZE, 1, file);
  if (written != 1) 
    return RC_WRITE_FAILED;

  // Update current page and total no. of pages if extended
  fHandle->curPagePos = pageNum;
  if (pageNum >= fHandle->totalNumPages) 
    fHandle->totalNumPages = pageNum + 1;

  return RC_OK;
}


RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

  // Validate file handle
  if (!fHandle || !fHandle->mgmtInfo)
    return RC_FILE_HANDLE_NOT_INIT;

  // Validate current page number
  if (fHandle->curPagePos < 0 || fHandle->curPagePos >= fHandle->totalNumPages) 
    return RC_WRITE_NON_EXISTING_PAGE;

  // Calculate offset
  long offset = fHandle->curPagePos * PAGE_SIZE;

  // Seek to offset and write page data
  FILE* file = fHandle->mgmtInfo;
  fseek(file, offset, SEEK_SET);
  size_t written = fwrite(memPage, PAGE_SIZE, 1, file);
  if (written != 1)
    return RC_WRITE_FAILED;

  return RC_OK;
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {

  // Validate file handle
  if (!fHandle || !fHandle->mgmtInfo)
    return RC_FILE_HANDLE_NOT_INIT;

  // Compute offset of new last page 
  long offset = fHandle->totalNumPages * PAGE_SIZE;

  // Seek to offset and write empty page
  FILE* file = fHandle->mgmtInfo;
  fseek(file, offset, SEEK_SET);

  char empty[PAGE_SIZE];
  memset(empty, 0, PAGE_SIZE);
  
  size_t written = fwrite(empty, PAGE_SIZE, 1, file);
  if(written != 1)
    return RC_WRITE_FAILED;

  // Increment total pages
  fHandle->totalNumPages++;

  return RC_OK;
}

RC ensureCapacity(int numPages, SM_FileHandle *fHandle) {

  // Validate file handle
  if(!fHandle || !fHandle->mgmtInfo) 
    return RC_FILE_HANDLE_NOT_INIT;

  // Calculate required capacity in bytes
  int requiredSize = numPages * PAGE_SIZE;

  // Get current file size
  FILE* file = fHandle->mgmtInfo;
  fseek(file, 0, SEEK_END);
  int currentSize = ftell(file);

  // If current size is less, append empty pages
  if(currentSize < requiredSize) {
    
    // Calculate number of pages to append
    int pagesToAppend = (requiredSize - currentSize) / PAGE_SIZE;

    for(int i = 0; i < pagesToAppend; i++) {
      RC result = appendEmptyBlock(fHandle);
      if(result != RC_OK)
        return result; 
    }

  }
  return RC_OK;

}
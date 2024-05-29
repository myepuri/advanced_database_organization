#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include "storage_mgr.h"

FILE *pageFile;

extern void initStorageManager (void) {
	// Initialising file pointer i.e. storage manager.
	pageFile = NULL;
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
    return RC_ERROR;

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


extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if pageNum is within the acceptable range
    if (pageNum < 0 || pageNum > fHandle->totalNumPages) {
        return RC_WRITE_FAILED;
    }

    // Open the file for reading and writing
    FILE *pageFile = fopen(fHandle->fileName, "r+");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Calculate the start position
    int startPosition = PAGE_SIZE * pageNum;

    // Check if it's the first page
    if (pageNum == 0) {
        // If it's the first page, write to it directly
        fseek(pageFile, startPosition, SEEK_SET);
        for (int i = 0; i < PAGE_SIZE; i++) {
            // If the file ends before we finish writing, append an empty block
            if (feof(pageFile)) {
                appendEmptyBlock(fHandle);
            }
            // Write a character from memPage to page file
            fputc(memPage[i], pageFile);
        }
    } else {
        // If it's not the first page, use writeCurrentBlock to write to it
        fHandle->curPagePos = startPosition;
        writeCurrentBlock(fHandle, memPage);
    }

    // Update the current page position and close the file
    fHandle->curPagePos = ftell(pageFile);
    fclose(pageFile);


    return RC_OK;
}

// Write the current block/page to disk 
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {

  // Open file for reading and writing
  FILE *pageFile = fopen(fHandle->fileName, "r+");
  // Check if file open failed
  if(pageFile == NULL) {
    return RC_FILE_NOT_FOUND; 
  }
  // Append empty block to extend file size
  appendEmptyBlock(fHandle);
  // Seek to current page position
  fseek(pageFile, fHandle->curPagePos, SEEK_SET);
  // Write page data to file 
  fwrite(memPage, sizeof(char), strlen(memPage), pageFile);
  // Update current page position
  fHandle->curPagePos = ftell(pageFile);
  // Close file
  fclose(pageFile);
  return RC_OK;
}


extern RC appendEmptyBlock (SM_FileHandle *fHandle) {
    // Create an empty page of size PAGE_SIZE bytes
    SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if(emptyBlock == NULL) {
        return RC_WRITE_FAILED; // Memory allocation failed
    }

    // Open the file for appending
    FILE* pageFile = fopen(fHandle->fileName, "a");
    if (pageFile == NULL) {
        free(emptyBlock);
        return RC_FILE_NOT_FOUND; 
    }

    // Write the empty page to the end of the file
    size_t written = fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageFile);
    if (written != PAGE_SIZE) {
        free(emptyBlock);
        fclose(pageFile);
        return RC_WRITE_FAILED;
    }

    // Close the file
    fclose(pageFile);

    // Deallocate the memory previously allocated to 'emptyBlock'
    free(emptyBlock);

    // Increment the total number of pages
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
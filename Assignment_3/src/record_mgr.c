#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"



// RecordManager is a custom data structure used for managing records.
typedef struct RecordManager
{
    // pageHandle is used to interact with page files via the Buffer Manager.
    BM_PageHandle pageHandle;
    
    // bufferPool is the Buffer Manager's buffer pool used for record handling.
    BM_BufferPool bufferPool;
    
    // recordID uniquely identifies a particular record.
    RID recordID;
    
    // condition is an expression that dictates the criteria for record scanning.
    Expr *condition;
    
    // tuplesCount holds the total number of tuples in the table.
    int tuplesCount;
    
    // freePage is the index of the first page in the table that has available slots.
    int freePage;
    
    // scanCount keeps track of the number of records scanned so far.
    int scanCount;

} RecordManager;



// MAX_NUMBER_OF_PAGES defines the maximum number of pages that can be handled.
#define MAX_NUMBER_OF_PAGES 100

// ATTRIBUTE_SIZE specifies the maximum character length of an attribute's name.
#define ATTRIBUTE_SIZE 15 

RecordManager *recordManager;



// This function returns a free slot within a page
int findFreeSlot(char *data, int recordSize)
{
    char *end = data + PAGE_SIZE;
    int slotIndex = 0;

    for (char *current = data; current < end; current += recordSize, ++slotIndex)
        if (*current != '+')
            return slotIndex;
    return -1;
}

// This function initializes the Record Manager
extern RC initRecordManager (void *mgmtData)
{
	// Initiliazing Storage Manager
	initStorageManager();
	return RC_OK;
}

// This functions shuts down the Record Manager
extern RC shutdownRecordManager ()
{
	recordManager = NULL;
	free(recordManager);
	return RC_OK;
}


// Helper function to write schema attributes to the page
void writeSchemaAttributes(char** pageHandle, Schema* schema) {
    for(int k = 0; k < schema->numAttr; k++) {
        strncpy(*pageHandle, schema->attrNames[k], ATTRIBUTE_SIZE);
        *pageHandle += ATTRIBUTE_SIZE;

        **(int**)pageHandle = schema->dataTypes[k];
        *pageHandle += sizeof(int);

        **(int**)pageHandle = schema->typeLength[k];
        *pageHandle += sizeof(int);
    }
}

extern RC createTable (char *name, Schema *schema)
{

    // Validate input parameters
    if (name == NULL || schema == NULL) {
        printf("Invalid parameters.\n");
        return RC_RM_NULL_ARGUMENT;  
    }

    char data[PAGE_SIZE];
    char *pageHandle = data;

    // Allocate memory for the record manager and initialize the buffer pool
    recordManager = (RecordManager*) malloc(sizeof(RecordManager));
    initBufferPool(&recordManager->bufferPool, name, MAX_NUMBER_OF_PAGES, RS_LRU, NULL);

    
 
    // Write initial configurations to the page
    *(int*)pageHandle = 0;  // number of tuples
    pageHandle += sizeof(int);
    *(int*)pageHandle = 1;  // first page
    pageHandle += sizeof(int);
    *(int*)pageHandle = schema->numAttr;  // number of attributes
    pageHandle += sizeof(int);
    *(int*)pageHandle = schema->keySize;  // Key Size of attributes
    pageHandle += sizeof(int);

    // Write schema attributes to the page
    writeSchemaAttributes(&pageHandle, schema);

    SM_FileHandle fileHandle;
    RC result;

    // Perform page file operations and check for errors
    if ((result = createPageFile(name)) != RC_OK ||
        (result = openPageFile(name, &fileHandle)) != RC_OK ||
        (result = writeBlock(0, &fileHandle, data)) != RC_OK ||
        (result = closePageFile(&fileHandle)) != RC_OK) {
        return result;
    }

    return RC_OK;
}




// Helper function to read schema attributes from the page
void readSchemaAttributes(char** pageHandle, Schema* schema) {
    for(int k = 0; k < schema->numAttr; k++) {
        // Allocate memory for the attribute name
        schema->attrNames[k] = (char*) malloc(ATTRIBUTE_SIZE);
        // Read the attribute name from the page
        strncpy(schema->attrNames[k], *pageHandle, ATTRIBUTE_SIZE);
        *pageHandle += ATTRIBUTE_SIZE;

        // Read the attribute data type from the page
        schema->dataTypes[k] = **(int**)pageHandle;
        *pageHandle += sizeof(int);

        // Read the attribute type length from the page
        schema->typeLength[k] = **(int**)pageHandle;
        *pageHandle += sizeof(int);
    }
}

extern RC openTable (RM_TableData *rel, char *name)
{
    // Initialize record manager and table data
    rel->mgmtData = recordManager;
    rel->name = name;
    // Pin the first page of the table
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);

    char* pageHandle = (char*) recordManager->pageHandle.data;
    // Read the tuples count from the page
    recordManager->tuplesCount = *(int*)pageHandle;
    pageHandle += sizeof(int);
    // Read the free page from the page
    recordManager->freePage = *(int*)pageHandle;
    pageHandle += sizeof(int);

    // Create a new schema and allocate memory for its attributes
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    // Read the number of attributes from the page
    schema->numAttr = *(int*)pageHandle;
    pageHandle += sizeof(int);
    // Allocate memory for the attribute names, data types, and type lengths
    schema->attrNames = (char**) malloc(sizeof(char*) * schema->numAttr);
    schema->dataTypes = (DataType*) malloc(sizeof(DataType) * schema->numAttr);
    schema->typeLength = (int*) malloc(sizeof(int) * schema->numAttr);

    // Read schema attributes from the page
    readSchemaAttributes(&pageHandle, schema);

    // Create result variable
    RC result;

    // Unpin the page and check for errors
    result = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (result != RC_OK) {
     return result;
    }

    // Force the page and check for errors
    result = forcePage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (result != RC_OK) {
        return result;
    }

    // Assign schema to relation
    rel->schema = schema;

    // Return success code
    return RC_OK;
}


  
// This function closes the table and cleans up the buffer pool
extern RC closeTable (RM_TableData *rel)
{
    // Get the record manager from the relation data
    RecordManager *recordManager = rel->mgmtData;

    // If the record manager exists, shut down the buffer pool
    if (recordManager != NULL && &recordManager->bufferPool != NULL) {
        shutdownBufferPool(&recordManager->bufferPool);
    }

    // Return success code
    return RC_OK;
}

// This function deletes a table by its name
extern RC deleteTable (char *tableName)
{
    // If the table name exists, destroy the page file
    if(tableName != NULL) {
        destroyPageFile(tableName);
    }

    // Return success code
    return RC_OK;
}

// This function returns the count of tuples in the given table
extern int getNumTuples (RM_TableData *rel)
{
    // If the relation data and record manager exist, return the tuples count
    if(rel != NULL && rel->mgmtData != NULL) {
        return ((RecordManager*)rel->mgmtData)->tuplesCount;
    }

    // If the relation data or record manager does not exist, return 0
    return 0;
}


// Function to manage page pinning and data setting for a record
void manageRecordPage(RM_TableData *rel, Record *record, int recordSize, int nextPageFlag) {
    // Validate input parameters
    if (rel == NULL || record == NULL || recordSize <= 0) {
        printf("Invalid parameters.\n");
        return;
    }

    // Retrieve the RecordManager and the RID from the parameters
    RecordManager *recordManager = rel->mgmtData;
    RID *recordID = &record->id;

    // Page management
    if (nextPageFlag) {
        if (unpinPage(&recordManager->bufferPool, &recordManager->pageHandle) != RC_OK) {
            printf("Failed to unpin page.\n");
            return;
        }
        recordID->page++;
    }

    if (pinPage(&recordManager->bufferPool, &recordManager->pageHandle, recordID->page) != RC_OK) {
        printf("Failed to pin page.\n");
        return;
    }

    // Find and set the slot for the record
    recordID->slot = findFreeSlot(recordManager->pageHandle.data, recordSize);
}

extern RC insertRecord (RM_TableData *rel, Record *record)
{
    // Validate input parameters
    if (rel == NULL || record == NULL) {
        printf("Invalid parameters.\n");
        return RC_INVALID_PARAMETER;
    }

    // Retrieve the RecordManager and the RID from the parameters
    RecordManager *recordManager = rel->mgmtData;
    RID *recordID = &record->id;

    // Calculate the size of the record
    int recordSize = getRecordSize(rel->schema);

    // Assign the free page to the record ID
    recordID->page = recordManager->freePage;

    // Manage the record page without incrementing the page
    manageRecordPage(rel, record, recordSize, 0);

    // If no free slot is found, manage the record page with incrementing the page
    while(recordID->slot == -1) {
        manageRecordPage(rel, record, recordSize, 1);
    }

    // Get the pointer to the slot in the page
    char *slotPointer = recordManager->pageHandle.data + (recordID->slot * recordSize);

    // Mark the page as dirty as it's going to be modified
    markDirty(&recordManager->bufferPool, &recordManager->pageHandle);

    // Assign the record to the slot and advance the slotPointer
    *slotPointer = '+';
    memcpy(slotPointer + 1, record->data + 1, recordSize - 1);

    // Unpin the page, as we've finished writing to it
    RC status = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (status != RC_OK) {
        printf("Failed to unpin the page.\n");
        return status;
    }

    // Increment the count of tuples in the record manager
    recordManager->tuplesCount++;

    // Pin the first page in the buffer pool
    status = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
    if (status != RC_OK) {
        printf("Failed to pin the first page.\n");
    }

    // Return the status of the operation
    return status;
}




RC deleteRecord (RM_TableData *rel, RID id) {
    // Validate input parameters
    if (rel == NULL) {
        printf("Invalid parameters.\n");
        return RC_RM_NULL_ARGUMENT;
    }

    // Retrieve the record manager from the table's meta data
    RecordManager *recordManager = rel->mgmtData;

    // Calculate the size of the record
    int recordSize = getRecordSize(rel->schema);

    // Pin the page containing the record to be deleted
    RC status = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);
    if (status != RC_OK) {
        printf("Failed to pin page.\n");
        return status;
    }

    // Set the data pointer to the start of the record to be deleted
    char *data = recordManager->pageHandle.data + id.slot * recordSize;

    // Mark the record with a tombstone as deleted 
    *data = '-';

    // Update the free page since this page now has a free slot
    recordManager->freePage = id.page;

    // Mark the page as dirty since it has been modified
    status = markDirty(&recordManager->bufferPool, &recordManager->pageHandle);
    if (status != RC_OK) {
        printf("Failed to mark page as dirty.\n");
        return status;
    }

    // Unpin the page since we're done modifying it
    status = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (status != RC_OK) {
        printf("Failed to unpin page.\n");
        return status;
    }

    return RC_OK;
}



extern RC updateRecord (RM_TableData *rel, Record *record)
{
    // Retrieve the record manager from the table's metadata
    RecordManager *recordManager = rel->mgmtData;
    
    // Calculate the size of the record
    int recordSize = getRecordSize(rel->schema);
    
    // Pin the page containing the record to be updated
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, record->id.page);

    // Calculate the start position of the record to be updated
    char *data = recordManager->pageHandle.data + record->id.slot * recordSize;
    
   // Update the record data
    data[0] = '+';
    memcpy(data + 1, record->data + 1, recordSize - 1);

    // Handle page modifications
    if (markDirty(&recordManager->bufferPool, &recordManager->pageHandle) == RC_OK && 
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle) == RC_OK) {
        return RC_OK;
    } else {
        return RC_ERROR;
    }
}



RC getRecord(RM_TableData *rel, RID id, Record *record) {
    // Validate inputs
    if(rel == NULL || record == NULL) {
        return RC_INVALID_PARAMETER;
    }

    // Initialize RecordManager
    RecordManager *recordManager = rel->mgmtData;

    // Calculate the record size
    int recordSize = getRecordSize(rel->schema);
    if(recordSize <= 0) {
        return RC_RM_UNKOWN_DATATYPE;
    }

    // Pin the page containing the record
    RC pinStatus = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);
    if(pinStatus != RC_OK) {
        return pinStatus;
    }

    // Calculate the slot's position in the page
    char *slotPointer = recordManager->pageHandle.data + (id.slot * recordSize);

    // Check if the record exists
    if(*slotPointer != '+') {
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle); 
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    }

    // Copy the record's data
    record->id = id;
    char *recordData = record->data;
    memcpy(++recordData, ++slotPointer, recordSize - 1);

    // Unpin the page
    RC unpinStatus = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if(unpinStatus != RC_OK) {
        return unpinStatus;
    }

    return RC_OK;
}




// ******** SCAN FUNCTIONS ******** //

// This function scans all the records using the condition
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    // Return an error if the scan condition is not provided
    if (!cond) return RC_SCAN_CONDITION_NOT_FOUND;

    // Open the table in memory
    openTable(rel, "ScanTable");

    // Allocate memory for Scan Manager
RecordManager *scanManager = calloc(1, sizeof(RecordManager));
// Handle memory allocation failure
if(scanManager == NULL) return RC_RM_NO_MORE_MEMORY;

// Set the mgmtData of scan to scanManager
scan->mgmtData = scanManager;

// Set the initial page and slot for the scan
scanManager->recordID = (RID){ .page = 1, .slot = 0 };

// Initialize scanCount and condition
scanManager->scanCount = 0; 
scanManager->condition = cond;


    // Set the scan's table
    scan->rel= rel;

    // Set the table manager attributes
    RecordManager *tableManager = rel->mgmtData;
    tableManager->tuplesCount = ATTRIBUTE_SIZE;  // Set the tuple count

    return RC_OK;
}



// Helper function to increment the record ID
void incrementRecordID(RecordManager *scanManager, int totalSlots) {
    if(++scanManager->recordID.slot >= totalSlots) {
        scanManager->recordID.slot = 0;
        scanManager->recordID.page++;
    }
}

extern RC next (RM_ScanHandle *scan, Record *record) {
    // Validate input parameters
    if (scan == NULL || record == NULL) {
        printf("Invalid parameters.\n");
        return RC_RM_NULL_ARGUMENT;
    }

    // Retrieve the necessary data
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *tableManager = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;
    int totalSlots = PAGE_SIZE / getRecordSize(schema);
    Value *result = (Value *) malloc(sizeof(Value));

    // Early return if no tuples or no scan condition
    if (tableManager->tuplesCount == 0) return RC_RM_NO_MORE_TUPLES;
    if (scanManager->condition == NULL) return RC_SCAN_CONDITION_NOT_FOUND;

    // Scan loop
    while(scanManager->scanCount++ < tableManager->tuplesCount) {
        // Handle incrementing record ID
        if (scanManager->scanCount > 1) incrementRecordID(scanManager, totalSlots);

        // Pin the page
        RC status = pinPage(&tableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page);
        if (status != RC_OK) {
            printf("Failed to pin page.\n");
            return status;
        }

        // Retrieve record data
        char *data = scanManager->pageHandle.data + (scanManager->recordID.slot * getRecordSize(schema));

        // Copy record data
        record->id = scanManager->recordID;
        *record->data = '-';
        memcpy(record->data + 1, data + 1, getRecordSize(schema) - 1);

        // Check record against scan condition
        evalExpr(record, schema, scanManager->condition, &result);

        // If record meets condition, unpin page and return
        if(result->v.boolV == TRUE) {
            status = unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
            if (status != RC_OK) {
                printf("Failed to unpin page.\n");
                return status;
            }
            free(result);
            return RC_OK;
        }
    }

    // Reinitialize scan manager
    scanManager->recordID = (RID){ .page = 1, .slot = 0 };
    scanManager->scanCount = 0;

    // No more tuples
    return RC_RM_NO_MORE_TUPLES;
}



extern RC closeScan (RM_ScanHandle *scan)
{
	if (scan == NULL || scan->rel == NULL) {
    printf("Invalid parameters.\n");
    return RC_RM_NULL_ARGUMENT; 
    }


    // Retrieve the management data
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *recordManager = scan->rel->mgmtData;

    // Reset the Scan Manager's state
    scanManager->scanCount = 0;
    scanManager->recordID = (RID){ .page = 1, .slot = 0 };

    // Unpin the page
    unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);

    // De-allocate all the memory space allocated to the scans's meta data
    free(scanManager);
    scan->mgmtData = NULL;
	
    return RC_OK;
}






extern int getRecordSize (Schema *schema)
{
	int size = 0;

	// Iterate through all the attributes in the schema
	for(int i = 0; i < schema->numAttr; i++)
	{
		// addition assignment to accumulate size
		switch(schema->dataTypes[i])
		{
			case DT_STRING:
				size += schema->typeLength[i];
				break;
			case DT_INT:
				size += sizeof(int);
				break;
			case DT_FLOAT:
				size += sizeof(float);
				break;
			case DT_BOOL:
				size += sizeof(bool);
				break;
		}
	}

	// Increment size outside the loop to reduce computations
	return size + 1;
}




extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    // Check the size of the Schema type to ensure it is valid.
    // If the Schema size is invalid, print an error message and return NULL.
    if (sizeof(Schema) <= 0) {
        printf("Error: Invalid Schema size.\n");
        return NULL;
    }

    // Allocate memory space to the schema.
    // If allocation fails (i.e., malloc returns NULL), return NULL to indicate failure.
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    if(schema == NULL) {
        return NULL;
    }

    // Initialize the schema with the provided attribute names, data types, type lengths, key size, and keys.
    *schema = (Schema){
        .numAttr = numAttr,
        .attrNames = attrNames,
        .dataTypes = dataTypes,
        .typeLength = typeLength,
        .keySize = keySize,
        .keyAttrs = keys
    };

    // Return the pointer to the newly created schema.
    return schema; 
}

extern RC freeSchema (Schema *schema)
{
    // If schema is NULL, return early
    if (schema == NULL) {
        return RC_OK;
    }

    // Free dynamically allocated members
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);

    // Free the schema itself
    free(schema);
    
    return RC_OK;
}


extern RC createRecord (Record **record, Schema *schema)
{
    // Allocate memory for the new record and initialize its members
    *record = (Record*) malloc(sizeof(Record));
    if(*record == NULL) {
        // Handle memory allocation failure
        return RC_MEMORY_ALLOCATION_ERROR;
    }

    // Retrieve the record size and allocate memory for the record's data
    int recordSize = getRecordSize(schema);
    (*record)->data = (char*) malloc(recordSize);
    if((*record)->data == NULL) {
        // Handle memory allocation failure
        free(*record);
        return RC_MEMORY_ALLOCATION_ERROR;
    }

    // Initialize the record's ID
    (*record)->id = (RID){ .page = -1, .slot = -1 };

    // Initialize the data with tombstone and null terminator
    (*record)->data[0] = '-';
    (*record)->data[1] = '\0';
    return RC_OK;
}


RC attrOffset (Schema *schema, int attrNum, int *result)
{
    *result = 1;

    // Iterate through the attributes in the schema until "attrNum"
    for(int i = 0; i < attrNum; i++)
    {
        // Use addition assignment to accumulate size
        switch (schema->dataTypes[i])
        {
            case DT_STRING:
                *result += schema->typeLength[i];
                break;
            case DT_INT:
                *result += sizeof(int);
                break;
            case DT_FLOAT:
                *result += sizeof(float);
                break;
            case DT_BOOL:
                *result += sizeof(bool);
                break;
        }
    }

    return RC_OK;
}

extern RC freeRecord (Record *record)
{
    if (record == NULL) {
        return RC_OK;
    }

    // Free dynamically allocated members
    free(record->data);

    // Free the record itself
    free(record);
    
    return RC_OK;
}



// Function prototypes
char* getStringValue(char* dataPointer, int length);
int getIntValue(char* dataPointer);
float getFloatValue(char* dataPointer);
bool getBoolValue(char* dataPointer);

// This function retrieves an attribute from the given record in the specified schema
extern RC getAttr(Record *record, Schema *schema, int attrNum, Value **value)
{
    // Allocate memory for the Value structure 
    Value *attribute = (Value*) malloc(sizeof(Value));

    // Calculate offset for the attribute using attrOffset function
    int offset;
    attrOffset(schema, attrNum, &offset);

    // Get a pointer to the location of the attribute in the record's data
    char *dataPointer = record->data + offset;
    
    // Adjust the dataType for attrNum == 1 and get the corresponding type
    // If attrNum == 1, type is set to 1, else it is set to the corresponding type from schema
    DataType type = (attrNum == 1) ? 1 : schema->dataTypes[attrNum];
    
    // Retrieve the attribute value based on its type
    switch(type)
    {
        case DT_STRING:
            // Set the data type of the Value structure to DT_STRING
            attribute->dt = DT_STRING;

            // Call the getStringValue function to get the string value from the data pointer
            attribute->v.stringV = getStringValue(dataPointer, schema->typeLength[attrNum]);
            break;
        case DT_INT:
            // Set the data type of the Value structure to DT_INT
            attribute->dt = DT_INT;

            // Call the getIntValue function to get the integer value from the data pointer
            attribute->v.intV = getIntValue(dataPointer);
            break;
        case DT_FLOAT:
            // Set the data type of the Value structure to DT_FLOAT
            attribute->dt = DT_FLOAT;

            // Call the getFloatValue function to get the float value from the data pointer
            attribute->v.floatV = getFloatValue(dataPointer);
            break;
        case DT_BOOL:
            // Set the data type of the Value structure to DT_BOOL
            attribute->dt = DT_BOOL;

            // Call the getBoolValue function to get the boolean value from the data pointer
            attribute->v.boolV = getBoolValue(dataPointer);
            break;
        default:
            // Free the allocated memory for the Value structure
            free(attribute);

            // Print an error message if the data type is not defined
            printf("Serializer not defined for the given datatype. \n");

            // Return an error code
            return RC_ERROR;
    }
    
    // Set the value pointer to point to the attribute
    *value = attribute;

    // Return a success code
    return RC_OK;
}

// Function to handle string data type
char* getStringValue(char* dataPointer, int length)
{
    // Allocate memory for the string
    char* str = (char*)malloc(length + 1);

    // Copy the string from the data pointer to the allocated memory
    strncpy(str, dataPointer, length);

    // Add a null terminator at the end of the string
    str[length] = '\0';

    // Return the string
    return str;
}

// Function to handle integer data type
int getIntValue(char* dataPointer)
{
    // Declare an integer to hold the value
    int value;

    // Copy the integer value from the data pointer to the integer variable
    memcpy(&value, dataPointer, sizeof(int));

    // Return the integer value
    return value;
}

// Function to handle float data type
float getFloatValue(char* dataPointer)
{
    // Declare a float to hold the value
    float value;

    // Copy the float value from the data pointer to the float variable
    memcpy(&value, dataPointer, sizeof(float));

    // Return the float value
    return value;
}

// Function to handle boolean data type
bool getBoolValue(char* dataPointer)
{
    // Declare a boolean to hold the value
    bool value;

    // Copy the boolean value from the data pointer to the boolean variable
    memcpy(&value, dataPointer, sizeof(bool));

    // Return the boolean value
    return value;
}

// This function sets the attribute value in the record in the specified schema
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    int offset;

    // Checking if attrNum is valid
    if (attrNum < 0 || attrNum >= schema->numAttr) {
        printf("Invalid attribute number. \n");
        return RC_INVALID_ATTRIBUTE_NUM;
    }

    // Get the offset of the attribute in the record
    attrOffset(schema, attrNum, &offset);

    // Get the pointer to the location in the record where the attribute should be stored
    char *dataPointer = record->data + offset;

    // Check if the datatype of the value to be set matches the datatype in the schema
    if (value->dt != schema->dataTypes[attrNum]) {
        printf("Datatype of value does not match the datatype in the schema. \n");
        return RC_DATATYPE_MISMATCH;
    }

   // Set the attribute value in the record based on its type
switch (value->dt) {
    case DT_STRING:
        // For string type, copy the string from Value to the location in record's data
        // Make sure we only copy up to the maximum length defined in the schema
        strncpy(dataPointer, value->v.stringV, schema->typeLength[attrNum]);
        break;

    case DT_INT:
        // For integer type, directly set the integer value at the location in record's data
        // Use a cast to treat the character pointer as an integer pointer
        *(int *)dataPointer = value->v.intV;
        break;

    case DT_FLOAT:
        // For float type, directly set the float value at the location in record's data
        // Use a cast to treat the character pointer as a float pointer
        *(float *)dataPointer = value->v.floatV;
        break;

    case DT_BOOL:
        // For boolean type, directly set the boolean value at the location in record's data
        // Use a cast to treat the character pointer as a boolean pointer
        *(bool *)dataPointer = value->v.boolV;
        break;

    default:
        // In case the data type is not recognized, print an error and return an error code
        printf("Serializer not defined for the given datatype. \n");
        return RC_SERIALIZER_NOT_DEFINED;
}

    return RC_OK;
}
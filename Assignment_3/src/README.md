RUNNING THE SCRIPT
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1) Go to Project root (Assignment_3) using Terminal.

3) Type "make clean" to delete old compiled .o files.

4) Type "make" to compile all project files including "test_assign3_1.c" file and "test_expr" file.

5) Type "./test_assign3_1" to run "test_assign3_1.c" file.

7) Type "./test_expr" to run "test_expr.c" file.



++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bonus: We have implemented tombstone mechanism
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Functions
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


findFreeSlot():
-This function scans through a page in memory to find a free slot, where a new record can be inserted.
-It takes two arguments: a char pointer to the data in memory (data) and the size of each record (recordSize).
-It works by iterating over the data, advancing by recordSize each time and checking if the current slot is free (indicated by anything other than a '+').
-If it finds a free slot, it returns the index of that slot. If no free slot is found, it returns -1.

initRecordManager():
-This function initializes the Record Manager.
-It takes one argument: a void pointer to the management data (mgmtData).
-It starts by initializing the Storage Manager.
-It returns RC_OK to indicate successful initialization.

shutdownRecordManager():
-This function shuts down the Record Manager.
-It does not take any arguments.
-It first sets the recordManager pointer to NULL and then frees the memory allocated to the recordManager.
-It returns RC_OK to indicate successful shutdown.

writeSchemaAttributes():
-This helper function writes the schema attributes to a page in memory.
-It takes two arguments: a double pointer to a char (pageHandle) and a pointer to the Schema struct (schema).
-The function iterates over the number of attributes in the schema and for each attribute, it writes the attribute name, data type, and type length to the memory page.

createTable():
-This function creates a new table with the given name and schema.
-It takes two arguments: a char pointer to the name of the table (name) and a pointer to the Schema struct (schema).
-The function begins by validating the input parameters. If either is NULL, it returns an error code RC_RM_NULL_ARGUMENT.
-It then allocates memory for the record manager and initializes the buffer pool.
-The function writes the initial configurations to the page, such as number of tuples, first page, number of attributes, and key size.
-It then calls writeSchemaAttributes() to write the schema attributes to the page.
-Finally, the function performs several page file operations: creating a page file, opening the page file, writing the block to the file, and closing the page file. If any of these operations fail, it returns the error code from that operation.


readSchemaAttributes():
-This helper function reads the schema attributes from a page in memory.
-It takes two arguments: a double pointer to a char (pageHandle) and a pointer to the Schema struct (schema).
-The function iterates over the number of attributes in the schema. For each attribute, it allocates memory for the attribute name, then reads the attribute name, data type, and type length from the memory page.

openTable():
-This function opens an existing table with the given name.
-It takes two arguments: a pointer to RM_TableData (rel) and a char pointer to the name of the table (name).
-The function begins by initializing the record manager and table data.
-It pins the first page of the table and reads the tuples count and free page from it.
-The function then creates a new schema and initializes it by reading the schema attributes from the page using readSchemaAttributes().
-It unpins and forces the page, checking for errors after each operation. If an error occurs, it returns the error code.
-Finally, it assigns the schema to the relation and returns RC_OK to indicate successful operation.

closeTable():
-This function closes an open table and cleans up the associated buffer pool.
-It takes a single argument: a pointer to RM_TableData (rel).
-The function gets the record manager from the relation data and checks if it is not NULL. If the record manager exists, it shuts down the buffer pool associated with it.
-The function returns RC_OK to indicate successful operation.

deleteTable():
-This function deletes an existing table based on its name.
-It takes a single argument: a char pointer to the name of the table (tableName).
-The function checks if the table name is not NULL. If the table name exists, it destroys the page file associated with it.
-The function returns RC_OK to indicate successful operation.

getNumTuples():
-This function returns the count of tuples in a given table.
-It takes a single argument: a pointer to RM_TableData (rel).
-The function checks if the relation data and record manager are not NULL. If they exist, it returns the tuples count from the record manager.
-If the relation data or record manager is NULL, the function returns 0.

manageRecordPage():
-This function handles page pinning and data setting for a record.
-It takes four arguments: a pointer to RM_TableData (rel), a pointer to Record (record), an integer for record size (recordSize), and a flag for next page (nextPageFlag).
-It validates the input parameters before proceeding.
-The function retrieves the RecordManager and RID from the input parameters.
-Depending on the nextPageFlag, it unpins the current page and increments the page number in the RID.
-It then pins the new page. If the pin operation fails, it returns from the function.
-The function finds a free slot in the new page and sets it in the RID.

insertRecord():
-This function inserts a record into a table.
-It takes two arguments: a pointer to RM_TableData (rel) and a pointer to Record (record).
-It validates the input parameters before proceeding.
-The function retrieves the RecordManager and RID from the input parameters.
-It calculates the size of the record and assigns the free page to the RID.
-The function uses manageRecordPage() to manage the record page without incrementing the page.
-If no free slot is found, it keeps managing the record page with incrementing the page until a free slot is found.
-After finding a free slot, the function marks the page as dirty, assigns the record to the slot, and unpins the page.
-The function increments the count of tuples in the RecordManager and pins the first page in the buffer pool.
-The function returns the status of the operation.

updateRecord():
-This function updates a record in a table.
-It takes two arguments: a pointer to RM_TableData (rel) and a pointer to Record (record).
-The function retrieves the RecordManager from the table's metadata and calculates the size of the record.
-It pins the page containing the record to be updated.
-The function calculates the start position of the record to be updated in the page.
-It updates the record data in the page.
-If the page has been successfully marked as dirty and unpinned after the update, the function returns RC_OK, indicating a successful operation. If not, it returns RC_ERROR.

getRecord():
-This function retrieves a record from a table.
-It takes three arguments: a pointer to RM_TableData (rel), an RID (id), and a pointer to Record (record).
-The function validates the input parameters before proceeding.
-It initializes a RecordManager and calculates the size of the record.
-The function pins the page containing the record to be retrieved.
-It calculates the slot's position in the page and checks if the record exists.
-If the record exists, the function copies the record's data.
-The function then unpins the page and returns RC_OK, indicating a successful operation. If the page cannot be unpinned, it returns the status of the unpin operation.

startScan():
-This function initiates a scan of all the records in a table based on a specified condition.
-It takes three arguments: a pointer to RM_TableData (rel), a pointer to RM_ScanHandle (scan), and an Expr (cond).
-The function checks if a scan condition is provided and returns an error if not.
-It opens the table in memory for the scan.
-The function allocates memory for RecordManager, which manages the scanning process. If memory allocation fails, it returns an error.
-It sets the mgmtData of scan to scanManager and initializes the recordID, scanCount, and condition in scanManager.
-The function sets the rel of scan to the input rel and sets the tuples count in the table's RecordManager.

incrementRecordID():
-This helper function increments the recordID in RecordManager.
-It takes two arguments: a pointer to RecordManager (scanManager) and the total number of slots (totalSlots).
-The function increments the slot in recordID. If the slot reaches the total number of slots, it resets the slot to 0 and increments the page in recordID.

next():
-This function retrieves the next record from an ongoing scan that meets the scan condition.
-It takes two arguments: a pointer to RM_ScanHandle (scan) and a pointer to Record (record).
-The function validates input parameters and retrieves the necessary data from scan, RecordManager, and Schema.
-It returns early if there are no tuples or if the scan condition is missing.
-The function then enters a loop to scan the records. The loop increments the record ID, pins the page, retrieves and copies the record data, evaluates the record against the scan condition.
-If a record meets the condition, the function unpins the page and returns RC_OK.
-If there are no more tuples, the function reinitializes the scan manager and returns RC_RM_NO_MORE_TUPLES.

closeScan():
-This function closes an ongoing scan.
-It takes one argument: a pointer to RM_ScanHandle (scan).
-The function validates the input parameter, retrieves the management data, and resets the scan manager's state.
-It unpins the page from the buffer pool and deallocates the memory space allocated to the scan's meta-data.
-The function returns RC_OK indicating a successful operation.

getRecordSize():
-This function calculates and returns the size of a record based on the provided schema.
-It takes one argument, a pointer to Schema (schema).
-The function iterates through all the attributes in the schema, and based on the data type of each attribute, it adds the corresponding size to the total size of the record.
-The function returns the total size of the record incremented by one. The increment accounts for the space for the null terminator in string representation of the record.

createSchema():
-This function creates a new schema and returns a pointer to it.
-It takes six arguments: the number of attributes (numAttr), an array of attribute names (attrNames), an array of data types (dataTypes), an array of lengths for each type (typeLength), the size of the primary key (keySize), and an array of keys (keys).
-The function first checks the size of the Schema type to ensure it is valid.
-It then allocates memory for the new schema. If memory allocation fails, it returns NULL.
-The function sets the attributes of the new schema based on the input parameters and returns the pointer to the new schema.

freeSchema():
-This function frees memory allocated to a schema.
-It takes one argument, a pointer to Schema (schema).
-If the schema pointer is NULL, the function returns immediately with RC_OK.
-It then frees the memory allocated to the schema's attributes, data types, type lengths, and keys.
-Finally, it frees the memory allocated to the schema itself.

createRecord():
-This function creates a new record based on a provided schema.
-It takes two arguments: a double pointer to Record (record) and a pointer to Schema (schema).
-The function allocates memory for a new record and initializes its members.
-It then calculates the size of the record based on the schema and allocates memory for the record's data.
-The function initializes the record's ID with -1 for both page and slot, indicating an uninitialized state.
-It sets the first character of the data to '-,' a tombstone indicating the record is deleted, followed by a null terminator.

attrOffset():
-This function calculates the offset of an attribute in a record.
-It takes three arguments: a pointer to Schema (schema), attribute number (attrNum), and a pointer to int (result).
-The function initializes result to 1, then iterates through the attributes in the schema until it reaches attrNum.
-For each attribute, it adds the size of the attribute to result based on the attribute's data type.

freeRecord():
-This function frees memory allocated to a record.
-It takes one argument, a pointer to Record (record).
-If the record pointer is NULL, the function returns immediately with RC_OK.
-It then frees the memory allocated to the record's data.
-Finally, it frees the memory allocated to the record itself.

getAttr():
-This function retrieves an attribute from a given record within a specified schema.
-It takes four arguments: a pointer to Record (record), a pointer to Schema (schema), an attribute number (attrNum), and a double pointer to Value (value).
-The function allocates memory for a Value structure and calculates the offset of the attribute in the record.
-It adjusts the data type for the attribute based on its number and retrieves the value of the attribute based on its type.
-The attribute value is then added to the Value structure and the pointer is returned.

getStringValue():
-This function extracts a string value from a given location in a record.
-It takes two arguments: a pointer to the location of the data in the record (dataPointer), and the length of the string (length).
-The function allocates memory for the string, copies the string from the record, and adds a null terminator at the end.

getIntValue():
-This function extracts an integer value from a given location in a record.
-It takes one argument: a pointer to the location of the data in the record (dataPointer).
-The function copies the integer value from the record and returns it.

getFloatValue():
-This function extracts a floating-point value from a given location in a record.
-It takes one argument: a pointer to the location of the data in the record (dataPointer).
-The function copies the floating-point value from the record and returns it.

getBoolValue():
-This function extracts a boolean value from a given location in a record.
-It takes one argument: a pointer to the location of the data in the record (dataPointer).
-The function copies the boolean value from the record and returns it.

setAttr():
-This function sets the value of a specified attribute in a given record according to a provided schema.
-It takes four arguments: a pointer to Record (record), a pointer to Schema (schema), an attribute number (attrNum), and a pointer to Value (value).
-The function starts by checking the validity of the attribute number. If the number is less than zero or greater than or equal to the number of attributes in the schema, an error message is printed and an error code (RC_INVALID_ATTRIBUTE_NUM) is returned.
-Next, it calculates the offset of the attribute in the record using the attrOffset() function.
-It then retrieves a pointer to the location in the record's data where the attribute value should be stored.
-The function checks that the data type of the provided value matches the data type of the attribute in the schema. If they do not match, an error message is printed and an error code (RC_DATATYPE_MISMATCH) is returned.
-Finally, the function sets the attribute value in the record according to the data type. If the data type is not recognized, an error message is printed and an error code (RC_SERIALIZER_NOT_DEFINED) is returned.
#include "Storage.h"
#include <cerrno>
#include <fcntl.h>

// #############################################################
/**
 * @brief Initializes a file, creating it if not existing.
 *
 * @param fileName
 * @return int (filesize, -1 on errors)
 */
int initFile(const char *fileName)
{
    // Open file, create if not existing.
    int fileDescriptor = open(fileName, O_RDWR | O_CREAT);

    int returnValue = -1;

    // Check for errors.
    if (fileDescriptor == -1)
        Serial.printlnf("Error opening file %d", errno);
    else
    {
        // Use fstat to determine file size
        struct stat statBuffer;
        if (fstat(fileDescriptor, &statBuffer) == -1)
            Serial.printlnf("Error getting stats: %d", errno);
        else
            returnValue = statBuffer.st_size;
    }

    close(fileDescriptor);
    return returnValue;
}

// #############################################################
/**
 * @brief Overloaded initializer. Takes an initial string.
 *
 * @param fileName
 * @param initString
 * @return int
 */
int initFile(const char *fileName, String initString)
{
    // Initialize file.
    int initResult = initFile(fileName);

    // Result = -1 = error.
    if (initResult == -1)
        Serial.println("Error Occured on initializing file.");

    // Result = 0 = File created - write initial string.
    if (initResult == 0)
        writeFile(fileName, initString);

    return initResult;
}

// #############################################################
/**
 * @brief Writes data to the file (overwriting).
 *
 * @param filePathName
 * @param data
 * @return true
 * @return false
 */
bool writeFile(const char *filePathName, String data)
{
    // Open file for writing (truncate to zero length).
    int fileDescriptor = open(filePathName, O_WRONLY | O_TRUNC);

    // Check for errors.
    if (fileDescriptor == -1)
        Serial.printlnf("Error opening file: %d", errno);
    else
    {
        if (write(fileDescriptor, data.c_str(), data.length()) == -1)
            Serial.printlnf("Error writing to file: %d", errno);
    }

    close(fileDescriptor);

    // Return true, if no errors
    if (errno)
        return false;
    else
        return true;
}

// #############################################################
/**
 * @brief Function for fetching data in the indicated file.
 *
 * @param filePathName
 * @return String
 */
String loadFile(const char *filePathName)
{
    String returnString;

    // Open file
    int fileDescriptor = open(filePathName, O_RDONLY);

    // Check for errors.
    if (fileDescriptor == -1)
        Serial.printlnf("Error opening file: %d", errno);
    else
    {
        // Use fstat to determine file size
        struct stat statBuffer;
        if (fstat(fileDescriptor, &statBuffer) == -1)
            Serial.printlnf("Error getting stats: %d", errno);
        else
        {
            // Read data.
            char data[statBuffer.st_size + 1] = {};
            if (read(fileDescriptor, data, statBuffer.st_size) == -1)
                Serial.printlnf("Error reading file: %d", errno);
            else
                returnString = String(data);
        }
    }

    close(fileDescriptor);

    // If no errors, return data.
    if (errno)
        return "";
    else
        return returnString;
}
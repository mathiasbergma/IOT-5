#include "Storage.h"
#include <cerrno>
#include <fcntl.h>

// ########################### Prototypes #################################
bool initLastWrite();
bool initWhFiles();
String loadFileData(const char *filePathName);
bool writeWattHours(const char *filePathName, String data);
bool updateLastWrite();

// ######################## Function Implementations #####################

// ###################################################
// Function for initalizing storage files.
// Run this during "setup".
// ###################################################
bool initStorage(String *whToday, String *whYesterday)
{

    if (initLastWrite() && initWhFiles())
    {
        // If last write happened today, read files.
        uint16_t lastWriteDay = getLastWrite();
        uint16_t dayNow = Time.format(Time.now(), "%j").toInt();
        if (lastWriteDay == dayNow)
        {
            *whToday = loadWhToday();
            *whYesterday = loadWhYesterday();
        }
        else if (lastWriteDay == dayNow - 1)
        {
            *whYesterday = loadWhToday();
        }
        return true;
    }
    else
        return false;
}

// ###################################################
// Function initializes the lastWrite.txt file to "0"
// It's holding the current day number of the year,
// i.e. 1 - 364
// ###################################################
bool initLastWrite()
{
    bool initOK = true;

    // Open file (r/w). Create if not existing.
    int fdLastWrite = open("/lastWrite.txt", O_RDWR | O_CREAT);

    // Check for errors.
    if (fdLastWrite == -1)
    {

        Serial.printlnf("Error initializing lastWrite.txt file %d", errno);
        initOK = false;
    }
    else
    {
        // File was just created - write "0" to it.
        uint16_t initData = 0;
        if (write(fdLastWrite, &initData, sizeof(initData)) == -1)
        {
            Serial.printlnf("Error writing initial data to lastWrite.txt: %d", errno);
            initOK = false;
        }
    }
    close(fdLastWrite);
    return initOK;
}

// ##############################################
// Function initializes the wattHour files.
// Creates them if not existing.
// ##############################################
bool initWhFiles()
{
    bool initOK = true;

    // Open files (r/w). Create if not existing.
    int fdWhToday = open("/wattHourToday.txt", O_RDWR | O_CREAT);
    int fdWhYesterday = open("/wattHourYesterday.txt", O_RDWR | O_CREAT);

    // Check for errors.
    if (fdWhToday == -1 || fdWhYesterday == -1)
    {
        Serial.printlnf("Error opening file %d", errno);
        initOK = false;
    }
    close(fdWhToday);
    close(fdWhYesterday);
    return initOK;
}

// ###################################################
// Function that returns the last day# of saving data.
// ###################################################
uint16_t getLastWrite()
{
    // String returnString;
    uint16_t day;

    // Open file
    int fdLastWrite = open("/lastWrite.txt", O_RDONLY);

    // Check for errors.
    if (fdLastWrite == -1)
        Serial.printlnf("Error opening lastWrite.txt: %d", errno);
    else
    {
        // Read data.
        if (read(fdLastWrite, &day, sizeof(day)) == -1)
            Serial.printlnf("Error reading file: %d", errno);
    }

    close(fdLastWrite);

    // If no errors, return data.
    if (errno)
        return 0;
    else
        return day;
}

// #############################################################
// Write current time to the lastWrite.txt file.
// #############################################################
bool updateLastWrite()
{
    // Open file - truncate length to zero.
    int fdLastWrite = open("/lastWrite.txt", O_WRONLY | O_TRUNC);

    // Check for errors.
    if (fdLastWrite == -1)
        Serial.printlnf("Error opening lastWrite.txt: %d", errno);
    else
    {
        // Get current day number.
        uint16_t day = Time.format(Time.now(), "%j").toInt();

        if (write(fdLastWrite, &day, sizeof(day)) == -1)
            Serial.printlnf("Error writing to file: %d", errno);
    }

    close(fdLastWrite);

    // Return true, if no errors
    if (errno)
        return false;
    else
        return true;
}

// #############################################################
// Fetch the saved wattHourToday data.
// #############################################################
String loadWhToday()
{
    return loadFileData("/wattHourToday.txt");
}

// #############################################################
// Fetch the saved wattHourYesterday data.
// #############################################################
String loadWhYesterday()
{
    return loadFileData("/wattHourYesterday.txt");
}

// #############################################################
// Function for fetching data in the indicated file.
// #############################################################
String loadFileData(const char *filePathName)
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
            char data[statBuffer.st_size];
            if (read(fileDescriptor, data, statBuffer.st_size) == -1)
                Serial.printlnf("Error reading file: %d", errno);
            else
                returnString = data;
        }
    }

    close(fileDescriptor);

    // If no errors, return data.
    if (errno)
        return "";
    else
        return returnString;
}

// #############################################################
// Write the wattHourToday data.
// #############################################################
bool writeWhToday(String whToday)
{
    return (writeWattHours("/wattHourToday.txt", whToday));
}

// #############################################################
// Write the wattHourYesterday data.
// #############################################################
bool writeWhYesterday(String whYesterday)
{
    return (writeWattHours("/wattHourYesterday.txt", whYesterday));
}

// #############################################################
// Function for writing data to the indicated file.
// #############################################################
bool writeWattHours(const char *filePathName, String data)
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
    {
        // Update "lastWrite" file, then return.
        updateLastWrite();
        return true;
    }
}

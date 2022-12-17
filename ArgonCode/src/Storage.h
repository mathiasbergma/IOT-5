#pragma once

#include "application.h"

int initFile(const char *fileName);
int initFile(const char *fileName, String initString);
bool writeFile(const char *filePathName, String data);
String loadFile(const char *filePathName);
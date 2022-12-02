#pragma once

#include "application.h"

bool initStorage(String *whToday, String *whYesterday);
String loadWhToday();
String loadWhYesterday();
bool writeWhToday(String whToday);
bool writeWhYesterday(String whYesterday);
uint16_t getLastWrite();

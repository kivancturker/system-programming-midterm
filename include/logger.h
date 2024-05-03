#ifndef LOGGER_H
#define LOGGER_H


static const char *LOG_FILE = "log.txt";

void createLogFile();
void writeLog(char *message);

#endif // LOGGER_H

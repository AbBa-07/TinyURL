#ifndef TINYURL_H
#define TINYURL_H

#define TABLE_SIZE 101

struct URLRecord
{
    char shortCode[20];
    char originalURL[500];
    int clickCount;
    struct URLRecord *next;
};

void initializeTable(void);
int hashFunction(const char code[]);
void generateShortCode(char shortCode[]);

void insertURL(const char originalURL[]);
int createURL(const char originalURL[], char shortCode[]);

struct URLRecord *searchURL(const char shortCode[]);
void retrieveURL(const char shortCode[]);
void deleteURL(const char shortCode[]);
void displayAllURLs(void);

void loadData(void);
void saveData(void);
void freeTable(void);

#endif
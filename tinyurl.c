#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinyurl.h"

#define DATA_FILE "tinyurl.dat"

struct URLRecord *hashTable[TABLE_SIZE];

struct StoredRecord
{
    char shortCode[20];
    char originalURL[500];
    int clickCount;
};

static int validURL(const char url[])
{
    if (url == NULL)
        return 0;

    if (strncmp(url, "http://", 7) != 0 &&
        strncmp(url, "https://", 8) != 0)
        return 0;

    if (url[7] == '\0' && strncmp(url, "http://", 7) == 0)
        return 0;

    if (url[8] == '\0' && strncmp(url, "https://", 8) == 0)
        return 0;

    for (int i = 0; url[i] != '\0'; i++)
    {
        unsigned char ch = (unsigned char)url[i];

        if (ch <= 32 || ch == 127)
            return 0;
    }

    return 1;
}

void initializeTable(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
        hashTable[i] = NULL;
}

int hashFunction(const char code[])
{
    unsigned long hash = 0;

    for (int i = 0; code[i] != '\0'; i++)
        hash = (hash * 31 + (unsigned char)code[i]) % TABLE_SIZE;

    return (int)hash;
}

void generateShortCode(char shortCode[])
{
    static unsigned long long counter = 1000;

    do
    {
        snprintf(shortCode, 20, "%llu", counter++);
    }
    while (searchURL(shortCode) != NULL);
}

struct URLRecord *searchURL(const char shortCode[])
{
    int index = hashFunction(shortCode);
    struct URLRecord *current = hashTable[index];

    while (current != NULL)
    {
        if (strcmp(current->shortCode, shortCode) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}

int createURL(const char originalURL[], char shortCode[])
{
    if (!validURL(originalURL) || strlen(originalURL) >= 500)
        return 0;

    struct URLRecord *newNode = malloc(sizeof(struct URLRecord));

    if (newNode == NULL)
        return 0;

    generateShortCode(newNode->shortCode);

    strcpy(newNode->originalURL, originalURL);
    newNode->clickCount = 0;

    int index = hashFunction(newNode->shortCode);

    newNode->next = hashTable[index];
    hashTable[index] = newNode;

    strcpy(shortCode, newNode->shortCode);

    saveData();

    return 1;
}

void insertURL(const char originalURL[])
{
    char shortCode[20];

    if (createURL(originalURL, shortCode))
        printf("\nShort URL code: %s\n", shortCode);
    else
        printf("\nInvalid URL or memory allocation failed!\n");
}

void retrieveURL(const char shortCode[])
{
    struct URLRecord *result = searchURL(shortCode);

    if (result != NULL)
    {
        result->clickCount++;

        printf("\nOriginal URL: %s\n", result->originalURL);
        printf("Click Count: %d\n", result->clickCount);

        saveData();
    }
    else
    {
        printf("\nURL not found!\n");
    }
}

void deleteURL(const char shortCode[])
{
    int index = hashFunction(shortCode);

    struct URLRecord *current = hashTable[index];
    struct URLRecord *previous = NULL;

    while (current != NULL)
    {
        if (strcmp(current->shortCode, shortCode) == 0)
        {
            if (previous == NULL)
                hashTable[index] = current->next;
            else
                previous->next = current->next;

            free(current);
            saveData();

            printf("\nURL deleted successfully!\n");
            return;
        }

        previous = current;
        current = current->next;
    }

    printf("\nURL not found!\n");
}

void displayAllURLs(void)
{
    int found = 0;

    printf("\n========== STORED URLS ==========\n");

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        struct URLRecord *current = hashTable[i];

        while (current != NULL)
        {
            printf("\nShort Code: %s\n", current->shortCode);
            printf("Original URL: %s\n", current->originalURL);
            printf("Click Count: %d\n", current->clickCount);

            found = 1;
            current = current->next;
        }
    }

    if (!found)
        printf("No URLs stored.\n");

    printf("=================================\n");
}

void saveData(void)
{
    FILE *file = fopen(DATA_FILE, "wb");

    if (file == NULL)
    {
        printf("Error: Could not save data!\n");
        return;
    }

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        struct URLRecord *current = hashTable[i];

        while (current != NULL)
        {
            struct StoredRecord record;

            strcpy(record.shortCode, current->shortCode);
            strcpy(record.originalURL, current->originalURL);
            record.clickCount = current->clickCount;

            if (fwrite(&record, sizeof(record), 1, file) != 1)
            {
                fclose(file);
                return;
            }

            current = current->next;
        }
    }

    fclose(file);
}

void loadData(void)
{
    FILE *file = fopen(DATA_FILE, "rb");

    if (file == NULL)
        return;

    struct StoredRecord record;

    while (fread(&record, sizeof(record), 1, file) == 1)
    {
        record.shortCode[19] = '\0';
        record.originalURL[499] = '\0';

        if (record.shortCode[0] == '\0' ||
            record.originalURL[0] == '\0' ||
            searchURL(record.shortCode) != NULL)
            continue;

        struct URLRecord *newNode = malloc(sizeof(struct URLRecord));

        if (newNode == NULL)
            break;

        strcpy(newNode->shortCode, record.shortCode);
        strcpy(newNode->originalURL, record.originalURL);

        newNode->clickCount = record.clickCount;

        int index = hashFunction(newNode->shortCode);

        newNode->next = hashTable[index];
        hashTable[index] = newNode;
    }

    fclose(file);
}

void freeTable(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        struct URLRecord *current = hashTable[i];

        while (current != NULL)
        {
            struct URLRecord *temp = current;
            current = current->next;
            free(temp);
        }

        hashTable[i] = NULL;
    }
}
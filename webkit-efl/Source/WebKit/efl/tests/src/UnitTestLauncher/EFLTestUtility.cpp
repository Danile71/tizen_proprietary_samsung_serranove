#include "EFLTestUtility.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

namespace EFLUnitTests {

char* EFLTestUtility::composeUrlPath(const char* currentDirPath, const char* relativePath)
{
    const int currentDirLen = strlen(currentDirPath);

    const char* protocol = "file://";
    const int protocolLen = strlen(protocol);

    const int relativePathLen = strlen(relativePath);

    char* searchPath = (char*)malloc(protocolLen + currentDirLen + relativePathLen + 1);
    strcpy(searchPath, protocol);
    strcat(searchPath, currentDirPath);
    strcat(searchPath, relativePath);

    return searchPath;
}

char* EFLTestUtility::createDefaultUrlPath(UtilitySearchPathOptions option)
{
    char* currentDirPath = get_current_dir_name();
    if (!currentDirPath)
        return 0;

    static const char* dirs[] = {"Source", "Programs"};
    char realPathLevel[6] = {0};

    char* baseName = basename(currentDirPath);

    if (!strcmp(baseName, dirs[1]))
        strcpy(realPathLevel, "../..");
    else if (!strcmp(baseName, dirs[0]))
        strcpy(realPathLevel, "..");

    if (realPathLevel[0] != '\0') {
        free(currentDirPath);
        currentDirPath = realpath(realPathLevel, 0);
    }

    char* searchPath = 0;

    if (option == DomBindingsTestDocument)
        searchPath = composeUrlPath(currentDirPath, DOM_BINDINGS_TEST_DOCUMENT);
    else if (option == DefaultTestPage)
        searchPath = composeUrlPath(currentDirPath, DEFAULT_TEST_PAGE);

    free(currentDirPath);
    return searchPath;
}

}

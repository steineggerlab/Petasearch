// Created by matchy on 2/1/22.
#ifndef SRASEARCH_SRAUTIL_H
#include "BaseMatrix.h"

#include <string>
#include <vector>

namespace SRAUtil {
    /**
     * @brief Reverse a string and put the results in strRev
     * @param strRev the destination to put the reversed string
     * @param str the string to be revesed
     * @param len the length of the string
     */
    inline void strrev(char *strRev, const char *str, int len) {
        int start = 0;
        int end = len - 1;
        while (start <= end) {
            strRev[start] = str[end];
            strRev[end] = str[start];
            ++start;
            --end;
        }
        strRev[len] = '\0';
    }

/**
 * @brief Make a slice origStr[start:end], start inclusive, end exclusive
 * */
    char *substr(char *origStr, unsigned int start, unsigned int end);

/**
 * @brief Strip invalid characters from a string (@, *, newline, tab, etc)
 */
    void stripInvalidChars(const char *src, char *dest);

    void stripInvalidChars(char *src);

    std::vector<std::string> getFileNamesFromFile(const std::string &filename);

    std::string extractProfileSequence(const char *seqData, size_t seqLen, BaseMatrix *subMat);

}
#define SRASEARCH_SRAUTIL_H

#endif

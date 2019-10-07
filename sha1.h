#include <string>
#include <unordered_map> 
#include <dirent.h>
#ifndef SHA1_H
#define SHA1_H

const int SHA1_LEN = 21;

/*
 * computeChecksum
 * computes SHA1 checksum of the given file
 * Args: 
 * * filename: name of filename
 * * hash: a Pass-By-Reference null-terminated char array containing the 
 * * SHA1 hash of the file
 *
 * Return: None
 * Assumptions: filename refers to a file which exists
 */
void computeChecksum(std::string filename, unsigned char (&hash)[SHA1_LEN]);
void checkDirectory(char *dirname);
bool isFile(std::string fname);
std::string makeFileName(std::string dir, std::string name);
void fillChecksumTable(std::unordered_map<std::string, std::string> &filehash,
                       DIR *SRC, const char* sourceDir);

#endif

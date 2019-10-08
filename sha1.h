#include <string>
#include <map> 
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

/*
 * checkDirectory
 * Makes sure directory eists
 * Args:
 * * char* dirname
 *
 * Return: None, exits if dir does not exist
 */
void checkDirectory(char *dirname);

/*
 * isFile
 * Checks if fname is a file or dir
 * Args:
 * * string fname: name of suspected file
 *
 * Return: bool, if fname is the name of a file
 */
bool isFile(std::string fname);

/*
 * makeFileName
 * gets full filename
 * Args:
 * * string dir: directory name
 * * string name: file name 
 *
 * Return: string full name of file
 */
std::string makeFileName(std::string dir, std::string name);

/*
 * fillChecksumTable
 * Flls a directory checksum table mapping file names to SHA1 hashs
 * Args:
 * * map<string, string> &filehash: PBR An empty map\
 * * DIR* SRC: Pointer to the source dir
 * * const char* sourceDir: name of the source directory
 *
 * Return: None the map is pass-by-reference 
 */
void fillChecksumTable(std::map<std::string, std::string> &filehash,
                       DIR *SRC, const char* sourceDir);
/*
 * getDirHash
 * Computes the SHA1 hash of the entire directory
 * Args:
 * * map<string, string> filehash: A map mapping all dir files to SHA1 hashs
 *
 * Return string directory hash
 */
std::string getDirHash(std::map<std::string, std::string> filehash);

/*
 * printHash
 * Prints a SHA1 hash in a human readable form
 * Args:
 * *const unsigned char* hash: the hash to be printed
 * 
 * Return: none
  */
void printHash(const unsigned char *hash);

#endif

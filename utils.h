#include <string>
#include <map> 
#include <dirent.h>
#ifndef SHA1_H
#define SHA1_H

const int SHA1_LEN = 21;

/*
 * computeChecksum
 * computes SHA1 checksum of the given buffer
 * Args: 
 * * data: pointer to a buffer of data, assumed to be contents of a file
 * * size: number of bytes of information that data points to
 * * hash: a Pass-By-Reference null-terminated char array containing the 
 * * SHA1 hash of the file after the call to computeChecksum
 *
 * Return: None
 * Assumptions: Data points to an area of memory that is 'size' bytes long 
 */
void computeChecksum(const unsigned char *data, size_t size,
                     unsigned char (&hash)[SHA1_LEN]);

/*
 * trustedFileRead
 * Reads a desired file and returns its contents. Checks the authenticity of
 * the contents via voting method.
 * Args: 
 * * source_dir: name of the source directory of the file
 * * file_name: name of the file to be read
 * * size: pass-by-reference size_t that will be filled with the number of
 *         bytes read from the file
 *
 * Returns: pointer to a malloc'd array of bytes that contains the contents of
 *          the desired file.
 */
char *trustedFileRead(std::string source_dir, std::string file_name, size_t &size);

/*
 * getFileChecksum
 * computes SHA1 checksum of a given filename and stores it in a given unsigned
 * char array. Returns a pointer to the contents of the file. It is the
 * caller's responsibility to free the memory malloc'd by this function.
 * Args:
 * * source_name: the name of a directory that exists
 * * file_name: the name of a file that exists in that directory
 * * size:      pass-by-reference size_t that will store the number of bytes
 *              of data pointed to by the return value of the function
 * * hash:      pass-by-reference array of unsigned chars that will be used to
 *              store the hash value of the file
 * 
 * Returns: A pointer to a block of malloc'd memory that is 'size' bytes long
 *          and contains the contents of the desired file
 *
 */
char *getFileChecksum(std::string source_name, std::string file_name,
                      size_t &size, unsigned char (&hash)[SHA1_LEN]);

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

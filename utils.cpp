/*
 * endtoend.cpp: Implements a function for directory checksum comparison
 * Written by: Dylan Hoffmann and Lucas Campbell
 */

#include "utils.h"
#include "c150nastyfile.h"        // for c150nastyfile & framework
#include "c150grading.h"
#include <string>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>                // for errno string formatting
#include <cerrno>
#include <iostream>               // for cout
#include <fstream>                // for input files 

int FILE_NASTINESS;
int NETWORK_NASTINESS;


using namespace std;
using namespace C150NETWORK;

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
                     unsigned char (&hash)[SHA1_LEN])
{
    SHA1(data, size, hash);
    hash[SHA1_LEN-1] = '\0';
}

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
char *trustedFileRead(string source_dir, string file_name, size_t &size)
{
    // Put together directory and filenames SRC/file TARGET/file
    string source_name = makeFileName(source_dir, file_name);

    bool found_match = false;
    int correct_index;
    string hashes[3];
    char *file_buffs[2];

    while (!found_match) {
        for (int i = 0; i < 3; i++) {
            //  Misc variables, mostly for return codes
            void *fopenretval;
            string errorString;
            char *buffer;
            unsigned char hash[SHA1_LEN];
            struct stat statbuf;  
            size_t src_size;

            // Read whole input file 
            if (lstat(source_name.c_str(), &statbuf) != 0) {
                fprintf(stderr,"trustedFileRead: error stating supplied source"
                        "file %s\n", source_name.c_str());
                exit(20);
            }
            // Make an input buffer large enough for
            // the whole file
            src_size = statbuf.st_size;
            buffer = (char *)malloc(src_size);
            if (i != 2) { //not the 3rd file opening
                file_buffs[i] = buffer;
            }

            NASTYFILE inputFile(FILE_NASTINESS);

            // do an fopen on the input file
            fopenretval = inputFile.fopen(source_name.c_str(), "rb");  
          
            if (fopenretval == NULL) {
              cerr << "Error opening input file " << source_name << 
                      " errno=" << strerror(errno) << endl;
              exit(12);
            }
            // Read the whole file
            size = inputFile.fread(buffer, 1, src_size);
            if (size != src_size) {
              cerr << "Error reading file " << source_name << 
                      "  errno=" << strerror(errno) << endl;
              exit(16);
            }
            // Close the file
            if (inputFile.fclose() != 0 ) {
              cerr << "Error closing input file " << source_name << 
                      " errno=" << strerror(errno) << endl;
              exit(16);
            }

            computeChecksum((const unsigned char *)buffer, size, hash);
            hashes[i] = string((const char *)hash);
            // Only need to store first 2 buffers
            if (i == 2)
                delete buffer;
        }
        // check to see if any checksums match
        if ((hashes[0] == hashes[1]) || (hashes[0] == hashes[2])) {
            delete file_buffs[1];
            correct_index = 0;
            found_match = true;
        } else if (hashes[1] == hashes[2]) {
            delete file_buffs[0];
            correct_index = 1;
            found_match = true;
        }
        else {
            delete file_buffs[0];
            delete file_buffs[1];
        }
    }
    return file_buffs[correct_index];
}

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
char *getFileChecksum(string source_name, string file_name, size_t &size,
                        unsigned char (&hash)[SHA1_LEN])
{
    char *file_data = trustedFileRead(source_name, file_name, size);
    computeChecksum((const unsigned char *)file_data, size, hash);
    return file_data;
}


// ------------------------------------------------------
//
//                   checkDirectory
//
//  Make sure directory exists
//     
// ------------------------------------------------------

void checkDirectory(char *dirname)
{
  struct stat statbuf;  
  if (lstat(dirname, &statbuf) != 0) {
    fprintf(stderr,"Error stating supplied source directory %s\n", dirname);
    exit(8);
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    fprintf(stderr,"File %s exists but is not a directory\n", dirname);
    exit(8);
  }
}

// ------------------------------------------------------
//
//                   isFile
//
//  Make sure the supplied file is not a directory or
//  other non-regular file.
//     
// ------------------------------------------------------

bool isFile(string fname)
{
  const char *filename = fname.c_str();
  struct stat statbuf;  
  if (lstat(filename, &statbuf) != 0) {
      fprintf(stderr,"isFile: Error stating supplied source file %s\n",
              filename);
    return false;
  }

  if (!S_ISREG(statbuf.st_mode)) {
    fprintf(stderr,"isFile: %s exists but is not a regular file\n", filename);
    return false;
  }
  return true;
}


// ------------------------------------------------------
//
//                   makeFileName
//
// Put together a directory and a file name, making
// sure there's a / in between
//
// ------------------------------------------------------

string makeFileName(string dir, string name) 
{
    stringstream ss;

  ss << dir;
  // make sure dir name ends in /
  if (dir.substr(dir.length()-1,1) != "/")
    ss << '/';
  ss << name;     // append file name to dir
  return ss.str();  // return dir/name
  
}


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
void fillChecksumTable(map<string, string> &filehash,
                        DIR *SRC, const char* sourceDir)
{
    struct dirent *sourceFile;  // Directory entry for source file
    while ((sourceFile = readdir(SRC)) != NULL) {

            if ( (strcmp(sourceFile->d_name, ".") == 0) ||
                 (strcmp(sourceFile->d_name, "..")  == 0 ) ) 
                continue;          // never copy . or ..

            string full_filename = makeFileName(sourceDir, sourceFile->d_name);
            string filename = sourceFile->d_name;

            // check that is a regular file
            if (!isFile(full_filename))
                 continue;                     
            // add {filename, checksum} to the table
            unsigned char hash[SHA1_LEN];
            size_t size; //throwaway
            getFileChecksum(string(sourceDir), filename, size, hash);
            
            string hash_str = string((const char*)hash);
            filehash[filename] = hash_str;
    }
}

/*
 * printHash
 * Prints a SHA1 hash in a human readable form
 * Args:
 * *const unsigned char* hash: the hash to be printed
 * 
 * Return: none
 */
void printHash(const unsigned char *hash)
{
    for (int i = 0; i < SHA1_LEN-1; i++)
    {
        printf ("%02x", (unsigned int) hash[i]);
    }
}

/*
 *  NB: While the use of tmpname generates compiler warnings, we have elected
 *  to use it because the preferred alternative, 'mkstemp' is less broadly
 *  portable. We plan to engineer this check to use a different method for the
 *  final iteration of this assignment.
 *
 * getDirHash
 * Writes the file checksum map to a file (outside src) and calculates
 * the SHA1 checksum of that file, which equates to the SHA1 checksum of
 * the source directory
 *
 * Args:
 * * &unordered_map<str, str>: the hashmap of filename:SHA1hash pairs
 *
 * Return: string which is the directory hash
 */ 
string getDirHash(map<string, string> filehash)
{
    return ":)";
//    string file = tmpnam(nullptr);
//    ofstream stream(file);
//    for(auto& kv : filehash) {
//        stream << kv.second << endl;
//    }
//    unsigned char hash[SHA1_LEN];
//    size_t size; //throwaway
//    getFileChecksum(string("."), file, size, hash);
//    string hash_str = string((const char*)hash);
//    stream.close();
//    cout << file << endl;
//    remove(file.c_str());
//    return hash_str;
}

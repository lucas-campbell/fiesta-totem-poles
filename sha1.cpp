/*
 * endtoend.cpp: Implements a function for directory checksum comparison
 * Written by: Dylan Hoffmann and Lucas Campbell
 */

#include "sha1.h"
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

using namespace std;

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
void computeChecksum(string filename, unsigned char (&hash)[SHA1_LEN])
{
    ifstream *t;
    stringstream *buffer;
    t = new ifstream(filename);
    buffer = new stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), hash);
    hash[SHA1_LEN-1] = '\0';
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
            computeChecksum(filename, hash);
            
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
    string file = tmpnam(nullptr);
    ofstream stream(file);
    for(auto& kv : filehash) {
        stream << kv.second << endl;
    }
    unsigned char hash[SHA1_LEN];
    computeChecksum(file, hash);
    string hash_str = string((const char*)hash);
    stream.close();
    cout << file << endl;
    remove(file.c_str());
    return hash_str;
}

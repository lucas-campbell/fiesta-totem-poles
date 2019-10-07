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
#include <unordered_map> 

using namespace std;


void computeChecksum(std::string filename, unsigned char (&hash)[SHA1_LEN])
{
    ifstream *t;
    std::stringstream *buffer;
    t = new ifstream(filename);
    buffer = new std::stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), hash);
    hash[20] = '\0';
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

bool isFile(std::string fname)
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

std::string makeFileName(std::string dir, std::string name) 
{
    std::stringstream ss;

  ss << dir;
  // make sure dir name ends in /
  if (dir.substr(dir.length()-1,1) != "/")
    ss << '/';
  ss << name;     // append file name to dir
  return ss.str();  // return dir/name
  
}

void fillChecksumTable(std::unordered_map<std::string, std::string> &filehash,
                        DIR *SRC, const char* sourceDir)
{
    struct dirent *sourceFile;  // Directory entry for source file
    while ((sourceFile = readdir(SRC)) != NULL) {

            if ( (strcmp(sourceFile->d_name, ".") == 0) ||
                 (strcmp(sourceFile->d_name, "..")  == 0 ) ) 
                continue;          // never copy . or ..

            std::string filename = makeFileName(sourceDir, sourceFile->d_name);
            
            // check that is a regular file
            if (!isFile(filename))
                 continue;                     
            // add {filename, checksum} to the table
            unsigned char hash[SHA1_LEN];
            computeChecksum(filename, hash);
            
            std::string hash_str = std::string((const char*)hash);
            filehash[filename] = hash_str;
    }
}

void printHash(char *hash)
{
    for (i = 0; i < 20; i++)
    {
        printf ("%02x", (unsigned int) hash[i]);
    }
}

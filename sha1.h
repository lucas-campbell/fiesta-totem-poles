#ifndef SHA1_H
#define SHA1_H
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
void computeChecksum(std::string filename, unsigned char (&hash)[21]);

#endif

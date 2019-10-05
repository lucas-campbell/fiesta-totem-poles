/*
 * protocol.h: Interface for UDP file copy protocol
 * Written By Dylan Hoffmann & Lucas Campbell
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
/*
 * makeFilePilot
 * Constructs the pilot packet for files
 * Args:
 * * int num_packets: # packets for this file = file_size / PACKET_DATA_SIZE
 * * string fname: name of the file
 * * int file_ID: numerical id of this file (incrememntal)
 * * string hash: SHA1 hash of file contents
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 */  
std::string makeFilePilot(int num_packets, std::string fname, int file_ID,
                         std::string hash);

/*
 * makeDirPilot
 * Constructs the pilot packet for directories
 * Args:
 * * int num_files: # files in this directory
 * * string target: name of copy target directory
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 */  
std::string makeDirPilot(int num_files, std::string TARGET);

/*
 * makeFilePacket
 * Constructs the data packet for files
 * Args:
 * * int packet_num: which packet in order this packet is TODO: fix this <--
 * * int file_ID: numerical id of this file (incrememntal)
 * * string data: file data payload -- 420 bytes except final packet
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 * Additional info: the data payload is always 420 bytes except for the
 * final packet for a file, which may be shorter
 */  
std::string makeFilePacket(int packet_num, int file_ID, std::string data);

#endif

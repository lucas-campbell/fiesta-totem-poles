/*
 * protocol.h: Interface for UDP file copy network protocols protocol
 * Written By Dylan Hoffmann & Lucas Campbell
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include<string>

// number of digits allowed for number of files
const int MAX_FILENUM = 7; 
// number of digits allowed for number of packets
const int MAX_PACKNUM = 7; 
// number of fields in file pilot packet, including packet type
const int FILE_PILOT_FIELDS = 5; 


/*
 * FilePilot
 * The pilot packet for files
 * Constructor args:
 * * int num_packets: # packets for this file = file_size / PACKET_DATA_SIZE
 * * int file_ID: numerical id of this file (incrememntal)
 * * string hash: SHA1 hash of file contents
 * * string fname: name of the file
 * Assumptions: TODO
 */  
struct FilePilot {
    int num_packets;
    int file_ID;
    std::string hash;
    std::string fname;
    FilePilot(int p, int i, std::string h, std::string f) :
        num_packets(p), file_ID(i), hash(h), fname(f) {}
};

/*
 * Args: a struct containing pilot packet info for a file
 * Returns: a string - packet with metadata of the pilot packet
 * */
std::string makeFilePilot(FilePilot pilot_packet);

/*
 * Args: a string containing pilot packet metadata
 * Returns: a corresponding FilePilot struct with the same metadata
 * */
FilePilot unpackFilePilot(std::string packet);


//////////////////
/*
 * DirPilot
 * The pilot packet for directories
 * Constructor args:
 * * int num_files: # files in this directory
 * * string hash: SHA1 hash of the directory
 * Assumptions: TODO
 */  
struct DirPilot {
    int num_files;
    std::string hash;
    DirPilot(int n, std::string h) :
        num_files(n), hash(h) {}
};

/*
 * Args: a struct containing pilot packet info for a directory
 * Returns: a string - packet with metadata of the pilot packet
 * */
std::string makeDirPilot(DirPilot pilot_packet);

/*
 * Args: a string containing pilot packet metadata
 * Returns: a corresponding FilePilot struct with the same metadata
 * */
DirPilot unpackDirPilot(std::string packet);


///////////////////
/*
 * FilePacket
 * The data packet for files
 * Constructor args:
 * * int packet_num: integer indicating where the data contained fits in with
 *                   the rest of the file data
 * * int file_ID: numerical id of this file (incrememntal)
 * * string data: file data payload -- 420 bytes except final packet
 * Assumptions: TODO
 * Additional info: the data payload is always 420 bytes except for the
 * final packet for a file, which may be shorter
 */  
struct FilePacket {
    int packet_num;
    int file_ID;
    std::string data;
    FilePacket(int p, int f, std::string d) :
        packet_num(p), file_ID(f), data(d) {}
};

/*
 * Args: a struct containing info for a single data packet
 * Returns: a string - packet with metadata of the pilot packet
 * */
std::string makeFilePacket(FilePacket packet);

/*
 * Args: a string containing pilot packet metadata
 * Returns: a corresponding FilePilot struct with the same metadata
 * */
FilePacket unpackFilePacket(std::string packet);


#endif

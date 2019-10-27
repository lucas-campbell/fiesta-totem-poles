// --------------------------------------------------------------
//
//                        filecopyserver.cpp
//
//        Authors: Lucas Campbell and Dylan Hoffmann
//
//        - adapted from pingserver.cpp, written by Noah Mendelsohn
//   

//
//
//        COMMAND LINE
//
//          fileserver <networknastiness> <filenastiness> <targetdir>
//
//
//        OPERATION
//
//              filecopy server will wait until receiving a directory
//              pilot packet, set up the file environment, and then
//              begin receiving file specific packets. As each packet
//              will be numbered it will ignore any duplicates it
//              receives and fill in the file data as packets arrive.
//             There will likely be additional logic to request packets,
//             and to handle multiple files at once, but as we haven't
//             encountered any issues yet we don't have any concrete
//             plans to address them.
//
//
//     
// --------------------------------------------------------------

#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150debug.h"
#include "utils.h"
#include "protocol.h"
#include <fstream>
#include <set>
#include <map>
#include <vector>


using namespace C150NETWORK;  // for all the comp150 utilities 
using namespace std;

// Forward declarations
void setUpDebugLogging(const char *logname, int argc, char *argv[]);
DirPilot receiveDirPilot(C150NastyDgmSocket *sock);
void receiveFile(C150NastyDgmSocket *sock, string incoming,
                 vector<string> &failed_e2es, map<string, string> &filehash);
void receiveDataPackets(C150NastyDgmSocket *sock, FilePacket first_packet,
                        FilePilot file_pilot, vector<string> &failed_e2es,
                        map<string, string> &filehash);
bool internalE2E(string file_data, FilePilot file_pilot,
                 map<string, string> &filehash);
void sendE2E(C150NastyDgmSocket *sock, vector<string> failed,
             map<string, string> filehash, DirPilot dir_pilot);


/********** Global Constants **********/
const int NETWORK_NASTINESS_ARG = 1;
const int FILE_NASTINESS_ARG = 2;
const int TARGET_ARG = 3;
extern int NETWORK_NASTINESS;
extern int FILE_NASTINESS;
// max number of attempts to write file to disk
const int MAX_WRITE_TRIES = 5; 
string TARGET_DIR;



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                           main program
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 
int
main(int argc, char *argv[])
{
    GRADEME(argc, argv);

    //
    // Check command line and parse arguments
    //
    if (argc != 4)  {
        fprintf(stderr,"Correct syntax is: %s <network nastiness>"
                        "<file nastiness> <target directory>\n", argv[0]);
        exit(1);
    }
    if (strspn(argv[NETWORK_NASTINESS_ARG], "0123456789") != 
        strlen(argv[NETWORK_NASTINESS_ARG])) {
        fprintf(stderr,"Nastiness %s is not numeric\n",
                argv[NETWORK_NASTINESS_ARG]);     
        fprintf(stderr,"Correct syntax is: %s <network nastiness>"
                        "<file nastiness> <target directory>\n", argv[0]);
        exit(4);
    }
    if (strspn(argv[FILE_NASTINESS_ARG], "0123456789") !=
        strlen(argv[FILE_NASTINESS_ARG])) {
        fprintf(stderr,"Nastiness %s is not numeric\n",
                argv[FILE_NASTINESS_ARG]);     
        fprintf(stderr,"Correct syntax is: %s <network nastiness>"
                        "<file nastiness> <target directory>\n", argv[0]);
        exit(4);
    }
    //
    // Variable declarations
    //
    ssize_t readlen;             // amount of data read from socket
    char incoming_msg[512];   // received message data
    DIR* TRG;
    // map of filenames to checksums, as they exist written in target dir
    map<string, string> filehash; 
    // vector of filenames, to be reported to client as part of e2e check
    vector<string> failed_e2es;
    // convert command line args
    NETWORK_NASTINESS = atoi(argv[NETWORK_NASTINESS_ARG]);   
    FILE_NASTINESS = atoi(argv[FILE_NASTINESS_ARG]);   
    TARGET_DIR = string(argv[TARGET_ARG]);
       
    //
    //  Set up debug message logging
    //
    setUpDebugLogging("filecopyserverdebug.txt",argc, argv);

    //
    // Open the target directory
    //
    TRG = opendir(TARGET_DIR.c_str());
    if (TRG == NULL) {
        fprintf(stderr,"Error opening source directory %s\n",
                TARGET_DIR.c_str());
        exit(8);
    }

    //
    // We set a debug output indent in the server only, not the client.
    // That way, if we run both programs and merge the logs this way:
    //
    //    cat pingserverdebug.txt pingserverclient.txt | sort
    //
    // it will be easy to tell the server and client entries apart.
    //
    // Note that the above trick works because at the start of each
    // log entry is a timestamp that sort will indeed arrange in 
    // timestamp order, thus merging the logs by time across 
    // server and client.
    //
    c150debug->setIndent("    ");       // if we merge client and server
                                        // logs, server stuff will be indented

    
    //
    // Create socket, loop receiving and responding
    //
    try {
        //TODO add GRADING messages here
        cerr << "Creating C150NastyDgmSocket(nastiness=" <<
            NETWORK_NASTINESS <<endl;
        c150debug->printf(C150APPLICATION,"Creating "
                          "C150NastyDgmSocket(nastiness=%d)",
                          NETWORK_NASTINESS);
        C150NastyDgmSocket *sock = new C150NastyDgmSocket(NETWORK_NASTINESS);
        cerr << "ready to accept messages\n";
        c150debug->printf(C150APPLICATION,"Ready to accept messages");

        // Wait until client sends a DirPilot
        DirPilot dir_pilot = receiveDirPilot(sock);

        int num_files = dir_pilot.num_files;
        int received_files = 0;
        
        while (received_files < num_files) {
            // Read a message
            cout << "In while\n";
            readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
            if (readlen == 0) {
                c150debug->printf(C150APPLICATION,"Read zero length message,"
                                  " trying again");
                continue;
            }
            incoming_msg[readlen] = '\0'; // make sure null terminated
            cout << "readlen" << readlen << endl;
            printf("%s\n", incoming_msg);
            string incoming(incoming_msg); // Convert to C++ string
            c150debug->printf(C150APPLICATION,"Successfully read %d bytes."
                              " Message=\"%s\"", readlen, incoming.c_str());
            cout << incoming << endl;
            cout << incoming[0] << endl;
            // Check for FilePilot
            if (incoming[0] == 'P') {
                cout << "Case P\n";
                receiveFile(sock, incoming, failed_e2es, filehash); //TODO args
                received_files++;
            }
            // Resend confirmation of DirPilot if client appears to need it
            else if (incoming[0] == 'D') {
                cout << "Case D\n";
                string response = "DPOK";
                c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                                  response.c_str());
                sock -> write(response.c_str(), response.length()+1);
            }
            else
                continue;
        }
        // Send E2E and wait for response
        sendE2E(sock, failed_e2es, filehash, dir_pilot);
    }

    catch (C150NetworkException& e) {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG,"Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: "
             << e.formattedExplanation() << endl;
    }

    // This only executes if there was an error caught above
    return 4;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                     setUpDebugLogging
//
//        For COMP 150-IDS, a set of standards utilities
//        are provided for logging timestamped debug messages.
//        You can use them to write your own messages, but 
//        more importantly, the communication libraries provided
//        to you will write into the same logs.
//
//        As shown below, you can use the enableLogging
//        method to choose which classes of messages will show up:
//        You may want to turn on a lot for some debugging, then
//        turn off some when it gets too noisy and your core code is
//        working. You can also make up and use your own flags
//        to create different classes of debug output within your
//        application code
//
//        NEEDSWORK: should be factored and shared w/pingclient
//        NEEDSWORK: document arguments
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 
void setUpDebugLogging(const char *logname, int argc, char *argv[]) {

    //   
    //           Choose where debug output should go
    //
    // The default is that debug output goes to cerr.
    //
    // Uncomment the following three lines to direct
    // debug output to a file. Comment them to 
    // default to the console
    //  
    // Note: the new DebugStream and ofstream MUST live after we return
    // from setUpDebugLogging, so we have to allocate
    // them dynamically.
    //
    //
    // Explanation: 
    // 
    //     The first line is ordinary C++ to open a file
    //     as an output stream.
    //
    //     The second line wraps that will all the services
    //     of a comp 150-IDS debug stream, and names that filestreamp.
    //
    //     The third line replaces the global variable c150debug
    //     and sets it to point to the new debugstream. Since c150debug
    //     is what all the c150 debug routines use to find the debug stream,
    //     you've now effectively overridden the default.
    //
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);


    //
    //  Put the program name and a timestamp on each line of the debug log.
    //
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp(); 

    //
    // Ask to receive all classes of debug message
    //
    // See c150debug.h for other classes you can enable. To get more than
    // one class, you can or (|) the flags together and pass the combined
    // mask to c150debug -> enableLogging 
    //
    // By the way, the default is to disable all output except for
    // messages written with the C150ALWAYSLOG flag. Those are typically
    // used only for things like fatal errors. So, the default is
    // for the system to run quietly without producing debug output.
    //
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC | 
                             C150NETWORKDELIVERY); 
}

/*
 * receiveDirPilot
 * wait for a DirPilot packet from the server, return the information received
 *
 * Args:
 * * sock:  A C150NastyDgmSocket used to listen for messages
 *
 * Returns: DirPilot Struct
 */
DirPilot receiveDirPilot(C150NastyDgmSocket *sock)
{
    cout << "Receiving Dir Pilot\n";
    ssize_t readlen;             // amount of data read from socket
    char incoming_msg[512];   // received message data
    while (true) {
        readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
        if (readlen == 0) {
            c150debug->printf(C150APPLICATION,"Read zero length message,"
                              " trying again");
            continue;
        }
        incoming_msg[readlen] = '\0'; // make sure null terminated
        string incoming(incoming_msg); // Convert to C++ string
        c150debug->printf(C150APPLICATION,"Successfully read %d bytes."
                          " Message=\"%s\"", readlen, incoming.c_str());
        // Loop until we get a DirPilot
        if (incoming[0] != 'D')
            continue;
        DirPilot dir_pilot = unpackDirPilot(incoming);
        cout << incoming << endl;
        //
        // confirm to client that we received the DirPilot
        //
        string response = string("DPOK");
        c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                          response.c_str());
        sock -> write(response.c_str(), response.length()+1);
        cout << "Dir pilot received\n";
        return dir_pilot;
    }
    
}

/*
 * receiveFile
 * Listens for FilePilot and corresponding data packets on a given socket, 
 * writing the data to disk and making note if there was a failure to write
 * correctly (TODO).
 *
 * TODO args/return
 *
 */

void receiveFile(C150NastyDgmSocket *sock, string incoming,
                 vector<string> &failed_e2es, map<string, string> &filehash)
{
    cout << "Receiving File " << incoming << endl; 
    ssize_t readlen;             // amount of data read from socket
    char incoming_msg[512];   // received message data
    FilePilot file_pilot = unpackFilePilot(incoming);

    // Loop until we get a FilePacket
    while (true) {
        cout << "In receive file while\n";
        readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
        if (readlen == 0) {
            c150debug->printf(C150APPLICATION,"Read zero length message,"
                              " trying again");
            continue;
        }
        incoming_msg[readlen] = '\0'; // make sure null terminated
        string incoming(incoming_msg); // Convert to C++ string
        cout << "incoming " << incoming << endl;
        c150debug->printf(C150APPLICATION,"Successfully read %d bytes."
                          " Message=\"%s\"", readlen, incoming.c_str());
        // If we receive the same FilePilot again, the client did not get our
        // confirmation the first time so we re-send it
        if (incoming[0] == 'P') {
            FilePilot fp2 = unpackFilePilot(incoming);
            if (fp2.file_ID == file_pilot.file_ID) {
                string response = "FPOK" + to_string(fp2.file_ID);
                c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                                  response.c_str());
                sock -> write(response.c_str(), response.length()+1);
                continue;
            }
        }
        // Make sure the File Packet is for the correct file
        else if (incoming[0] == 'F') {
            FilePacket first_packet = unpackFilePacket(incoming);
            if (first_packet.file_ID == file_pilot.file_ID) {
                receiveDataPackets(sock, first_packet, file_pilot,
                                   failed_e2es, filehash);
            }
        }
        else
            continue;
    }
    cout << "File received\n";
}

void receiveDataPackets(C150NastyDgmSocket *sock, FilePacket first_packet,
                        FilePilot file_pilot, vector<string> &failed_e2es,
                        map<string, string> &filehash)
{
    cout << "Rece3iving Data Packets\n";
    ssize_t readlen;             // amount of data read from socket
    bool timedout = false;
    char incoming_msg[512];
    // To be filled with data from client
    string file_data(PACKET_SIZE*file_pilot.num_packets, ' ');
    set<int> packets;
    // Create set with packet_IDs of the packets we need to receive
    for (int i = 0; i < file_pilot.num_packets; i++) {
        packets.insert(packets.end(), i);
    }
    // insert the first data packet we received, take it out of the set
    int loc = first_packet.packet_num*PACKET_SIZE;
    file_data.insert(loc, first_packet.data);
    packets.erase(first_packet.packet_num);
    sock -> turnOnTimeouts(200); //TODO figure out good timeout for this

    // Construct a 'missing' file IDs string for the client
    string missing = "M" + to_string(file_pilot.file_ID) + " ";
    for (auto iter  = packets.begin(); iter != packets.end(); iter++) {
        missing += to_string(*iter);
        if (next(iter) != packets.end())
            missing += " ";
        //Don't overfill buffer!
        if (missing.length() + MAX_FILENUM + 1 > 512)
            break;
    }
    // Loop until there are no more packets left to be received
    do {
        while (!timedout) {
            readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
            timedout = sock -> timedout();
            if (timedout) {
                c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                                  missing.c_str());
                sock -> write(missing.c_str(), missing.length()+1);
                continue;
            }
            if (readlen == 0) {
                c150debug->printf(C150APPLICATION,"Read zero length message,"
                                  " trying again");
                continue;
            }
            incoming_msg[readlen] = '\0'; // make sure null terminated
            string incoming(incoming_msg); // Convert to C++ string
            c150debug->printf(C150APPLICATION,"Successfully read %d bytes."
                              " Message=\"%s\"", readlen, incoming.c_str());
            // Check that we got a File Pilot
            if (incoming[0] == 'F') { 
                FilePacket packet = unpackFilePacket(incoming);
                // Check for correct file_ID
                if (packet.file_ID == file_pilot.file_ID) {
                    // Add data to buffer
                    int loc = packet.packet_num*PACKET_SIZE;
                    file_data.insert(loc, packet.data);
                    // remove packet # from set to mark that we recevied it
                    packets.erase(packet.packet_num);
                    // Re-create 'missing' string
                    missing = "M" + to_string(file_pilot.file_ID) + " ";
                    for (auto iter  = packets.begin(); iter != packets.end(); iter++) {
                        missing += to_string(*iter);
                        if (next(iter) != packets.end())
                            missing += " ";
                        //Don't overfill buffer!
                        if (missing.length() + MAX_FILENUM + 1 > 512)
                            break;
                    }
                }
            }
        } //we timed out, so client is done sending packets (for now)
        //TODO remove
        missing = "M" + to_string(file_pilot.file_ID) + " ";
        for (auto iter  = packets.begin(); iter != packets.end(); iter++) {
            missing += to_string(*iter);
            if (next(iter) != packets.end())
                missing += " ";
            //Don't overfill buffer!
            if (missing.length() + MAX_FILENUM + 1 > 512)
                break;
        }
        c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                          missing.c_str());
        sock -> write(missing.c_str(), missing.length()+1);

    } while (!packets.empty()); // we have received data for all packets in this file

    if (!internalE2E(file_data, file_pilot, filehash)) {
        failed_e2es.push_back(to_string(file_pilot.file_ID));
        *GRADING << "File: " << file_pilot.fname
                             << " server-side end-to-end check failed\n";
    }
    else {
        *GRADING << "File: " << file_pilot.fname
                             << " server-side end-to-end check succeeded\n";
    }
    cout << "Data packets received\n";
}

bool internalE2E(string file_data, FilePilot file_pilot,
                map<string, string> &filehash)
{
    cout << "Interal e2eing\n";
    bool internal_e2e_succeeded = false;
    void *fopenretval;
    size_t len;
    int num_tries = 0;
    while (!internal_e2e_succeeded && (num_tries < MAX_WRITE_TRIES)) {

        //
        // Write the file using nastyfile interface
        //
        NASTYFILE outputFile(FILE_NASTINESS);
        string TMPname = file_pilot.fname + ".TMP";
        string full_TMPname = makeFileName(TARGET_DIR.c_str(), TMPname);

        // do an fopen on the output file
        fopenretval = outputFile.fopen(full_TMPname.c_str(), "wb");  

        if (fopenretval == NULL) {
          cerr << "Error opening output file " << full_TMPname << 
                  " errno=" << strerror(errno) << endl;
          exit(12);
        }
      
        // Write the whole file
        size_t num_bytes = file_data.size();
        len = outputFile.fwrite(file_data.c_str(), 1, num_bytes);
        if (len != num_bytes) {
          cerr << "Error writing file " << full_TMPname << 
                  "  errno=" << strerror(errno) << endl;
          num_tries++;
          continue;

        }
        // Close after writing
        if (outputFile.fclose() != 0) {
          cerr << "Error closing file " << full_TMPname << 
                  "  errno=" << strerror(errno) << endl;
          num_tries++;
          continue;
        }

        // check if getFileHash == what we expect from file_pilot
        // if not, try writing/checking again.
        size_t read_size;
        unsigned char target_file_hash[SHA1_LEN];
        getFileChecksum(TARGET_DIR.c_str(), TMPname,
                        read_size, target_file_hash);
        if (read_size != num_bytes) {
            cerr << "Error reading file " << file_pilot.fname << 
                    " after writing to disk." << endl;
            num_tries++;
            continue;
        }

        // Add checksum to table, regardless of whether it is correct.
        // This will be used later on for a full E2E directory check
        filehash[file_pilot.fname] = string((const char *)target_file_hash,
                                             SHA1_LEN-1);

        unsigned char expected_hash[SHA1_LEN];
        memcpy(expected_hash, file_pilot.hash.c_str(), SHA1_LEN);
        // Compare hash of written file and hash from FilePilot
        bool write_success = cmpChecksums(target_file_hash, expected_hash);
        if (write_success) {
            string total = makeFileName(TARGET_DIR.c_str(), file_pilot.fname);
            int result = rename(full_TMPname.c_str(), total.c_str());
            if (result != 0) {
                string msg = "Error renaming file " + full_TMPname;
                perror(msg.c_str());
            }
            else 
            // NEEDSWORK if we fail to rename the file, we will not report to
            // the client that the e2e check was successful. However, the .TMP
            // fille will techincally be correct.
            internal_e2e_succeeded = true;
        }
        else {
            num_tries++;
            continue;
        }
    } // tried & failed too many times
    cout << "internal e2eed\n";
    return internal_e2e_succeeded;
}

void sendE2E(C150NastyDgmSocket *sock, vector<string> failed,
             map<string, string> filehash, DirPilot dir_pilot)
{
    cout << "senbding e2e\n";
    string target_hash_str = getDirHash(filehash);
    unsigned char target_dir_hash[SHA1_LEN];
    memcpy(target_dir_hash, target_hash_str.c_str(), SHA1_LEN);

    unsigned char source_dir_hash[SHA1_LEN];
    memcpy(source_dir_hash, dir_pilot.hash.c_str(), SHA1_LEN);
    // Compare dir hashes
    bool success = cmpChecksums(target_dir_hash, source_dir_hash);

    string response = "E2E";

    if (success) {
        *GRADING << "Server-side: End-to-end directory check successful\n";
        response += "S";
    }
    else {
        *GRADING << "Server-side: End-to-end directory check failed\n";
        response += "F" + to_string(failed.size());
    }
        
    bool e2e_received = false;
    ssize_t readlen;
    char incoming_msg[512];
    // Loop until we receive confirmation from client that E2E was received
    while(!e2e_received) {
        *GRADING << "Sending E2E response to client\n";
        c150debug->printf(C150APPLICATION,"Responding with e2e message=\"%s\"",
                                      response.c_str());
        sock->write(response.c_str(), response.length()+1);
        readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
        if (readlen == 0) {
            c150debug->printf(C150APPLICATION,"Read zero length message,"
                              " trying again");
            continue;
        }
        incoming_msg[readlen] = '\0'; // make sure null terminated
        string incoming(incoming_msg); // Convert to C++ string
        if (incoming == "E2E received")
            e2e_received = true;
        
    }
    cout << "E2E confirmed\n";
}

/*
 * handleDir TODO remove
 * Handle a directory pilot packet and send response to the client
 * about if the target directory hash and given hash match.
 * Args: 
 *      incoming: a string containing file metadata from the client
 *      filehash: a map of filenames to checksums in the target directory
 *      sock: pointer to socket open for reading
 * Returns: Null
 *
 */
void handleDir(string incoming, map<string, string> filehash,
                C150DgmSocket *&sock)
{
    // Unpack into corresponding struct
    DirPilot dir_pilot = unpackDirPilot(incoming);
    
    // checksum of target directory
    string target_dir_hash = getDirHash(filehash);
    
    bool hashes_match = (target_dir_hash == dir_pilot.hash);
    
    string response;
    if (hashes_match)
        response = "DirHashOK";
    else
        response = "DirHashError";
    
    //
    // write the return message
    //
    c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                      response.c_str());
    sock -> write(response.c_str(), response.length()+1);
    cout << "Dir Pilot Received\n";
}

/*
 * TODO remove
 * handleFilePilot
 * Send response to client indicating whether the file in the target directory
 * has the same SHA1 checksum as the checksum in the given packet.
 * Args: 
 *      incoming: a string containing file metadata from the client
 *      filehash: a map of filenames to checksums in the target directory
 *      sock: pointer to socket open for reading
 */
void handleFilePilot(string incoming, map<string, string> filehash,
                     C150DgmSocket *&sock)
{
    cerr << "HANDLING PILE PILOT\n";
    // Unpack into struct
    FilePilot file_pilot = unpackFilePilot(incoming);
     //get correspnding checksum of file from target dir
    string trg_file_hash = filehash[file_pilot.fname];
    
    bool hashes_match = (trg_file_hash == file_pilot.hash);
    
    string response;
    if (hashes_match) {
        *GRADING << "File: " << file_pilot.fname
                 << " end-to-end check succeeded\n";
        response = "FileHashOK";
    }
    else {
        *GRADING << "File: " << file_pilot.fname
                 << " end-to-end check failed\n";
        response = "FileHashError";
    }
    GRADING->flush();
    
    //
    // write the return message
    //
    c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                      response.c_str());
    sock -> write(response.c_str(), response.length()+1);
    cout << "File Pilot Handled\n";
}

/*
 * TODO remove
 * handleFilePilot
 * NEEDSWORK: document function contract, args
 */
void handleData(string incoming, C150DgmSocket *&sock)
{
    //Dummy return statement for now
    string response = "PacketReceived";

    sock -> write(response.c_str(), response.length()+1);
}

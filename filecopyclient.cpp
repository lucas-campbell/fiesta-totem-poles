// --------------------------------------------------------------
//
//                        filecopyclient.cpp
//
//        Author: Dylan Hoffmann & Lucas Campbell       
//   
//
//        COMMAND LINE
//
//              fileclient <srvrname> <networknasty#> <filenasty#> <src>
//
//
//        OPERATION
//
//              Given a server, file and network nastiness levels, and a
//              directory, sends the contents of the directory to the server.
//              Quits after receiving an end-to-end response or if the server
//              times out.
//
//
//        LIMITATIONS
//
//              Works with file nastiness level up to 4 and network nastiness
//              level up to 4.
//
//
//     
// --------------------------------------------------------------

#include "utils.h"
#include "protocol.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150debug.h"
#include <string>
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>


using namespace std;          // for C++ std library
using namespace C150NETWORK;  // for all the comp150 utilities 

// forward declarations
void setUpDebugLogging(const char *logname, int argc, char *argv[]);
void sendDirPilot(int num_files, string hash, C150NastyDgmSocket *sock,
                  char *argv[]);
void sendFiles(DIR* SRC, const char* sourceDir, C150NastyDgmSocket *sock,
                 map<string, string> &filehash);
void sendFile(FilePilot fp, string f_data,  C150NastyDgmSocket *sock);
vector<FilePacket> makeDataPackets(FilePilot fp, string f_data);
void receiveE2E(C150NastyDgmSocket *sock);



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                    Command line arguments and Global Variables
//
// The following are used as subscripts to argv, the command line arguments
// If we want to change the command line syntax, doing this
// symbolically makes it a bit easier.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

const int SERVER_ARG = 1;     // server name is 1st arg
const int NETWORK_NASTINESS_ARG = 2;        // network nastiness is 2nd arg
const int FILE_NASTINESS_ARG = 3;        // file nastiness is 3rd arg
const int SRC_ARG = 4;            // source directory is 4th arg
const int TIMEOUT_MS = 300;       //ms for timeout
extern int NETWORK_NASTINESS;
extern int FILE_NASTINESS;
char* PROG_NAME;
const int MAX_SEND_TO_SERVER_TRIES = 20;




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                           main program
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 
int main(int argc, char *argv[]) {    

    GRADEME(argc, argv);

    //
    // Make sure command line looks right
    //
    
    // Command line args not used
    if (argc != 5) {
        fprintf(stderr,"Correct syntax is: %s <srvrname>"
                " <networknasty#> <filenasty#> <src>\n", argv[0]);
        exit(1);
    }

    if ((strspn(argv[NETWORK_NASTINESS_ARG], "0123456789") !=
        strlen(argv[NETWORK_NASTINESS_ARG])) ||
        (strspn(argv[FILE_NASTINESS_ARG], "0123456789") !=
         strlen(argv[FILE_NASTINESS_ARG]))) {
         fprintf(stderr,"Nastiness %s is not numeric\n", argv[FILE_NASTINESS_ARG]);     
         fprintf(stderr,"Correct syntax is: %s <srvrname>"
                " <networknasty#> <filenasty#> <src>\n", argv[0]);
         exit(4);
     }
     
    
    //
    // Variable declarations
    //
    DIR *SRC;                   // Unix descriptor for open directory
    NETWORK_NASTINESS = atoi(argv[NETWORK_NASTINESS_ARG]);
    FILE_NASTINESS = atoi(argv[FILE_NASTINESS_ARG]);
    PROG_NAME = argv[0];
    
    checkDirectory(argv[SRC_ARG]);  //Make sure src exists

    //
    // Open the source directory
    //
    SRC = opendir(argv[SRC_ARG]);
    *GRADING << "Opening Directory:" << argv[SRC_ARG] << endl;
    if (SRC == NULL) {
        fprintf(stderr,"Error opening source directory %s\n", argv[SRC_ARG]);
        exit(8);
    }

    //
    //  Set up debug message logging
    //
    setUpDebugLogging("filecopyclientdebug.txt",argc, argv);

    //
    //
    //        Send / receive / print 
    //
    
    try {

        // Create the socket
        c150debug->printf(C150APPLICATION,"Creating C150NastyDgmSocket");
        c150debug->printf(C150APPLICATION,"Creating"
                          " C150NastyDgmSocket(nastiness=%d)",
			 NETWORK_NASTINESS);
        C150NastyDgmSocket *sock = new C150NastyDgmSocket(NETWORK_NASTINESS);

        // Tell the DGMSocket which server to talk to
        sock -> setServerName(argv[SERVER_ARG]);  
        
        // Timeout of .3 seconds
        sock -> turnOnTimeouts(TIMEOUT_MS);

        *GRADING << "Prelim Setup Complete\n";

        // Loop through source directory, create hashtable with filenames
        // as keys and  individual file checksums as values
        map<string, string> filehash;
        fillChecksumTable(filehash, SRC, argv[SRC_ARG]);
        closedir(SRC);

        // Open directory again because we loop through it in fillChecksumTable
        SRC = opendir(argv[SRC_ARG]);
        *GRADING << "Opening Directory again:" << argv[SRC_ARG] << endl;
        if (SRC == NULL) {
            fprintf(stderr,"Error opening source directory %s\n", argv[SRC_ARG]);
            exit(8);
        }
        int num_files = filehash.size();

        // Creates a total directory checksum value based on filenames and
        // checksums together
        string dir_checksum = getDirHash(filehash);

        // Send directory pilot to server
        sendDirPilot(num_files, dir_checksum, sock, argv);
        
        // Loop to send files one by one to server
        sendFiles(SRC, argv[SRC_ARG], sock, filehash);

        // Wait for end-to-end check from server
        receiveE2E(sock);

        *GRADING << "Closing dir\n";
        closedir(SRC);
        *GRADING << flush;
        delete sock;
    }

    
    //
    //  Handle networking errors: if an exception is caught, print it to
    //  screen and exit. Mainly used to report timeouts.
    //
    catch (C150NetworkException& e) {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG,"Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: "
             << e.formattedExplanation() << endl;
    }
    
    
    return 0;
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
//        NEEDSWORK: should be factored into shared code w/pingserver
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
    // debug output to a file. Comment them
    // to default to the console.
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
 * sendDirPilot
 * Send directory pilot (number of files in directory, hash of directory) over
 * a given socket.
 * * Args:
 *    num_files: the number of files in the directory
 *    hash:      a string containing the checksum for the directory, generated
 *               by getDirHash()
 *    sock:      a socket already opened/configured, to be sent over
 *    argv:      command line arguments to the program, used for error messages
 *
 * Returns: None
 *    
 */
void sendDirPilot(int num_files, string hash, C150NastyDgmSocket *sock,
                  char *argv[])
{
    ssize_t readlen;
    bool timedout = true;
    char incoming_msg[512];   // received message data
    int num_tries = 0;
    DirPilot pilot = DirPilot(num_files, hash);
    // 'Packetized' DirPilot struct
    string dir_pilot_packet = makeDirPilot(pilot);
    int pack_len = dir_pilot_packet.size();
    const char * c_style_msg = dir_pilot_packet.c_str();
    
    while (timedout && num_tries < MAX_SEND_TO_SERVER_TRIES) {

        if (num_tries > 0)
            *GRADING << "Sending DP " << "for dir " << string(argv[SRC_ARG])
                     << " attempt #" << num_tries+1 << endl;
        // Send the message to the server
        c150debug->printf(C150APPLICATION, "%s: Writing message: \"%s\"",
                          PROG_NAME, c_style_msg);
        sock->write(c_style_msg,pack_len+1);
        // Read the response from the server
        c150debug->printf(C150APPLICATION,"%s: Returned from write,"
                          " doing read()", PROG_NAME);
        readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
        // Check for timeout
        timedout = sock -> timedout();
        if (timedout) {
            num_tries++;
            continue;
        }
        if (readlen == 0) {
            c150debug->printf(C150APPLICATION,"Read zero length message,"
                              " trying again");
            continue;
        }
        // Check for acknowledgement from server
        string incoming(incoming_msg, readlen-1);
        if (incoming == "DPOK")
            break;
    } //we timed out or tried 5 times

    if (num_tries == MAX_SEND_TO_SERVER_TRIES)
    {
        throw C150NetworkException("Confirmation from server timed out"
                                   " too many times for DirPilot");     
    }
}

/*
 * sendFiles
 * Loop to send contents of source directory over a given socket
 * Args:
 * * SRC: DIR * of source directory, open for reading.
 * * sourceDir: char *, name of the source directory
 * * sock: A nasty socket pointer, used to communicate with the server
 * * filehash: a map of {filename --> file SHA1} for the files of the source dir
 *
 * Returns: None
 *
 */
void sendFiles(DIR* SRC, const char* sourceDir, C150NastyDgmSocket* sock,
                 map<string, string> &filehash)
{
    int F_ID = 0; // gets incremented, the nth file has file_ID == n
    struct dirent *sourceFile;  // Directory entry for source file
    bool timedout = true;
    char incoming_msg[512];   // received message data
    int num_tries = 0;
    ssize_t readlen;
    size_t size;
    // Loop through source dir and create/send file pilot, then send file
    // packets once we know server has received the correct file pilot
    while ((sourceFile = readdir(SRC)) != NULL) {

        if ( (strcmp(sourceFile->d_name, ".") == 0) ||
             (strcmp(sourceFile->d_name, "..")  == 0 ) ) 
            continue;          // never copy . or ..
        
        string filename = sourceFile->d_name;
        string full_filename = makeFileName(sourceDir, filename);
        
        // check that is a regular file
        if (!isFile(full_filename))
            continue;                     
        //
        // Add {filename, checksum} to the table and construct FilePilot
        //
        unsigned char hash[SHA1_LEN];
        // Read data, compute checksum, put size of file in 'size'
        char *f_data_c = getFileChecksum(sourceDir, filename, size, hash);
        string f_data(f_data_c, size);
        //free malloc'd data
        free(f_data_c);
        
        string hash_str((const char*)hash, SHA1_LEN-1);
        filehash[filename] = hash_str;
        int num_packs = size / PACKET_SIZE;
        if (size % PACKET_SIZE != 0)
                num_packs++;
        
        FilePilot fp = FilePilot(num_packs, F_ID, hash_str, filename);
        string f_pilot = makeFilePilot(fp); //packetized
        int pack_len = f_pilot.size();
        const char * c_style_msg = f_pilot.c_str();

        //
        // Attempt to send File Pilot to server
        //
        while (timedout && num_tries < MAX_SEND_TO_SERVER_TRIES) {
            if (num_tries > 0)
                *GRADING << "Sending FP " << fp.file_ID
                         << " attempt #" << num_tries+1 << endl;
            // Send the message to the server
            c150debug->printf(C150APPLICATION,
                              "%s: Sending File Pilot: \"%s\"",
                              PROG_NAME, c_style_msg);
            sock->write(c_style_msg, pack_len+1);
            // Read the response from the server
            c150debug->printf(C150APPLICATION,"%s: Returned from write,"
                              " doing read()", PROG_NAME);
            readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
            // Check for timeout or 0 length message (nasty socket)
            timedout = sock -> timedout();
            if (timedout) {
                num_tries++;
                continue;
            }
            if (readlen == 0) {
                c150debug->printf(C150APPLICATION,"Read zero length message,"
                                  " trying again");
                continue;
            }
                
            string inc_str(incoming_msg, readlen-1);
            // Confirmation from server about specific File Pilot
            if ((inc_str.substr(0, 4) == "FPOK") &&
                (stoi(inc_str.substr(4)) == F_ID))
                break;
                
            timedout = true; // If we caught the wrong packet, reset
            
        } //we timed out or tried 5 times
        
        if (num_tries == MAX_SEND_TO_SERVER_TRIES)
        {
            throw C150NetworkException("Server is unresponsive, on FilePilot. "
                                       "Aborting"); 
        }

        // If we reached this point, server has received FilePilot and is
        // ready to receive packets
        sendFile(fp, f_data, sock);

        // Resets/increments for next file transmission loop
        num_tries = 0;
        F_ID++;
        timedout = true;
    }
    *GRADING << "Finished sending files to client\n";
}

/*
 * sendFile
 * Send contents of a file to server in a series of packetized FilePackets.
 * Args: 
 * * fp: FilePilot for the file being sent, contains metadata for the file
 * * f_data: a string containing the contents of the file
 * * sock: nasty socket used for communication with server
 *
 * Returns: None
 */
void sendFile(FilePilot fp, string f_data,  C150NastyDgmSocket *sock)
{
    *GRADING << "File: " << fp.fname << " beginning transmission\n";
    ssize_t readlen;
    char incoming_msg[512];   // received message data
    int num_file_tries = 0;

    // Break up buffer into FilePacket structs so that we can send data
    // piecemeal across the wire
    vector<FilePacket> dps = makeDataPackets(fp, f_data); //dps = data packets

    // packet_ids that the server still needs from us
    set<int> missing_packs; 
    // send all packets at least once, so 'missing' contains all packet
    // numbers to start
    for (size_t i = 0; i < dps.size(); i++)
        missing_packs.insert(i);
   
    do {
        if (num_file_tries > 1)
            *GRADING << "File: " << fp.fname
                << " sending missing data packets transmission #"
                << num_file_tries << endl;
        // Number of times we tried to send this iteration of missing packets
        int num_missing_tries = 0;
        bool timedout = true;
        // Send all packets that the server tells us it needs
        for (auto iter = missing_packs.begin(); iter != missing_packs.end(); iter++) {
            // Send each packet 5 times, for redundancy's sake
            for (int i = 0; i < 5; i++) {
                string data_pack = makeFilePacket(dps[*iter]);
                int pack_len = data_pack.size();
                const char * c_style_msg = data_pack.c_str();
                c150debug->printf(C150APPLICATION,
                                  "%s: Sending File Data, msg: \"%s\"",
                                  PROG_NAME, c_style_msg);
                sock->write(c_style_msg, pack_len+1);
            }
        }
        if (num_file_tries > 1) {
            *GRADING << "sent packets ";
            for (auto iter = missing_packs.begin(); iter != missing_packs.end(); iter++) {
                *GRADING << *iter << " ";
            }
            *GRADING << endl;
        }

        // Check for server message about which packets it still needs
        while (timedout && num_missing_tries < MAX_SEND_TO_SERVER_TRIES) {

            readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
            timedout = sock -> timedout();
            if (timedout) {
                num_missing_tries++;
                continue;
            }
            if (readlen == 0) {
                c150debug->printf(C150APPLICATION,"Read zero length message,"
                                  " trying again");
                continue;
            }
            string inc_str = string(incoming_msg, readlen-1);
            if ((inc_str.substr(0, 1) == "M") &&
                (stoi(inc_str.substr(1, inc_str.find(" ")-1)) == fp.file_ID)) {
                string missing = inc_str.substr(inc_str.find(" ") + 1);
                // construct new missing_packs id set using a convenient
                // constructor
                stringstream in(missing);
                missing_packs = set<int>{istream_iterator<int, char>{in},
                        istream_iterator<int, char>{}};
                break;
            }
                
            
            timedout = true; // If we caught the wrong packet, reset
            
        } //we timed out or tried 5 times
        
        if (num_missing_tries == MAX_SEND_TO_SERVER_TRIES)
        {
            throw C150NetworkException("Server is unresponsive on FilePacket. "
                                       "Aborting"); 
        }

        num_file_tries++;
 
    } while(!missing_packs.empty());

    // We don't report 'waiting for E2E' here, because our E2E is
    // directory-level
    *GRADING << "File: " << fp.fname << " transmission complete\n";
}

/*
 * makeDataPackets
 * Split a buffer containing file contents up into a vector of FilePacket
 * structs
 * Args:
 * * fp: FilePilot for the file, contains necessary metadata
 * * f_data: string containing the contents of the file
 *
 * Returns: vector of FilePacket structs, in the order that the data appears in
 * the file. If each struct's .data member is concatenated in the order it
 * appears in the vector, the resulting string will equal f_data.
 */
vector<FilePacket> makeDataPackets(FilePilot fp, string f_data){
    vector<FilePacket> data_packs;
    int i;
    for (i = 0; i < fp.num_packets-1; i++) {
        string data = f_data.substr(i*PACKET_SIZE, PACKET_SIZE);
        data_packs.push_back(FilePacket(i, fp.file_ID, data));
    }
    // Last packet may not be a full packet
    data_packs.push_back(FilePacket(i, fp.file_ID,
                                    f_data.substr(i*PACKET_SIZE)));
    return data_packs;
}

/*
 * receiveE2E
 * Wait for server to send E2E check over the network, log response
 *
 * Args:
 * * sock: nasty socket for server communication
 *
 * Returns: None
 */
void receiveE2E(C150NastyDgmSocket *sock)
{
    ssize_t readlen = 0;
    bool timedout = true;
    char incoming_msg[512];   // received message data
    int num_tries = 0;
    string E2EPilot("E2E Ready");
        
    while (timedout && (num_tries < MAX_SEND_TO_SERVER_TRIES)) {
        // Send the message to the server
        c150debug->printf(C150APPLICATION,
                          "%s: Sending E2E ready msg: \"%s\"",
                          PROG_NAME, E2EPilot.c_str());
        sock->write(E2EPilot.c_str(), E2EPilot.length()+1);
        readlen = sock -> read(incoming_msg, sizeof(incoming_msg));
        // Check for timeout
        timedout = sock -> timedout();
        if (timedout) {
            num_tries++;
            continue;
        }
        if (readlen == 0) {
            c150debug->printf(C150APPLICATION,"Read zero length message,"
                              " trying again");
            continue;
        }
        string inc_str = string(incoming_msg, readlen-1);
        // Check for success or failure
        if (inc_str.substr(0, 4) == "E2ES") {
            *GRADING << "Directory end-to-end check succeeded.\n";
            break;
        }
        else if (inc_str.substr(0, 4) == "E2EF") {
            size_t files_start_index = inc_str.find(" ");
            string num_failed = inc_str.substr(4, files_start_index-4);
            string files_failed = inc_str.substr(files_start_index);
            *GRADING << "Directory end-to-end check failed. "<< num_failed <<
                " file(s) not copied successfully:\n"
                << files_failed << endl;
            break;
        }
           
        timedout = true; // If we caught the wrong packet, reset    
    } //we timed out or tried 5 times

    if (num_tries == MAX_SEND_TO_SERVER_TRIES)
    {
        *GRADING << "Did not receive E2E. Aborting...\n";
        throw C150NetworkException("Did not receive E2E. Aborting..."); 
    }
    
    c150debug->printf(C150APPLICATION, "%s: Sending E2E confirmation",
                      PROG_NAME);
    // Blitz of messages to tell server we are done
    for (int i = 0; i < 10; i++)
        sock -> write("E2E received", 13);
}

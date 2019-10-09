// --------------------------------------------------------------
//
//                        filecopyclient.cpp
//
//        Author: Dylan Hoffmann & Lucas Campbell       
//        Based on pieces from ping code written by Noah Mendelsohn
//   
//
//        COMMAND LINE
//
//              fileclient <srvrname> <networknasty#> <filenasty#> <src>
//
//
//        OPERATION
//
//              As of end-to-end check: gets a hash of given directory, sends
//              to server, and awaits response. Quits after receiving response.
//
//
//        LIMITATIONS
//
//              This version only sends a Directory Pilot packet, which
//              contains a representative hash for the folder. It does not send
//              information on individual files.
//
//
//     
// --------------------------------------------------------------

#include "sha1.h"
#include "protocol.h"
#include "c150dgmsocket.h"
#include "c150debug.h"
#include <string>


using namespace std;          // for C++ std library
using namespace C150NETWORK;  // for all the comp150 utilities 

// forward declarations
void checkAndPrintMessage(ssize_t readlen, char *buf, ssize_t bufferlen);
void setUpDebugLogging(const char *logname, int argc, char *argv[]);
bool sendDirPilot(int num_files, string hash, C150DgmSocket **sock,
                    char *argv[]);
//NEEDSWORK This function needs to be implemented, to shorten body of the main
// server loop
bool sendFilePilot(int num_packets, int file_ID, string hash, string fname);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                    Command line arguments
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
const int TIMEOUT_MS = 3000;       //ms for timeout




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
        fprintf(stderr,"Correct syntxt is: %s <srvrname>"
                " <networknasty#> <filenasty#> <src>\n", argv[0]);
        exit(1);
    }

    //
    // Variable declarations
    //
    ssize_t readlen;              // amount of data read from socket
    char incoming_msg[512];   // received message data
    DIR *SRC;                   // Unix descriptor for open directory
    int DUMMY_PACKET_NUM = 10;
    int DUMMY_FILE_ID = 4;
    
    checkDirectory(argv[SRC_ARG]);  //Make sure src exists

    //
    // Open the source directory
    //
    SRC = opendir(argv[SRC_ARG]);
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
        c150debug->printf(C150APPLICATION,"Creating C150DgmSocket");
        C150DgmSocket *sock = new C150DgmSocket();

        // Tell the DGMSocket which server to talk to
        sock -> setServerName(argv[SERVER_ARG]);  
        
        // Timeout of 3 seconds
        sock -> turnOnTimeouts(TIMEOUT_MS);

        // Loop through source directory, create hashtable with filenames
        // as keys and checksums as values
        map<string, string> filehash;
        fillChecksumTable(filehash, SRC, argv[SRC_ARG]);

        int num_files = filehash.size();
        string dir_checksum = getDirHash(filehash);
        bool dir_sent = sendDirPilot(num_files, dir_checksum, &sock, argv);

        *GRADING << "SRC Dir hash check sent and matches: "
                 << dir_sent << endl;

        // Loop for getting user input
        string outgoing_msg;
        int num_tries = 0;
        //start at true so we "try" to send message "again"
        bool timedout = true;
        for (auto iter = filehash.begin(); iter != filehash.end(); iter++) {
            //NEEDSWORK: Fill in message with entire packet
            FilePilot pilot_struct = FilePilot(DUMMY_PACKET_NUM, DUMMY_FILE_ID,
                                               iter->second, iter->first);
            string pilot_pack = makeFilePilot(pilot_struct);
            outgoing_msg = pilot_pack;
            *GRADING << "Preparing to send outgoing: " << outgoing_msg << endl;
            
            const char *c_style_msg = outgoing_msg.c_str();
            *GRADING << "cstyle version: " << c_style_msg << endl;
            
            while (timedout && num_tries < 5) {
                *GRADING << "File: " << iter->first
                         << " ,beginning transmission, " << "attempt "
                         << num_tries + 1 << endl;
                // Send the message to the server
                c150debug->printf(C150APPLICATION,
                                  "%s: Writing message: \"%s\"",
                                  argv[0], c_style_msg);
                // +1 includes the null
                sock -> write(c_style_msg, strlen(c_style_msg)+1); 
                *GRADING << "File: " << iter->first
                         << " transmission complete, waiting for end-to-end"
                            " check, attempt " << num_tries + 1 << endl;
                // Read the response from the server
                c150debug->printf(C150APPLICATION,"%s: Returned from write,"
                                  " doing read()", argv[0]);
                readlen = sock -> read(incoming_msg,
                                       sizeof(incoming_msg));
                // Check for timeout
                timedout = sock -> timedout();
                if (timedout) {
                    num_tries++;
                    continue;
                }
                // Check and print the incoming message
                checkAndPrintMessage(readlen, incoming_msg,
                                     sizeof(incoming_msg));
                
                // Future iterations will resend or not resend appropriate
                // packets based on this value
                bool file_hash_eq = (strcmp(incoming_msg, "FileHashOK") == 0);
                if (file_hash_eq) {
                    *GRADING << "File: " << iter->first << " end-to-end check"
                              " succeeded, attempt " << num_tries + 1 << endl;
                }
                else { 
                    *GRADING << "File: " << iter->first << " end-to-end check"
                                " failed, attempt " << num_tries + 1 << endl;
                }

                *GRADING << "File: " << iter->first
                    << ", end of send/receive loop:\n"
                    << "timedout: " << timedout
                    << ", num_tries: " << num_tries
                    << ", received message: " << incoming_msg << endl;
            } // timedout == false or num_tries == 5
            
            if (num_tries == 5)
            {
                throw C150NetworkException("Write to server timed out"
                                           " too many times");     
            }
            timedout = true; // reset for next message send
        }
    
        *GRADING << "Closing dir\n";
        closedir(SRC);
    }

    
    //
    //  Handle networking errors -- for now, just print message and give up!
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
//                     checkAndPrintMessage
//
//        Make sure length is OK, clean up response buffer
//        and print it to standard output.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 


void checkAndPrintMessage(ssize_t readlen, char *msg, ssize_t bufferlen) {
    // 
    // Except in case of timeouts, we're not expecting
    // a zero length read
    //
    if (readlen == 0) {
        throw C150NetworkException("Unexpected zero length read in client");
    }

    // DEFENSIVE PROGRAMMING: we aren't even trying to read this much
    // We're just being extra careful to check this
    if (readlen > (int)(bufferlen)) {
        throw C150NetworkException("Unexpected over length read in client");
    }

    //
    // Make sure server followed the rules and
    // sent a null-terminated string (well, we could
    // check that it's all legal characters, but 
    // at least we look for the null)
    //
    if(msg[readlen-1] != '\0') {
        throw C150NetworkException("Client received message "
                                   "that was not null terminated");     
    };

    //
    // Use a routine provided in c150utility.cpp to change any control
    // or non-printing characters to "." (this is just defensive programming:
    // if the server maliciously or inadvertently sent us junk characters,
    // then we won't send them to our terminal -- some 
    // control characters can do nasty things!)
    //
    // Note: cleanString wants a C++ string, not a char*,
    // so we make a temporary one
    // here. Not super-fast, but this is just a demo program.
    string s(msg);
    cleanString(s);

    // Echo the response on the console

    c150debug->printf(C150APPLICATION,"PRINTING RESPONSE:"
                      " Response received is \"%s\"\n", s.c_str());
    printf("Response received is \"%s\"\n", s.c_str());

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
 * Send hash of directory contents over a given socket. Serves as an end to end
 * check by comminucating with the server and waiting for a response on whether
 * a hash of directory contents matches.
 * Args:
 *    num_files: the number of files in the directory
 *    hash:      a string containing the checksum for the directory, generated
 *               by getDirHash()
 *    sock:      a socket already opened/configured, to be sent over
 *    argv:      command line arguments to the program, used for error messages
 * Returns:
 *    A boolean indicating whether the hash of the target directory on the
 *    server and the hash sent over the socket match.
 *    
 */
bool sendDirPilot(int num_files, string hash, C150DgmSocket **sock,
                    char *argv[])
{
    bool dir_hash_matches = false;
    bool timedout = true;
    ssize_t readlen;              // amount of data read from socket
    char incoming_msg[512];   // received message data
    int num_tries = 0;
    DirPilot pilot = DirPilot(num_files, hash);
    string dir_pilot_packet = makeDirPilot(pilot);
    const char * c_style_msg = dir_pilot_packet.c_str();
    while (timedout && num_tries <= 5) {
        // Send the message to the server
        c150debug->printf(C150APPLICATION,
                          "%s: Writing message: \"%s\"",
                          argv[0], c_style_msg);
       (*sock)->write(c_style_msg, strlen(c_style_msg)+1);
        // Read the response from the server
        c150debug->printf(C150APPLICATION,"%s: Returned from write,"
                          " doing read()", argv[0]);
        readlen = (*sock) -> read(incoming_msg,
                               sizeof(incoming_msg));
        // Check for timeout
        timedout = (*sock) -> timedout();
        if (timedout) {
            num_tries++;
            continue;
        }
        // Check and print the incoming message
        checkAndPrintMessage(readlen, incoming_msg,
                             sizeof(incoming_msg));
        if (string(incoming_msg) == "DirHashOK")
            dir_hash_matches = true;
      
    } //we timed out or tried 5 times

    if (num_tries == 5)
    {
        throw C150NetworkException("Write to server timed out"
                                   " too many times");     
    }

    return dir_hash_matches;
}

# Makefile for COMP 117 End-to-End assignment
#
#    Copyright 2012 - Noah Mendelsohn
#
# The targets below are for samples that may be helpful, but:
#
#   YOUR MAIN JOB IN THIS ASSIGNMENT IS TO CREATE TWO 
#   NEW PROGRAMS: fileclient and fileserver
#
#   YOU WILL MODIFY THIS MAKEFILE TO BUILD THEM TOO
#
# Useful sample program targets:
#
#
#    nastyfiletest - demonstrates the c150nastyfile class that you 
#                    MUST use to read and write your files
#
#    sha1test -   sample code for computing SHA1 hash
#
#    makedatafile -  sometimes when testing file copy programs
#                    it's useful to have files in which it's
#                    relatively easy to spot changes.
#                    This program generates sample data files.
#
#  Maintenance targets:
#
#    Make sure these clean up and build your code too
#
#    clean       - clean out all compiled object and executable files
#    all         - (default target) make sure everything's compiled
#

# Do all C++ compies with g++
CPP = g++
CPPFLAGS = -g -Wall -Werror -I$(C150LIB)

# Where the COMP 150 shared utilities live, including c150ids.a and userports.csv
# Note that environment variable COMP117 must be set for this to work!

C150LIB = $(COMP117)/files/c150Utils/
C150AR = $(C150LIB)c150ids.a

LDFLAGS = 
INCLUDES = $(C150LIB)c150dgmsocket.h $(C150LIB)c150nastydgmsocket.h $(C150LIB)c150network.h $(C150LIB)c150exceptions.h $(C150LIB)c150debug.h $(C150LIB)c150utility.h utils.h protocol.h

UTILS = utils.o protocol.o

all: protocoltest shatest fileserver fileclient nastyfiletest datafilemake sha1test

protocoltest: test_protocol.cpp $(UTILS) $(C150AR) $(INCLUDES)
	$(CPP) -o protocoltest $(CPPFLAGS) test_protocol.cpp $(UTILS) -lssl -lcrypto $(C150AR)

shatest: shatest.cpp $(UTILS) $(C150AR) $(INCLUDES)
	$(CPP) -o shatest $(CPPFLAGS) shatest.cpp $(UTILS) -lssl -lcrypto $(C150AR)

#
# Build the filecopy client
#
fileserver: filecopyserver.cpp $(UTILS) $(C150AR) $(INCLUDES)
	$(CPP) -o fileserver $(CPPFLAGS) filecopyserver.cpp $(UTILS) -lssl -lcrypto $(C150AR)

#
# Build the filecopy client
#
fileclient: filecopyclient.cpp $(UTILS) $(C150AR) $(INCLUDES)
	$(CPP) -o fileclient $(CPPFLAGS) filecopyclient.cpp $(UTILS) -lssl -lcrypto $(C150AR)

#
# Build the sha1test
#
#sha1: sha1.cpp
#	$(CPP) -o sha1 sha1.cpp -lssl -lcrypto

#
# Build the nastyfiletest sample
#
nastyfiletest: nastyfiletest.cpp  $(C150AR) $(INCLUDES)
	$(CPP) -o nastyfiletest  $(CPPFLAGS) nastyfiletest.cpp $(C150AR)

#
# Build the sha1test
#
sha1test: sha1test.cpp
	$(CPP) -o sha1test sha1test.cpp -lssl -lcrypto

#
# Build the makedatafile 
#
datafilemake: datafilemake.cpp
	$(CPP) -o datafilemake datafilemake.cpp 

#
# To get any .o, compile the corresponding .cpp
#
%.o:%.cpp  $(INCLUDES)
	$(CPP) -c  $(CPPFLAGS) $< 


#
# Delete all compiled code in preparation
# for forcing complete rebuild#

clean:
	 rm -f protocoltest shatest fileclient fileserver nastyfiletest sha1test datafilemake *.o *~ GRADELOG.*



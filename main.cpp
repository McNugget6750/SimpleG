/**
    SimpleG - Ultra fast gCode2grbl streaming
    main.cpp
    Purpose: Connects to grbl 0.9j controller at 115200 baud and sends the given gCode file as fast as possible to the controller.

    @author Timo Birnschein (timo.birnschein@microforge.de)
    @version 0.1 10/12/16
*/

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <list>

using namespace std;

// RX buffer within grbl (or the Arduino you are using)
#define GRBL_RX_BUFFER_SIZE 127

// Required to tell main loop that the user tried to interrupt the process and wants to stop grbl
bool userCloseRequestReceived = false;

// The user requested the program to close before the gCode file was finished sending to the laser.
// This can cause the laser to stay in an undesired mode like laser on at full power will will
// eventually lead to a fire and then your house will burn down with your wife and your kids in it.
// (I told you never to leave your laser unattended...)
// Therefore, we must notify the gcode sender process to stop all activity and send a configurable
// command to shut down the laser and deactivate the motors before closing the program itself.
// There is two signals we have to handle. CTRL+C and general exit (like closing the console).
// Everything else is optional and can be added later.
BOOL WINAPI consoleHandler(DWORD signal)
{

    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_LOGOFF_EVENT || signal == CTRL_SHUTDOWN_EVENT)
    {
        cout << "\n\nWARNING: User abort! Attempting to turn off the laser and then shutdown!\n\n";
        userCloseRequestReceived = true;
    }

    return TRUE;
}

// Here is all the program code. There is no other functions to keep it lean and easy.
// We just want the fastest gCode sender on the planet, nothing more...
int main(int argc, char *argv[])
{

    /*************************************** Handle Parameters ******************************************/
    char* myFile = NULL;
    char* myEndFile = NULL;
    char* myPort = NULL;

    unsigned long numLine = 0;
    unsigned long numOK = 0;
    unsigned long numERROR = 0;
    bool done_error_reading = false;

    int charsInGRBLBuffer = 0;
    std::list<int> gCodesSent;

    bool newLineAvailable = true;
    bool gCodeSentOut = true;

    // Present welcome message to user
    cout << "\n\n\n############################ SimpleG High Speed grbl gCode Sender ################################ \n";
    cout << "#                                                                                                # \n";
    cout << "# Author: Timo Birnschein (timo.birnschein@microforge.de)                                        # \n";
    cout << "#                                                                                                # \n";
    cout << "# This gCode sender sends gCode as fast as possible to grbl by keeping track of the number of    # \n";
    cout << "# bytes sent to the grbl controller. This gets around the common problem is long latencies       # \n";
    cout << "# and stuttering tool heads.                                                                     # \n";
    cout << "# The main purpose if to drive laser cutters or engraving using a CNC mill.                      # \n";
    cout << "# Please check https://github.com/McNugget6750 to find the complete project with a working grbl. # \n";
    cout << "#                                                                                                # \n";
    cout << "# WARNING: THIS IS UNTESTED SOFTWARE! NEVER LEAVE YOUR MACHINE ALONE! I CANNOT BE MADE           # \n";
    cout << "# RESPONSIBLE FOR ANY PROBLEMS YOU MIGHT HAVE OR ANY DAMAGE OF ANY KIND TO ANYTHING OR ANY ONE!  # \n";
    cout << "################################################################################################## \n";

    cout << "\n\n\n\n\n Turn laser off now and hit ENTER!\n\n";
    cout << " Or grbl might turn it on at full power during startup\n (if you use Pin D13 on an Arduino Nano/Uno)!\n\n\n\n";
    cin.get();

    // argument handling from here: http://www.cplusplus.com/forum/articles/13355/
    // Check the value of argc. If not enough parameters have been passed, inform user and exit.
    if (argc != 7)
    {
        // Inform the user of how to use the program
        cout << "\n\nUsage is -p <port name like COM6> -f <path to file to stream to grbl> -F <path to abort gcode file in case you hit CTRL+C or something>\n";
        exit(0);
    }
    else     // if we got enough parameters...
    {
        for (int i = 1; i < argc; i++)
        {
            /* We will iterate over argv[] to get the parameters stored inside.
                                              * Note that we're starting on 1 because we don't need to know the
                                              * path of the program, which is stored in argv[0] */
            if (string(argv[i]) == "-f")
            {
                // We know the next argument *should* be the filename:
                myFile = argv[i + 1];
                cout << "File name: " << myFile << "\n";
            }
            else if (string(argv[i]) == "-F")
            {
                // We know the next argument *should* be the abort gcode filename:
                myEndFile = argv[i + 1];
                cout << "Emergency Stop File name: " << myEndFile << "\n";
            }
            else if (string(argv[i]) == "-p")
            {
                myPort = argv[i + 1];
                cout << "Port name: " << myPort << "\n";
            }
        }
    }

    // Tell the system where to find the function for handling any program abort functions.
    // UNTESTED: I have not tested the behavior of killing the process, shutting down windows or closing the console!
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE))
    {
        cout << "\nERROR: Could not set control handler";
        return 1;
    }

    /*************************************** Set up COM port **************************************/
    // Com port handling from here: http://www.dreamincode.net/forums/topic/322031-serial-communication-with-c-programming/
    DCB dcb= {0};
    HANDLE hCom;
    char *pcCommPort = myPort;
    char readBuffer[1024]= {0};
    char writeBuffer[1024]= {0};
    DWORD dwBytesRead=0;
    DWORD dwBytesWrite=0;

    hCom = CreateFile( pcCommPort,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,             // must be opened with exclusive-access
                       NULL,                        // no security attributes
                       OPEN_EXISTING,               // must use OPEN_EXISTING
                       FILE_ATTRIBUTE_NORMAL,       // not overlapped I/O
                       NULL                         // hTemplate must be NULL for comm devices
                     );

    if (hCom == INVALID_HANDLE_VALUE)
    {
        cout << "CreateFile failed with error: " << (int)GetLastError() << "\n";
        return (1);
    }

    /*************************************** CommTimeouts ******************************************/
    // TODO: Test what those timeouts actually do while receiving ok from grbl.
    // Since In only wait for single bytes for that operation it should not make any difference.
    // Needs thorough testing for further speed optimization.
    COMMTIMEOUTS timeouts= {0};
    timeouts.ReadIntervalTimeout=50;
    timeouts.ReadTotalTimeoutConstant=50;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=0;
    timeouts.WriteTotalTimeoutMultiplier=0;

    if(!SetCommTimeouts(hCom, &timeouts))
    {
        /*Well, then an error occurred*/
        cout << "Something went wrong setting timeouts for the com port\n";
    }

    if (!GetCommState(hCom, &dcb))
    {
        /* More Error Handling */
        cout << "GetCommState failed with error: " << (int)GetLastError() << "\n";
        return (2);
    }

    // Major settings for your grbl. Only changes to the baudrate should be necessary but since you want fast... just don't.
    dcb.BaudRate = 115200;          // set the baud rate
    dcb.ByteSize = 8;               // data size, xmit, and rcv
    dcb.Parity = NOPARITY;          // no parity bit
    dcb.StopBits = ONESTOPBIT;      // one stop bit

    if (!SetCommState(hCom, &dcb))
    {
        cout << "SetCommState failed with error: " << (int)GetLastError() << "\n";
        return (3);
    }
    else
        cout << "Serial port " << pcCommPort << " successfully configured.\n";

    /*************************************** Opening file for Streaming **************************************/
// File handling from here: http://www.cplusplus.com/forum/beginner/11304/

    string sendLine;
    string readLine;
    ifstream myfile(myFile);
    ifstream myendfile(myEndFile);

    // Open the file with the gCodes to stream to grbl
    if (!myfile)
    {
        cout << "Could not open file: \n" << myFile;
        exit(0);
    }

    // Open the file with the emergency gCodes that bring grbl to a safe state in case you press CTRL+C or worst.
    if (!myendfile)
    {
        cout << "Could not open emergency end file: \n" << myEndFile;
        exit(0);
    }

    /*************************************** Start grbl communication **************************************/

    cout << "Opening port and waiting for grbl to send welcome message...\n";

    if(ReadFile(hCom, readBuffer, 255, &dwBytesRead, NULL))
    {
        cout << readBuffer;
    }

    cout << "Requesting current grbl configuration from control board...\n";

    writeBuffer[0] = '$';
    writeBuffer[1] = '$';
    writeBuffer[2] = '\n';

    if(WriteFile(hCom, writeBuffer, 3, &dwBytesWrite, NULL))
    {
        cout << writeBuffer;
        cout << "\n";
    }
    else
    {
        cout << "Error writing to COM port! EXIT!\n";
        exit(0);
    }

    // And read back what we receive
    if(ReadFile(hCom, readBuffer, 1024, &dwBytesRead, NULL))
    {
        // I know this might look weird but there seems to be a bug. If I don't display the chars like this there is an $$ at the end.
        for(int j=0; j < (int)dwBytesRead; j++)
        {
            cout << readBuffer[j];
        }
    }
    else
    {
        cout << "Error reading from COM port! EXIT!\n";
        exit(0);
    }

    cout << "\n\n";
    cout << "If this is not correct, please press Ctrl+C now to abort.\nOtherwise hit enter to start sending gCode!\n";
    cin.get();
    cout << "############################ Start Streaming to grbl ############################ \n\n";

    // Secret Sauce: Send operation and buffering similar to https://github.com/grbl/grbl/blob/658eb6a8b3cff0da59a61cbb5aa04d2c693c36c3/doc/script/stream.py

    // The aggressive streaming protocol for grbl requires detailed counting of characters streamed
    sendLine.clear();
    charsInGRBLBuffer = 0;

    readLine.clear();
    memset(readBuffer, ' ', 1024);  // Erase whatever is in the buffer for a clean start.

    int lineLen = 0;                // The current line length we plan to send out.

    while (1)  // Main Loop
    {
        // Fetch a new line from the file
        if (gCodeSentOut == true)   // Did we send out the last gCode already?
        {
            if (userCloseRequestReceived == false)     // We received no emergency exit.
                newLineAvailable = getline(myfile, sendLine);
            else
                newLineAvailable = getline(myendfile, sendLine);    // USER ABORT! We received a user interrupt!

            if (newLineAvailable)   // We read a new line from the file we want to send data from.
            {
                // Remove all lines we don't want to send out because it can kill the execution speed of grbl
                if (sendLine.c_str()[0] == '(' || sendLine.c_str()[0] == '/' || sendLine.c_str()[0] == '\n' || sendLine.c_str()[0] == '\r'  || sendLine.c_str()[0] == ' '  || sendLine.c_str()[0] == '\0' )
                {
                    continue;
                }

                // Adding a delimiter for the line end
                sendLine += "\n";

                // Deleting all white space from the line to reduce number of characters send
                lineLen = sendLine.length();
                for (int i = 0; i < lineLen; i++)
                {
                    if(sendLine[i] == ' ')
                    {
                        sendLine.erase(i, 1);
                        lineLen--;
                        i--;
                    }
                }

                // Copy the string into a buffer and store the string length.
                strcpy(writeBuffer, sendLine.c_str());
                lineLen = sendLine.length();

                // Store that we haven't sent out this gCode yet.
                gCodeSentOut = false;
            }
        }

        // Check if there is enough space within the grbl rx buffer to receive the new gCode
        if (charsInGRBLBuffer < GRBL_RX_BUFFER_SIZE - lineLen && gCodeSentOut == false)
        {
            // And send it out to grbl
            if (WriteFile(hCom, writeBuffer, lineLen, &dwBytesWrite, NULL))
            {
                cout << sendLine;   // If you must know what we're sending... here it is on the console
                gCodesSent.push_back(lineLen); // Store the length of the line that was just sent
                gCodeSentOut = true;    // And make sure we know we can send a new gCode
                numLine++;  // Store how many lines we have sent total.
            }
        }

        // Is there something we should read from the com port?
        // TODO: Here is some potential for further speed up. Check the timeouts and test what's really necessary.
        ReadFile(hCom, readBuffer, 1, &dwBytesRead, NULL);
        readLine += readBuffer[0];

        // If we find an ok, we can react accordingly.
        if (readLine.find("ok") != string::npos)
        {
            gCodesSent.pop_front(); // Delete the front entry because we have received an OK for this entry
            numOK++;    // Count the number of "ok" messages
            cout << "ok\n";
            readLine.clear();   // Clear receive line buffer
        }
        else if (readLine.find("error") != string::npos) // In case of an error, we want the whole error message and continue after.
        {
            cout << "Error found, looking for end of line...\n";
            if(readLine.find('/n') != string::npos)
            {
                cout << "Found End of Line\n";
                done_error_reading = true;
            }

            if(done_error_reading == true)
            {
                cout << "ERROR MESG: " << readBuffer;   // Print the error message for the users convenience.
                gCodesSent.pop_front(); // Delete the front entry because we have received an Error for this entry
                numERROR++; // Count the number of "error" messages
                readLine.clear();   // Clear receive line buffer
                done_error_reading = false;
            }
            // TODO: Track what gCode was sent that caused the error message.
            //ReadFile(hCom, readBuffer, 255, &dwBytesRead, NULL);

        }

        // Calc the sum of all bytes sent to grbl
        charsInGRBLBuffer = 0;  // Reset the number of bytes in buffer
        // Run through the list of gCode sizes that were sent out and calculate the grbl rx buffer fill state
        for (std::list<int>::iterator it = gCodesSent.begin(); it != gCodesSent.end(); it++)
            charsInGRBLBuffer += *it;
        //cout << "Bytes in grbl RX buffer: " << charsInGRBLBuffer << "\n";

        // Have we reached the end of the file and the buffer is empty? Then exit.
        if (charsInGRBLBuffer == 0 && !newLineAvailable)
            break;
    } // End main loop

    // We have reached the end of the program and provide some statistical data
    cout << "\ngCode program end.\n";
    cout << "Sent out " << numLine << " line numbers.\n";
    cout << "For those I received " << numOK << " acknowledgments and " << numERROR << " errors.\n";

    // Did we finish properly
    if (userCloseRequestReceived == false)
        cout << "Finished!\n";
    else    // Or did the user kill the program before it could end?
    {
        cout << "\n\n##################################################################################################\n";
        cout << "#############  WARNING: SimpleG killed by user. Tried to turn of laser before exit.  #############\n";
        cout << "##################################################################################################\n";
        cout << "###########################   PLEASE CHECK IF LASER IS OFF!!!  ###################################\n";
        cout << "##################################################################################################\n";
    }

    // Give grbl a chance to finish the last commands and then let the user decide to exit...
    for (int i = 0; i < 2; i++)
    {
        cout << "Waiting for " << 2 - i << " seconds...\n";
        Sleep(1000);
    }

    cout << "Please press enter to exit...\nWARNING: grbl will immediately stop whatever it is currently doing! Even if that means the laser is still on!\n";
    cin.get(); // Needed to really let grbl finish.
    cout << "Bye...\n";

    myfile.close(); // Close the gCode file
    myendfile.close();  // Close the emergency stop gCode file
    CloseHandle(hCom);  // Close the com port
    return(0);  // Exit the program
}

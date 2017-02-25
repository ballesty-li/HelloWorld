/*
  SD card basic file example
 
 This example shows how to create and destroy an SD card file 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	
 TO DO:
 - Add in DEBUG ON/OFF MACRO
 - STANDARDIZE DEBUG STATEMENTS BASED ON FUNCTION AND LINE
 - RELAYOUT THE FUNCTIONS/CLASSES FOR CLARITY
 - ADD IN PROTECTION FROM CARRIAGE RETURN, SAME AS LINE FEED
 - ADD IN 8.3 FILENAMES, REPLACE 16 Character filenames.
 
 */
#include <SD.h>

#define SD_CHIP_SELECT 53

#define DEBUG_ON  0  // This Macro turns DEBUG Statements on and off. 1 = on.  0 = off.

File myFile;

#define MAX_COMMAND_STRING_SIZE       80
#define MAX_FILENAME_STRING_SIZE      16
#define MAX_FILE_CONTENTS_STRING_SIZE 64
#define TRUE                           1
#define FALSE                          0

// STATE definitions for parsing command line input from User on Serial Monitor.
#define NO_STATE                 0
#define COMMAND_TYPE_STATE       1
#define COMMAND_TYPE_STATE_BLANK 2
#define FILENAME_STATE           3
#define FILENAME_STATE_BLANK     4
#define FILECONTENTS_STATE       5
#define COMPLETE_STATE           6

// STATE definitions for taking actions on Filesystem
#define NO_ACTION                0
#define CREATE_A_FILE            1
#define APPEND_TO_FILE           2
#define DELETE_A_FILE            3
#define LIST_FILES               4
#define REVEAL_CONTENTS_FILE     5


// Runtime variables.
int AvailableBytes = 0;
int CommandStringSize = 0;
int CommandStringCurrentIndex = 0;
int FileNameStringCurrentIndex = 0;
int FileContentsStringCurrentIndex = 0;
char TempChar;
char CommandString[MAX_COMMAND_STRING_SIZE];
char FileNameString[MAX_FILENAME_STRING_SIZE];
char FileContentsString[MAX_FILE_CONTENTS_STRING_SIZE];
boolean ErrorInCommandString = FALSE;
int ParseState = COMMAND_TYPE_STATE;
int CommandType = 0;
int CommandComplete = FALSE;
File root;
int FileSize = 0;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  Serial.print("Initializing SD card...");
  pinMode(SD_CHIP_SELECT, OUTPUT);

  if (!SD.begin(SD_CHIP_SELECT)) 
  {
    
#if (DEBUG_ON)
    Serial.println("initialization failed!");
#endif

    return;
  }
  
#if (DEBUG_ON)
  Serial.println("initialization done.");
#endif

  // 'Help' block at start of program.
  Serial.println(); 
  Serial.println("Micro SD File Test Application.  Rev 8.");   
  Serial.println();  
  Serial.println("Command Line Usage:");
  Serial.println("c 'filename' 'contents' - Create a file named 'filename' and place the text 'contents' into the file");  
  Serial.println("d 'filename' - Delete a file named 'filename'");
  Serial.println("l - List all files on drive");
  Serial.println("r 'filename' - Reveal file contents");
  Serial.println();  
  Serial.println("Rules:");
  Serial.println("All 'filenames' shall be less than 8.3 characters."); 
  Serial.println("All 'contents' shall be less that 64 characters."); 
  Serial.println("Only one command line string per user entry."); 
  Serial.println("Flat filesystem.  No directories supported."); 
  Serial.println("Serial Monitor must be configured with Newline on."); 
  Serial.println();  
  
  RunTimeVariablesReset();

}



void loop()
{

  char FileChar;
  
  // Read Users' serial input, determine if User wants to Create a file, Add to a file, or Delete a file.
  // User input may come in multiple Serial.read statements!  so read as needed.
  // Collect all the input from the user into a CommmandString buffer.
  if ((AvailableBytes = Serial.available()) > 0)
  {

    // PLACE A TIMEOUT IN THIS WHILE LOOP!!

    // Continue reading data from the Serial Port, until run out of data OR parse incorrect message from User
    while ((ErrorInCommandString == FALSE) && ((AvailableBytes = Serial.available()) > 0))
    {

      // Retrieve the next character in the Serial Buffer
      TempChar = Serial.read();

      // Determine what State the parsing of the incoming command is in.
      switch(ParseState)
      {

      case COMMAND_TYPE_STATE:
        // Parsing the start of a new Command from the User.
        // Determine what command is being requested by user.

        switch(TempChar)
        {

        case 'c':  // create a new file
          CommandType = CREATE_A_FILE;
          
#if (DEBUG_ON)
          Serial.println("DEBUG: CREATE_A_FILE.");
#endif

          break;

        case 'd':  // delete an existing file
          CommandType = DELETE_A_FILE;

#if (DEBUG_ON)          
          Serial.println("DEBUG: DELETE_A_FILE."); 
#endif

          break;

        case 'l':  // List the files on the filesystem
          CommandType = LIST_FILES;
          
#if (DEBUG_ON)           
          Serial.println("DEBUG: LIST_FILES."); 
#endif

          // This is a complete command line entry from the user!
          CommandComplete = TRUE;
          break;

        case 'r':  // Reveal the contents of a file on the filesystem
          CommandType = REVEAL_CONTENTS_FILE;
          
#if (DEBUG_ON)       
          Serial.println("DEBUG: REVEAL_CONTENTS_FILE.");  
#endif

          break; 

        case '\n':  // Newline.  Throw away.  Disregard.

          CommandComplete = TRUE;
          CommandType = NO_ACTION;
          
#if (DEBUG_ON)          
          Serial.println("DEBUG: NEWLINE.");  
#endif

          break; 

        default :  // incorrect command from User.  Discard the command.
          ErrorInCommandString = TRUE; 
          break;
        } // End of switch for Command Character

        // Move to next STATE
        ParseState = COMMAND_TYPE_STATE_BLANK;
        break;

      case COMMAND_TYPE_STATE_BLANK:
      
#if (DEBUG_ON)
        Serial.println("DEBUG: COMMAND_TYPE_STATE_BLANK.");
#endif

        // determine if character in serial port is a Space ' ' character.  If so, move onto next state.  If not, then failure to 
        if (TempChar == ' ')
        {
          // Well formed command from User so far...
          // Allow this logic to flow to next State, and start parsing the Filename.
          // Zero out the counters and memory for storing a Filename.
          FileNameStringCurrentIndex = 0;
          FileNameString[0] = NULL;
        }
        else
        {
          // incorrect command from User.  Discard the command.
          ErrorInCommandString = TRUE; 
        }

        // Move to next STATE
        ParseState = FILENAME_STATE;
        break;

      case FILENAME_STATE:

#if (DEBUG_ON)
        Serial.println("DEBUG: FILENAME_STATE.");
#endif

        // Read the Filename in.
        if (FileNameStringCurrentIndex > MAX_FILENAME_STRING_SIZE )
        {
          // Command string exceeded the maximum size of a filename.
          // incorrect command from User.  Discard the command.
          ErrorInCommandString = TRUE; 
        }
        else
        {
          if ((TempChar == ' ') && (FileNameStringCurrentIndex == 0))
          {
            // Too many spaces between command, so error
            // incorrect command from User.  Discard the command.
            ErrorInCommandString = TRUE;
          }
          else if (TempChar == ' ')
          {
              // Now at the end of the Filename.  Transition to next State.
              // Move to next STATE
              ParseState = FILECONTENTS_STATE;  

              if ((CommandType == DELETE_A_FILE) || (CommandType == REVEAL_CONTENTS_FILE))
              {
                // Can move onto to file deletion, as we are at end of command string
                CommandComplete = TRUE;
              }
              
#if (DEBUG_ON)
              Serial.println("DEBUG: FILENAME :");
              Serial.println( FileNameString);
#endif

          } 
          else if (TempChar == '\n')
          {   
            if ((CommandType == DELETE_A_FILE) || (CommandType == REVEAL_CONTENTS_FILE))
            {
              // Can move onto to file deletion, as we are at end of command string
              CommandComplete = TRUE;
            } 
          }
          else
          {
            // Command string good so far.
            FileNameString[FileNameStringCurrentIndex] = TempChar;
            FileNameStringCurrentIndex++; 
            FileNameString[FileNameStringCurrentIndex] = NULL;
          }
        }

        break;

      case FILECONTENTS_STATE:

#if (DEBUG_ON)
        Serial.println("DEBUG: FILECONTENTS_STATE.");
#endif

        // Read the File contents in.
        if (FileContentsStringCurrentIndex > MAX_FILE_CONTENTS_STRING_SIZE )
        {
          // Contents string exceeded the maximum size of a filename.
          // incorrect command from User.  Discard the command.
          ErrorInCommandString = TRUE; 
        }
        else
          if ((TempChar == ' ') && (FileContentsStringCurrentIndex == 0))
          {
            // Too many spaces between command, so error
            // incorrect command from User.  Discard the command.
            ErrorInCommandString = TRUE;
          }
          else
            if ((TempChar == ' ') || (TempChar == '\n') || (TempChar == NULL))
            {
              // Now at the end of the Filename.  Transition to next State.
              // Move to next STATE
              CommandComplete = TRUE;
              
#if (DEBUG_ON)
              Serial.println("DEBUG: FILECONTENTS :");
              Serial.println( FileContentsString);
#endif

            }        
            else
            {
              // Command string good so far.
              FileContentsString[FileContentsStringCurrentIndex] = TempChar;
              FileContentsStringCurrentIndex++; 
              FileContentsString[FileContentsStringCurrentIndex] = NULL;
            }

        // Recieved a full and valid User Command.  
        // Execute the command

        break;

      default:
        // Unknown State!  
        ErrorInCommandString = TRUE; 

        // Not supposed to get to this point, so there is either an error in logic, or an error not being caught in user input.
        // Need to generate an error to indicate so.
        Serial.println("Incorrect logic. #3. Discarding command.");        
        break;

      } // End of Switch Statement
    } // end of while loop

    // If there is an error in the User Command, remove all data from the Serial Port, and reinitialize to start again
    if (ErrorInCommandString == TRUE)
    {
      // Unknown State!  Discard input command and reset to parse next command.
      Serial.println("Incorrect command. #1. Discarding command.");

      // Clear out data in the Serial buffer        
      while (Serial.available() > 0) 
      {
        CommandString[CommandStringSize] = Serial.read();
        CommandStringSize++;
        CommandString[CommandStringSize] = NULL;
      } // end of while   

      // Re-initialize all State Machine Variables to start again
      RunTimeVariablesReset();

    } // end of if ErrorInCommandString == TRUE

    if (CommandComplete == TRUE)
    {
      // The User has entered a full and complete command!
      // Now execute the command!
      switch (CommandType)
      {
      case NO_ACTION:

        // This state is used for accepting in entry data that has no affect.    
        break;        

      case CREATE_A_FILE:
      
#if (DEBUG_ON)
        Serial.println("DEBUG: Create a file!");       
#endif

        if (SD.exists(FileNameString))
        {
#if (DEBUG_ON)
          Serial.println("File exists.");
#endif
        }
        else
        {
          // Create the file
          myFile = SD.open(FileNameString, FILE_WRITE);

          // Fill the file
          myFile.println(FileContentsString);

          // close the file:
          myFile.close(); 

        }
        break;


      case DELETE_A_FILE:
      
#if (DEBUG_ON)      
        Serial.println("DEBUG: Delete a file!"); 
#endif

        if (SD.exists(FileNameString))
        {
          
#if (DEBUG_ON)          
          Serial.println("DEBUG: FILE TO DELETE EXISTS!");
#endif

          if (SD.remove(FileNameString))
          {
#if (DEBUG_ON)            
            Serial.println("DEBUG: DELETED FILE.");
#endif           
          }
          else
          {
#if (DEBUG_ON)            
            Serial.println("DEBUG: COULD NOT DELETE FILE.");
#endif
          }
        }
        break;

      case LIST_FILES:
      
#if (DEBUG_ON)     
        Serial.println("DEBUG: Listing files!");
#endif

        // Open the root directory and list all the files foun
        root = SD.open("/");
        root.rewindDirectory();
        printDirectory(root, 0);
        Serial.println("");
        root.close();

        break;

      case REVEAL_CONTENTS_FILE:
#if (DEBUG_ON)      
        Serial.println("DEBUG: Reveal File Contents!");
#endif

        // Does file exist?
        if (SD.exists(FileNameString))
        {
#if (DEBUG_ON)           
          Serial.println("File exists.");
#endif

          // Open the file
          myFile = SD.open(FileNameString, FILE_READ);
                 
          while ((FileChar = myFile.read()) != -1)
          {
            Serial.print(FileChar);
          }
          Serial.println();
          
        }
        else
        {
          
#if (DEBUG_ON)          
          // File does not exists.
          Serial.println("DEBUG: REVEAL: File does not exist.");
#endif
        }

        break;

      default:
        // Not supposed to get to this point, so there is either an error in logic, or an error not being caught in user input.
        // Need to generate an error to indicate so.
        Serial.println("Incorrect logic. #2. Discarding command."); 
        break;

      } // End of Switch CommandType
      
      // Re-initialize all State Machine Variables to start again
      RunTimeVariablesReset();
      
    } // End of If CommandComplete
  } // end of if (AvailableBytes = Serial.available()
} // end of Loop

// Print out a list of files contained in the directory.
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } 
    else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// RunTimeVariablesReset
// Reset all runtime variables back to init state, to allow restart of processing user commands
void RunTimeVariablesReset(void)
{
      AvailableBytes = 0;
      CommandStringSize = 0;
      CommandStringCurrentIndex = 0;
      FileNameStringCurrentIndex = 0;
      ErrorInCommandString = FALSE;   
      ParseState = COMMAND_TYPE_STATE;    
      CommandComplete = FALSE;      
      CommandType = NO_ACTION;
      FileContentsString[0] = NULL;
      FileContentsStringCurrentIndex = 0;
}













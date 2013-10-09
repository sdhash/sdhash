/**
 * sdhash-srv service definition file.
 */

namespace cpp sdhash


/**
 * exception struct
 */
exception InvalidValue {
  1: i32 what,
  2: string why
}

/**
 * Sdsrv service definition 
 */
service sdhashsrv {

   void ping(),

   string setList(1:bool json),
   string displaySet(1:i32 num1),
   string displayContents(1:i32 num1),
   oneway void compareAll(1:i32 num1, 2:i32 threshold, 3:i32 resultID),
   oneway void compareTwo(1:i32 num1, 2:i32 num2, 3:i32 threshold, 4:i32 sample, 5:i32 resultID),
   i32 loadSet(1:string filename, 2:i32 hashsetID),
   i32 saveSet(1:i32 num1, 2:string filename),
   i32 createHashsetID();
   oneway void hashString(1:string setname, 2:list<string> filenames, 3:i32 blocksize, 4:i32 hashsetID, 5:i32 searchIndex),
   list<string> displaySourceList();
   string getHashsetName(1:i32 num1),

   string displayResult(1:i32 resultID),
   string displayResultsList(1:string user, 2:bool json),
   i32 createResultID(1:string user),
   string displayResultStatus(1:i32 resultID),
   string displayResultDuration(1:i32 resultID),
   string displayResultInfo(1:i32 resultID),
   bool removeResult(1:i32 resultID),
   bool saveResult(1:i32 resultID, 2:string result, 3:string info),

   oneway void shutdown();
}

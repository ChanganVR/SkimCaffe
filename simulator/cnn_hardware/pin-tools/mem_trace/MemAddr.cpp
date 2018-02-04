#include <iostream>
#include <map>
#include <set>
#include <assert.h>
#include <zlib.h>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <cmath>
#include <queue>
#include <list>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cinttypes>


#include "pin.H"
#include "control_manager.H"
#include "portability.H"
#include <iomanip>

#include <string>
#include <unordered_set>

using namespace std;
using namespace CONTROLLER;


#if defined(TARGET_LINUX)
#define PINLINUX
#define WRITEM "w"
#else
#define WRITEM "wb"
#endif


// #define DUMMY_THREAD 100000


/* ===================================================================== */
/* Names of FUNCTIONS */
/* ===================================================================== */

#define STRING_SIZE 200

//#define ROI_FUNC "_Z19BlkSchlsEqEuroNoDivfffffif"

char* ROI_FUNC ;

// const char* ROI_FUNC = "table_pointer";
  template <typename T>
string ToString(T val, bool ishex)
{

  stringstream stream;
  if(ishex) {
    stream << "0X"<<hex<< val;
  }
  else {
    stream << val;
  }
  return stream.str();
}


/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

LOCALVAR CONTROL_MANAGER control;
bool g_enable_instrument = false;
bool g_enable_offload = false;

UINT64 detach_inst = 0;
UINT64 iteration_no=0;
UINT64 iteration_success=0;
UINT64 g_inst_count =0;
UINT64 INSTR_LIMIT = 50000000;
// UINT64 INSTR_LIMIT = 500;
UINT64 g_inst_offload_count =0;

UINT64 g_iter_count = 0;
vector<vector<std::string>> g_mem_set_offload;

std::ofstream OOOMEMFile;


UINT64 ofload_mem = 0;
UINT64 ooo_mem = 0;



/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

// KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
//     "o", "mem-dump.out", "specify axc trace file name");
//
// KNOB<string> KnobOutput_oooFile(KNOB_MODE_WRITEONCE, "pintool",
//     "o", "ooo-dump.out", "specify ooo trace file name");


KNOB<string> Knob_funcfile          (KNOB_MODE_WRITEONCE, "pintool", "func", "function.txt", "File which contains the names of functions to be accelerated");
KNOB<UINT64> Knob_detach_inst       (KNOB_MODE_WRITEONCE, "pintool", "detach", "0", "detach pintool after n instructions");





/* ===================================================================== */



//----------------------------------
//Function declaration
//----------------------------------

// void thread_end(void);
void get_ld_ea(ADDRINT addr, UINT32 mem_read_size);
void get_ld_ea2(ADDRINT addr1, ADDRINT addr2, UINT32 mem_read_size);
void get_st_ea(ADDRINT addr, UINT32 mem_st_size);
void finish(void);

VOID EnterROI();
VOID IterROI();
VOID ExitROI();

void IInstruction(INS ins, void* v);
void IncrementNumInstruction();
void write_string_to_ooo_file(std::string _string);




//////////////////////////////////////////////////////////////////////////////////////////////////////
// control handler for pinpoint (simpoint)
////////////////////////////////////////////////////////////////////////////////////////////////////////

LOCALFUN VOID Handler(EVENT_TYPE ev, VOID* v, CONTEXT * ctxt, VOID* ip, THREADID tid, bool bcast)
{

  switch (ev) {
    case EVENT_START: {
                        cerr << "-> CONTROL START " << g_inst_count << endl;
                        break;
                      }
    case EVENT_STOP: {
                       cerr << "-> Trace Generation Done at icount " << g_inst_count << endl;
                       g_enable_instrument = false;
                       PIN_Detach();
                       break;
                     }
    default: {
               ASSERTX(false);
               break;
             }
  }
}



/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Detach function
////////////////////////////////////////////////////////////////////////////////////////////////////////
void Detach(void *v)
{
  cout << "Detaching..." << endl;
  finish();
}



/////////////////////////////////////////////////////////////////////////////////////////
/// Finalization
/// Create configuration file
/// Final print to standard output
/////////////////////////////////////////////////////////////////////////////////////////
void finish(void)
{
  cout<<"Total Instructions in Parent Function: "<<g_inst_count<<"\n";
  cout<<"Total Memory instructions: " << ooo_mem << endl;
  cout<<"Total iterations: " << ooo_mem << endl;
  cout<<"INSTR_LIMIT " << INSTR_LIMIT << endl;
  cout<<"Exiting ...\n";



}




void write_string_to_ooo_file(std::string _string)
{

  // ----------------------------------------
  // detach : instruction counts exceed the limit
  // can we replace g_inst_count with trace_info->inst_count??
  // ----------------------------------------
  // if ( g_inst_count > INSTR_LIMIT) {
  if ( ooo_mem > INSTR_LIMIT) {
    cout << "Detaching after " << g_inst_count << " instructions " <<" mem_instr "<<ooo_mem << endl;
    PIN_Detach();


  }
  // cout<<"instructions: "<<g_inst_count <<endl;

  OOOMEMFile<<_string;
  OOOMEMFile<<"\n";
}




VOID IterROI()
{
  std::cout << "Enter new iteration ROI: iter_count "<< g_iter_count<<"  ooo_mem "<< ooo_mem << " instructions " << g_inst_count << endl;
  ++g_iter_count;
}

VOID EnterROI()
{
  std::cout << "App Enter ROI :" << endl;
  g_enable_instrument = true;
}

VOID ExitROI()
{
  std::cout << "App Exit ROI :" << endl;
  g_enable_instrument = false;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////
// Instruction - instrumentation
////////////////////////////////////////////////////////////////////////////////////////////////////////
void IInstruction(INS ins, void* v)
{
  // Count instructions
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)IncrementNumInstruction, IARG_END);
  // ----------------------------------------
  // Load instruction
  // ----------------------------------------
  if (INS_IsMemoryRead(ins)) {


    // 2 memory loads
    if (INS_HasMemoryRead2(ins)) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_ld_ea2, IARG_MEMORYREAD_EA, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
          IARG_END);
    }
    // 1 memory load
    else {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_ld_ea, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,IARG_END);
    }

  }



  // ----------------------------------------
  // Store instruction
  // ----------------------------------------
  if (INS_IsMemoryWrite(ins)) {

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_st_ea, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,IARG_END);
  }



}



/////////////////////////////////////////////////////////////////////////////////////////
// get an effetive load address : instrumentation function
/////////////////////////////////////////////////////////////////////////////////////////
void get_ld_ea(ADDRINT addr, UINT32 mem_read_size)
{

  if(g_enable_instrument && !g_enable_offload)
  {
    ooo_mem++;
    std::string str = ToString(g_inst_count,false) + " R "  +  ToString(addr,true) + " " +  ToString(addr,true) + " "   + ToString(mem_read_size,false);
    write_string_to_ooo_file(str);
  }
}

void get_ld_ea2(ADDRINT addr1, ADDRINT addr2, UINT32 mem_read_size)
{
  if(g_enable_instrument && !g_enable_offload)
  {

    ooo_mem++;

    std::string str = ToString(g_inst_count,false) + " R "  +  ToString(addr1,true) + " " +  ToString(addr1,true) + " "   + ToString(mem_read_size,false);
    write_string_to_ooo_file(str);


    ooo_mem++;

    std::string str2 = ToString(g_inst_count ,false) + " R "  +  ToString(addr2,true) + " " +  ToString(addr2,true) + " "   + ToString(mem_read_size,false);
    write_string_to_ooo_file(str2);
  }

}

/////////////////////////////////////////////////////////////////////////////////////////
// Get store address : instrumentation function
/////////////////////////////////////////////////////////////////////////////////////////
void get_st_ea(ADDRINT addr, UINT32 mem_st_size)
{
  if(g_enable_instrument && !g_enable_offload)
  {

    ooo_mem++;

    std::string str = ToString(g_inst_count,false) + " W "  +  ToString(addr,true) + " " +  ToString(addr,true) + " "   + ToString(mem_st_size,false);
    write_string_to_ooo_file(str);
  } 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Increment the number of instructions
////////////////////////////////////////////////////////////////////////////////////////////////////////
void IncrementNumInstruction()
{

  if(!g_enable_instrument)
    return;

  if(g_enable_offload)
    g_inst_offload_count++;

  g_inst_count++;

}

/////////////////////////////////////////////////////////////////////////////////////////
// Main instrumentation routine
// Depending on the type of an instruction, call corresponding subroutines.
// Do not modify this routine!!
/////////////////////////////////////////////////////////////////////////////////////////





/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */

VOID Image(IMG img, VOID *v)
{
  for( SEC sec= IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) )
  {
    for( RTN rtn= SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) )
    {
      string rtnName =  PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY);

      /*** Control Markers ***/

      if (rtnName == string("__app_roi_begin"))
      {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) EnterROI, IARG_END);
        RTN_Close(rtn);
      }

      else if (rtnName == string("__app_roi_end"))
      {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) ExitROI, IARG_END);
        RTN_Close(rtn);
      }

      if (rtnName == string("__app_roi_iter_begin"))
      {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) IterROI, IARG_END);
        RTN_Close(rtn);
      }
    }
  }


  cout << "Image Instrumentation\n";



}






/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
  finish();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
  cerr << "This tool produces a trace of calls to offload_func1." << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}






/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
  cout<<"start\n";


  ifstream funcfile;
  funcfile.open(Knob_funcfile.Value().c_str(),ios::in);

  if(!funcfile.is_open())
    assert(false && "Unable to open accelerated function names file (default: function.txt)");

  ROI_FUNC = new char[STRING_SIZE];

  std::string line;
  getline(funcfile, line); 

  line.copy(ROI_FUNC, line.size(), 0);

  cout<<"ROI_FUNC:"<<ROI_FUNC<<endl;

  // Initialize pin & symbol manager
  PIN_InitSymbols();
  if( PIN_Init(argc,argv) )
  {
    return Usage();
  }

  detach_inst = Knob_detach_inst.Value();

  // OOOMEMFile.open(KnobOutput_oooFile.Value().c_str());
  OOOMEMFile.open("ooo-dump.out");
  OOOMEMFile << hex;
  OOOMEMFile.setf(ios::showbase);


  control.RegisterHandler(Handler, 0, FALSE);
  control.Activate();


  // Register Image to be called to instrument functions.
  IMG_AddInstrumentFunction(Image, 0);
  PIN_AddFiniFunction(Fini, 0);
  INS_AddInstrumentFunction(IInstruction, 0);
  PIN_AddDetachFunction(Detach, 0);

  // Never returns
  PIN_StartProgram();

  return 0;
}


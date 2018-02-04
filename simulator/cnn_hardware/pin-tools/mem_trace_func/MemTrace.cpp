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




/* ===================================================================== */
/* Names of FUNCTIONS */
/* ===================================================================== */

#define STRING_SIZE 200


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

std::ofstream OOOMEMFile;
LOCALVAR CONTROL_MANAGER control;
bool g_enable_instrument = false;

UINT64 g_roi_inst_count =0;
UINT64 g_inst_count =0;
UINT64 INSTR_LIMIT = 50000000;
// UINT64 INSTR_LIMIT = 500;
UINT64 ooo_mem = 0;

//----------------------------------
//Function declaration
//----------------------------------

void get_ld_ea(ADDRINT addr, UINT32 mem_read_size);
void get_ld_ea2(ADDRINT addr1, ADDRINT addr2, UINT32 mem_read_size);
void get_st_ea(ADDRINT addr, UINT32 mem_st_size);
void finish(void);

void IInstruction(INS ins, void* v);
void docount();
void write_string_to_ooo_file(std::string _string);

VOID Trace(TRACE trace, VOID *v);
VOID EnterROI();
VOID ExitROI();


//////////////////////////////////////////////////////////////////////////////////////////////////////
// control handler for pinpoint (simpoint)
////////////////////////////////////////////////////////////////////////////////////////////////////////

LOCALFUN VOID Handler(EVENT_TYPE ev, VOID* v, CONTEXT * ctxt, VOID* ip, THREADID tid, bool bcast)
{

  switch (ev) {
    case EVENT_START: {
                        cerr << "-> CONTROL START " << g_roi_inst_count << endl;
                        break;
                      }
    case EVENT_STOP: {
                       cerr << "-> Trace Generation Done at icount " << g_roi_inst_count << endl;
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


/* ===================================================================== */
VOID EnterROI()
{
  g_enable_instrument = true;
  std::cout << "App Enter ROI at Inst#: " << g_roi_inst_count << endl;
  // std::cout << "App Enter  abs Inst#: " << g_inst_count << endl;


}


VOID ExitROI()
{
  g_enable_instrument = false;
  std::cout << "App Exit ROI at Inst#: " << g_roi_inst_count << endl;
  // std::cout << "App Exit abs Inst#: " << g_inst_count << endl;

}




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

  OOOMEMFile.close();
  cout<<"Total Instructions in ROI: "<<g_roi_inst_count<<"\n";
  cout<<"Total Instructions abs: "<<g_inst_count<<"\n";
  cout<<"Total Memory instructions: " << ooo_mem << endl;
  cout<<"INSTR_LIMIT " << INSTR_LIMIT << endl;
  cout<<"Exiting ...\n";
}


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

      if (rtnName == string("__app_roi_end"))
      {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) ExitROI, IARG_END);
        RTN_Close(rtn);
      }

    }
  }


  cout << "Image Instrumentation\n";

}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Instruction - instrumentation
////////////////////////////////////////////////////////////////////////////////////////////////////////
void IInstruction(INS ins, void* v)
{

  // if(g_enable_instrument)
  //   std::cout<<INS_Mnemonic(ins)<<endl;


  // Count instructions
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

  // ----------------------------------------
  // Load instruction
  // ----------------------------------------
  if (INS_IsMemoryRead(ins)) {

    // if(g_enable_instrument)
    //   std::cout<<"R "<<INS_Mnemonic(ins);
    // 2 memory loads
    if (INS_HasMemoryRead2(ins)) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_ld_ea2, IARG_MEMORYREAD_EA, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
          IARG_END);

      // if(g_enable_instrument) std::cout<<" 2"<<endl;

    }
    // 1 memory load
    else {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_ld_ea, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,IARG_END);

      // if(g_enable_instrument) std::cout<<" 1"<<endl;
    }
  }

  // ----------------------------------------
  // Store instruction
  // ----------------------------------------
  if (INS_IsMemoryWrite(ins)) {

    if(g_enable_instrument)
      // std::cout<<"W "<<INS_Mnemonic(ins)<<endl;
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)get_st_ea, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,IARG_END);
  }

}



/////////////////////////////////////////////////////////////////////////////////////////
// get an effetive load address : instrumentation function
/////////////////////////////////////////////////////////////////////////////////////////
void get_ld_ea(ADDRINT addr, UINT32 mem_read_size)
{

  if(!g_enable_instrument)
    return;

  ooo_mem++;
  // std::string str = ToString(g_roi_inst_count,false) + " R "  +  ToString(addr,true) + " " +  ToString(addr,true) + " "   + ToString(mem_read_size,false);
  std::string str = "0 "+ ToString(ooo_mem,false)+ " " + ToString(addr,true) + " " + ToString(mem_read_size,false) + " 0 0 false";
  write_string_to_ooo_file(str);
}

void get_ld_ea2(ADDRINT addr1, ADDRINT addr2, UINT32 mem_read_size)
{
  if(!g_enable_instrument)
    return;


  ooo_mem ++;

  // std::string str = ToString(g_roi_inst_count,false) + " R "  +  ToString(addr1,true) + " " +  ToString(addr1,true) + " "   + ToString(mem_read_size,false);
  std::string str = "0 " + ToString(ooo_mem,false)+ " " + ToString(addr1,true) + " " + ToString(mem_read_size,false) + " 0 0 false";
  write_string_to_ooo_file(str);

  ooo_mem ++;
  // std::string str2 = ToString(g_roi_inst_count ,false) + " R "  +  ToString(addr2,true) + " " +  ToString(addr2,true) + " "   + ToString(mem_read_size,false);
  std::string str2 = "1 " + ToString(ooo_mem,false)+ " " + ToString(addr2,true) + " " + ToString(mem_read_size,false) + " 0 0 false";
  write_string_to_ooo_file(str2);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Get store address : instrumentation function
/////////////////////////////////////////////////////////////////////////////////////////
void get_st_ea(ADDRINT addr, UINT32 mem_st_size)
{

  if(!g_enable_instrument)
    return;
  ooo_mem++;

  // std::string str = ToString(g_roi_inst_count,false) + " W "  +  ToString(addr,true) + " " +  ToString(addr,true) + " "   + ToString(mem_st_size,false);
  std::string str = "2 " + ToString(ooo_mem,false)+ " " + ToString(addr,true) + " " + ToString(mem_st_size,false) + " 0 0 true";
  write_string_to_ooo_file(str);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Increment the number of instructions
////////////////////////////////////////////////////////////////////////////////////////////////////////
void docount()
{
  if(g_enable_instrument)
    g_roi_inst_count++;

  g_inst_count++;
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

  // Initialize pin & symbol manager
  PIN_InitSymbols();
  if( PIN_Init(argc,argv) )
  {
    return Usage();
  }

  OOOMEMFile.open("ooo-dump.out");
  // OOOMEMFile << hex;
  OOOMEMFile.setf(ios::showbase);
  


  control.RegisterHandler(Handler, 0, FALSE);
  control.Activate();


  IMG_AddInstrumentFunction(Image, 0);
  // TRACE_AddInstrumentFunction(Trace, 0);
  INS_AddInstrumentFunction(IInstruction,0); 
  PIN_AddFiniFunction(Fini, 0);
  PIN_AddDetachFunction(Detach, 0);

  // Never returns
  PIN_StartProgram();

  return 0;
}

void write_string_to_ooo_file(std::string _string)
{

  // std::cout << _string<<endl;

  OOOMEMFile<<_string;
  OOOMEMFile<<"\n";
  // ----------------------------------------
  // detach : instruction counts exceed the limit
  // can we replace g_roi_inst_count with trace_info->inst_count??
  // ----------------------------------------
  if ( ooo_mem > INSTR_LIMIT) {
    cout << "Detaching after roi " << g_roi_inst_count << " instructions " <<" mem_instr "<<ooo_mem << endl;
    PIN_Detach();
  }
  // cout<<"instructions: "<<g_roi_inst_count <<endl;

}



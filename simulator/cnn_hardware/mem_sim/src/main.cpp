#include <iostream>
#include "O3sim_ruby.h"
#include "memory.hpp"
#include <zlib.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "data_type.hpp"
#include <list> 

using namespace std;
static const unsigned BUFLEN = 1024;

void error(const char* const msg)
{
    std::cerr << msg << "\n";
    exit(255);
}


void pop_mem_req(std::list<uint64_t> nodeid_list, std::list<uint64_t>& g_mem_req) {
  for( auto &n : nodeid_list)
  {
    D(cerr<<"graph: get resp from memory nodeid: "<<n
        <<" g_clk: "<<g_time
        <<endl);
    g_mem_req.remove(n);
  }

}

void advance_time(std::shared_ptr<O3sim_ruby> m_ruby, uint64_t & g_time) {
  m_ruby->advance_time();
  g_time++;

}


  int 
main(int argc, char* argv[])
{


  //------------------------------------------------------------------------------------------------

  uint64_t g_time=0;
  bool ishash2 = false;
  uint64_t mem_lat = 20;
  int _mem_req_stack = 32;
  std::list<uint64_t> g_mem_req;

  std::shared_ptr<O3sim_ruby> m_ruby (new O3sim_ruby(1, 1,4,1, true, true, 1 ,"","med","/dev/null"));
  m_ruby->initialize();

  Memory m_memory (Memory(m_ruby,"LSB_Counting-256:128" ,ishash2, mem_lat, _mem_req_stack));
  m_memory.initialize();

    // m_memory.fill_global_memaddr_map( iter, nodeid , MemValue {isWrite,addr});
  // auto nodeid_list = m_memory.recv_resp(m_prev_completion_cycles);
  // m_memory.print_mem_stats();
  //
  // m_ruby->advance_time();

    gzFile in = gzopen("test.gz", "rb");
    if (in == Z_NULL) std:cerr<<"Failed to open test.gz"<<endl;
    char buf[BUFLEN];
    char* offset = buf;

    for (;;) {
        int err, len = sizeof(buf)-(offset-buf);
        if (len == 0) error("Buffer to small for input line lengths");

        len = gzread(in, offset, len);

				if (len == 0) break;    
				if (len <  0) error(gzerror(in, &err));

				char* cur = buf;
				char* end = offset+len;

				for (char* eol; (cur<end) && (eol = std::find(cur, end, '\n')) < end; cur = eol + 1)
				{
					std::string str = std::string(cur, eol);
					istringstream iss(str);
					// do
					// {
          //
          //
					// 	string subs;
					// 	iss >> subs;
					// 	cout << "Substring: " << subs << endl;
					// } while (iss);

           uint64_t iter;
           uint64_t pc;
           bool isWrite;
           Addr vaddr;
           Addr byte_size;
           uint64_t core_id;
           uint64_t thread_id;

           iss>>iter;
           iss>>pc;
           iss>>hex>>vaddr;
           iss>>byte_size;
           iss>>core_id;
           iss>>thread_id;
           iss>>isWrite;

           // cout<<"iter: "<<iter<<" pc: "<<pc<<" vaddr: "<<vaddr
           //   <<" byte_size: "<<byte_size<<" core_id: "<<core_id
           //   <<" thread_id: "<<thread_id<<" isWrite: "<<isWrite<<endl;

           bool isSucessful = false;
           while(!isSucessful) {
             isSucessful= m_memory.send_req( pc,isWrite, vaddr, byte_size, core_id, thread_id);
             if(isSucessful) g_mem_req.push_back(pc); 

             if(g_mem_req.size() > 0)
               pop_mem_req(m_memory.recv_resp(g_time), g_mem_req);

             advance_time(m_ruby,g_time);

             // m_ruby->advance_time();
             // g_time++;
           }

        }

				// any trailing data in [eol, end) now is a partial line
				offset = std::copy(cur, end, buf);
		}

		// BIG CATCH: don't forget about trailing data without eol :)
		std::cout << std::string(buf, offset);

		if (gzclose(in) != Z_OK) error("failed gzclose");


    while(g_mem_req.size() > 0){
      // cout<<"g_mem_req_size: "<<g_mem_req.size()<<endl;
      pop_mem_req(m_memory.recv_resp(g_time), g_mem_req);

      advance_time(m_ruby,g_time);
      // g_time++;
      // m_ruby->advance_time();

    }









    //------------------------------------------------------------------------------------------------
    std::ofstream ruby_stat_file("ruby.stat.out", ios::out);
    m_ruby->print_stats(ruby_stat_file);
    ruby_stat_file.close();
    m_ruby->destroy();

    cout<<"Total time: "<<g_time<<endl;
    return 0;
}



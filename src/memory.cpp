#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/sysinfo.h"
#include "sys/types.h"
#include <map>

#include "initialize.h"

#define VNV_PAPI_ERROR(r) if (r!= PAPI_OK) { VnV_Error("Could not create PAPI Event Set (%s)", PAPI_descr_error(r)); }
       

namespace {

long parseLine(char *line) {
  // This assumes that a digit will be found and the line ends in " Kb".
  long i = strlen(line);
  const char *p = line;
  while (*p < '0' || *p > '9')
    p++;
  line[i - 3] = '\0';
  i = atol(p);
  return i * 1000;
}

} // namespace

class ProcInfo {
public:
  long long totalVirtualMem, virtualMemoryUsed, processVirtualMemory,
      peakProcessVirtualMemory;
  long long totalPhysicalMem, phyiscalMemoryUsed, processPhysicalMemory,
      peakProcessPhysicalMemory;
};

ProcInfo getMemoryInfo() {

  struct sysinfo memInfo;

  sysinfo(&memInfo);

  ProcInfo j;

  // System Level Information

  // Total virtual mem
  long long totalVirtualMem = memInfo.totalram;
  totalVirtualMem += memInfo.totalswap;
  totalVirtualMem *= memInfo.mem_unit;
  j.totalVirtualMem = totalVirtualMem;

  // virtual memory used
  long long virtualMemUsed = memInfo.totalram - memInfo.freeram;
  virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
  virtualMemUsed *= memInfo.mem_unit;
  j.virtualMemoryUsed = virtualMemUsed;

  // total physical memory
  long long totalPhysMem = memInfo.totalram;
  totalPhysMem *= memInfo.mem_unit;
  j.totalPhysicalMem = totalPhysMem;

  // physical memory used.
  long long physMemUsed = memInfo.totalram - memInfo.freeram;
  physMemUsed *= memInfo.mem_unit;
  j.phyiscalMemoryUsed = physMemUsed;

  // process virtual memory used.
  int processVirtualMem = -1;
  {
    FILE *file = fopen("/proc/self/status", "r");
    char line[128];

    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "VmPeak:", 7) == 0) {
        j.peakProcessVirtualMemory = parseLine(line);
      } else if (strncmp(line, "VmSize:", 7) == 0) {
        j.processVirtualMemory = parseLine(line);
      } else if (strncmp(line, "VmHWM:", 6) == 0) {
        j.peakProcessPhysicalMemory = parseLine(line);
      } else if (strncmp(line, "VmRSS:", 6) == 0) {
        j.processPhysicalMemory = parseLine(line);
        break;
      }
    }
    fclose(file);
  }

  return j;
}

void hardware(VnV::IOutputEngine* engine) {
  
  VnVPapi::InitalizePAPI();

  const PAPI_hw_info_t *hwinfo = PAPI_get_hardware_info();

  if (hwinfo == nullptr) {
    return;
  }
    struct utsname uname_info;
    uname(&uname_info);
    nlohmann::json uinfo = nlohmann::json::object();

    VnV::MetaData d;
    d["table"] = "hardware";

    engine->Put("Operating System", uname_info.sysname, d);
    engine->Put("Release", uname_info.release, d);
    engine->Put("Node Name", uname_info.nodename, d);
    engine->Put("Version", uname_info.version, d);
    engine->Put("Machine", uname_info.machine, d);
    engine->Put("CPU Revision", hwinfo->revision, d);

    std::ostringstream oss;
    oss << hwinfo->vendor_string << "(" << hwinfo->vendor << ")";
    engine->Put("vendor", oss.str(), d);

    std::ostringstream os1;
    os1 << hwinfo->model_string << "(" << hwinfo->model << ")";
    engine->Put("model", os1.str(), d);

    engine->Put("CPU Max MHz", hwinfo->cpu_max_mhz, d);
    engine->Put("CPU Min MHz", hwinfo->cpu_min_mhz, d);
    engine->Put("Total Cores", hwinfo->totalcpus, d);
    engine->Put("SMT Threads Per Core", hwinfo->threads, d);
    engine->Put("Cores Per Socket", hwinfo->cores, d);
    engine->Put("Sockets", hwinfo->sockets, d);
    engine->Put("NUMA Regions", hwinfo->nnodes, d);
    engine->Put("Running in a VM", hwinfo->virtualized, d);

    if (hwinfo->virtualized) {
      engine->Put("VM Vendor", hwinfo->virtual_vendor_string, d);
      engine->Put("VM Vendor Version", hwinfo->virtual_vendor_version, d);
    }
    
    auto a = getMemoryInfo();
    engine->Put("totalPhys", a.totalPhysicalMem, d);
    
}




class ProcAction : public VnV::IAction {

  int EventSet = PAPI_NULL;
  std::size_t EventSetCounter = 0;
  std::vector<long long> values;

public:
  ProcAction(const nlohmann::json &config) {
    implements_injection_point(true);
    VnVPapi::InitalizePAPI();

    int r = PAPI_create_eventset(&EventSet); VNV_PAPI_ERROR(r);
    r = PAPI_add_event(EventSet, PAPI_TOT_INS); VNV_PAPI_ERROR(r);
    r = PAPI_add_event(EventSet, PAPI_TOT_CYC); VNV_PAPI_ERROR(r);
    r = PAPI_add_event(EventSet, PAPI_L1_DCM); VNV_PAPI_ERROR(r);
    
    values.resize(3);
    
  }

  void write(std::string name) {

    PAPI_read(EventSet, &values[0]);


    auto a = getMemoryInfo();

    getEngine()->Put("name", name);
    getEngine()->Put("system_phys_used", a.phyiscalMemoryUsed);
    getEngine()->Put("process_phys_used", a.processPhysicalMemory);
    getEngine()->Put("peak_process_phys_used", a.peakProcessPhysicalMemory);
    getEngine()->Put("instructions", values[0]);
    getEngine()->Put("cycles", values[1]);
    getEngine()->Put("l1", values[2]);
    
  }

  virtual void initialize() override {
    
    PAPI_start(EventSet);
    hardware(getEngine());    
    write("Initialize");
  };

  virtual void injectionPointStart(std::string packageName, std::string id) {
    write(packageName + ":" + id);
  };

  virtual void injectionPointIteration(std::string stageId) { write(stageId); };

  virtual void injectionPointEnd() { write("End"); };

  virtual void finalize() { 
    
    write("Finalize");
    PAPI_stop(EventSet, &values[0]);

  }
};

/**
 * @title Papi Memory And Hardware Monitor
 * @shortTitle Memory Usage
 * 
 * These results show memory usage statistics collected 
 * by the Papi plugin during runtime. 
 * 
 *
 * Hardware Information
 * --------
 *  
 * .. vnv-quick-table::
 *    :names: ["Property", "Value"]
 *    :fields: ["name", "value"]
 *    :data: {{ as_json(*|[?_table==`hardware`].{ "name" : Name , "value" : Value }) }}
 *
 * .. vnv-plotly::
 *    :trace.ram: scatter
 *    :trace.pram: scatter
 *    :trace.peak: scatter
 *    :trace.inst: scatter
 *    :trace.cycl: scatter
 *    :ram.y: {{system_phys_used}}
 *    :ram.x: {{name}}
 *    :pram.y: {{process_phys_used}}
 *    :pram.x: {{name}}
 *    :peak.y: {{peak_process_phys_used}}
 *    :peak.x: {{name}}
 *    :inst.y: {{instructions}}
 *    :inst.x: {{name}} 
 *    :cycl.y: {{cycles}}
 *    :cycl.x: {{name}}
 * 
 * .. vnv-plotly::
 *    :trace.fram: scatter
 *    :trace.pram: scatter
 *    :trace.peak: scatter
 *    :trace.inst: scatter
 *    :trace.cycl: scatter
 *    :fram.y: {{vec_delta(system_phys_used)}}
 *    :fram.x: {{name}}
 *    :pram.y: {{vec_delta(process_phys_used)}}
 *    :pram.x: {{name}}
 *    :peak.y: {{vec_delta(peak_process_phys_used)}}
 *    :peak.x: {{name}}
 *    :inst.y: {{vec_delta(instructions)}}
 *    :inst.x: {{name}} 
 *    :cycl.y: {{vec_delta(cycles)}}
 *    :cycl.x: {{name}}
 */

INJECTION_ACTION(PNAME, Monitor, "{}") { return new ProcAction(config); }



#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/sysinfo.h"
#include "sys/types.h"
#include <map>

#include "initialize.h"

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

class ProcAction : public VnV::IAction {
public:
  ProcAction(const nlohmann::json &config) {
    implements_injection_point(true);
    VnVPapi::InitalizePAPI();
  }

  void write(std::string name) {
    auto a = getMemoryInfo();

    getEngine()->Put("name", name);
    getEngine()->Put("system_phys_used", a.phyiscalMemoryUsed);
    getEngine()->Put("process_phys_used", a.processPhysicalMemory);
    getEngine()->Put("peak_process_phys_used", a.peakProcessPhysicalMemory);
  }

  virtual void initialize() override {
    
    auto engine = getEngine();

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
    engine->Put("totalPhys", a.totalPhysicalMem);
    write("Initialize");


  };

  virtual void injectionPointStart(std::string packageName, std::string id) {
    write(packageName + ":" + id);
  };

  virtual void injectionPointIteration(std::string stageId) { write(stageId); };

  virtual void injectionPointEnd() { write("End"); };

  virtual void finalize() { write("Finalize"); }
};

/**
 *
 * Total Physical Memory: :vnv:`totalPhys` Bytes
 * ------------------------------------------
 *
 * .. vnv-plotly::
 *    :trace.ram: scatter
 *    :trace.pram: scatter
 *    :trace.peak: scatter
 *    :ram.y: {{system_phys_used}}
 *    :pram.y: {{process_phys_used}}
 *    :peak.y: {{peak_process_phys_used}}
 *
 * Hardware
 * --------
 *  
 * .. vnv-quick-table::
 *    :names: ["Property", "Value"]
 *    :fields: ["name", "value"]
 *    :data: *|[?_table==`hardware`].{ "name" : Name , "value" : Value }
 *
 * 
 */
INJECTION_ACTION(PNAME, Monitor, "{}") { return new ProcAction(config); }
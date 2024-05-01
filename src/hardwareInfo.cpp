
#include "initialize.h"


INJECTION_PLUGIN(Papi)

/**
 * 
 * COMPUTER HARDWARE INFORMATION
 * ========================================
 *
 * Compute hardware is extracted using PAPI at runtime.. All the information available
 * is: 
 * 
 * .. vnv-quick-table::
 *    :names: ["Property", "Value"]
 *    :fields: ["name", "value"]
 *    :data: {{*|[?_table==`hardware`].{ "name" : Name , "value" : Value }}}
 *
 */
INJECTION_ACTION(PNAME, Hardware, "{\"type\":\"object\"}") {

  class PapiHardwareAction : public VnV::IAction {

    json config;

  public:
    PapiHardwareAction(const json &config) { 
      VnVPapi::InitalizePAPI();
      this->config = config;
    }

    void initialize() override {
      auto engine = getEngine();

      const PAPI_hw_info_t *hwinfo = PAPI_get_hardware_info();

      if (hwinfo == nullptr) {
        return;
      }

      struct utsname uname_info;
      uname(&uname_info);
      nlohmann::json uinfo = nlohmann::json::object();

      MetaData d;
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
    }
  };

  return new PapiHardwareAction(config);
}




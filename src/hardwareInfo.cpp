
#include "VnV.h"
#include "papi.h"
#include <sys/utsname.h>
#include <iostream>

#define PNAME Papi


using namespace VnV;

namespace {

static bool PAPI_IS_INITIALIZED = false;

bool InitalizePAPI() {
  if (!PAPI_IS_INITIALIZED) {
     int retVal = PAPI_library_init( PAPI_VER_CURRENT );
     PAPI_IS_INITIALIZED = (retVal == PAPI_VER_CURRENT);
  }
  return PAPI_IS_INITIALIZED;
 }

void getDefaultHWJson(VnV::IOutputEngine* engine ) {

   nlohmann::json j = nlohmann::json::object();
   const PAPI_hw_info_t* hwinfo = PAPI_get_hardware_info();

   if (hwinfo == nullptr) {
      return;
   }

   struct utsname uname_info;
   uname(&uname_info);
   nlohmann::json uinfo = nlohmann::json::object();
   
   MetaData d;
   d["table"] = "table1";

   engine->Put("Operating System", uname_info.sysname, d);
   engine->Put("Release", uname_info.release, d);
   engine->Put("Node Name", uname_info.nodename, d);
   engine->Put("Version", uname_info.version, d);
   engine->Put("Machine", uname_info.machine, d);
   engine->Put("CPU Revision",hwinfo->revision, d);
   
   std::ostringstream oss;
   oss << hwinfo->vendor_string << "(" << hwinfo->vendor << ")";
   engine->Put("vendor", oss.str(), d);
  
   std::ostringstream os1;
   os1 << hwinfo->model_string << "(" << hwinfo->model << ")";
   engine->Put("model", os1.str(), d);
   
   d["table"] = "table2";

   engine->Put("CPU Max MHz", hwinfo->cpu_max_mhz, d);
   engine->Put("CPU Min MHz", hwinfo->cpu_min_mhz, d);
   engine->Put("Total Cores", hwinfo->totalcpus, d);
   engine->Put("SMT Threads Per Core", hwinfo->threads, d);
   engine->Put("Cores PER Socket", hwinfo->cores, d);
   engine->Put("Sockets", hwinfo->sockets, d);
   engine->Put("NUMA Regions", hwinfo->nnodes, d);
   engine->Put("Running in a VM", hwinfo->virtualized, d);
   
   if (hwinfo->virtualized) {
    engine->Put("VM Vendor", hwinfo->virtual_vendor_string ,d);
    engine->Put("VM Vendor Version", hwinfo->virtual_vendor_version , d);
   }
}

}

class PapiHardwareAction : public VnV::IAction {
public:

   PapiHardwareAction() {
      InitalizePAPI();
   }

   void initialize() override {
      getDefaultHWJson(getEngine());
   }
};


/**
 * COMPUTER HARDWARE AND MEMORY INFORMATION
 * ========================================
 *
 * The compute hardware information is:
 * 
 * .. vnv-quick-table::
 *    :names: ["Property", "Value"]
 *    :fields: ["name", "value"]
 *    :data: *|[]|[?_table==`table1`].{ "name" : name , "value" : value }
 *    :title: Computer Information    
 *     
 * 
 * The processor information is:
 * 
 * 
 * .. vnv-quick-table::
 *    :names: ["Property", "Value"]
 *    :fields: ["name", "value"]
 *    :data: *|[]|[?_table==`table2`].{ "name" : name , "value" : value }
 *    :title: Processor Information    
 *     
 *    
 * .. vnv-dashboard::  
 * 
 *   .. vnv-db-widget::
 *      :row: 0
 *      :col: 0
 *      :size-x: 6
 *      :size-y: 2
 * 
 *      .. vnv-gauge::
 *         :title: Hello
 *         :min: `10`
 *         :max: `200`
 *         :curr: `120`
 *      
 * 
 *   .. vnv-db-widget::
 *      :row: 0
 *      :col: 6
 *      :size-x: 6
 *      :size-y: 2
 *      
 *      .. vnv-line::
 *         :title: Test Line Chart
 *         :xaxis: `["e","f","g"]`
 *         :yaxis: `[1,2,3]`
 *         :label: Sample Series
 *      
 * 
 *   .. vnv-db-widget::
 *      :row: 2
 *      :col: 0
 *      :size-x: 6
 *      :size-y: 2     
 * 
 *      .. vnv-multi-line::
 *         :title: Multi Line Chart 
 *         :xaxis: `["e","f","g"]`
 *         :yaxis: ["`[1,2,3]`","`[2,4,6]`","`[2,4,6]`"]
 *         :labels: ["Series 1", "Series 2","Series 3"] 
 *         :type: ["line", "area", "column"]
 *      
 * 
 *   .. vnv-db-widget::
 *      :row: 2
 *      :col: 6
 *      :size-x: 6
 *      :size-y: 2
 *      
 *      .. vnv-time-series::
 *         :title: Time Series Example
 *         :times: `[10,20,30,40,60]`
 *         :data: `[10,20,20,30,50]`
 *         :label: Time Data
 *       
 * .. vnv-scatter::
 *         :title: Scatter Example
 *         :xdata: `[10,20,30,40,60]`
 *         :ydata: `[10,20,20,30,50]`
 *         :label: Scatter Data
 *          
 * 
 * .. vnv-if:: `1`==`1`
 *  
 *    Welcome to the machine. 
 * 
 */
INJECTION_ACTION(PNAME,hardware, "{\"type\":\"object\"}") {
  return new PapiHardwareAction();
}

class flopsRunner {
public:
  int count = 0;
  std::size_t EventSetCounter = 0;
  bool started = false;
  int EventSet = PAPI_NULL;
  std::vector<long long> values;

  flopsRunner() {
    InitalizePAPI();
    int r = PAPI_create_eventset(&EventSet);
    if ( r != PAPI_OK) {
        std::cout << PAPI_descr_error(r) << std::endl;;
      }
  }
  ~flopsRunner() {
    PAPI_destroy_eventset(&EventSet);
  }

  int addEvent(int id) {
    if (!started) {
       int r = PAPI_add_event(EventSet,id);
       if (r == PAPI_OK)
         EventSetCounter++;
       else
         std::cout << "SDFSDFSDF " << id << " " << PAPI_TOT_CYC << PAPI_descr_error(r) << std::endl;;
       return r;
    }
    return -1;
  }

  std::vector<long long>& start() {
    started = true;
    values.resize(EventSetCounter,0);
    int r = PAPI_start(EventSet);
    return values;
  }

  void reset() {
    std::fill(values.begin(),values.end(),0);
  }

  std::vector<long long>& stop() {
    started = false;
    PAPI_stop(EventSet, &values[0]);
    return values;
  }

  // Accum adds to values and then resets counters.
  // Read copies counters into values, does not reset.
  std::vector<long long>& iter( bool accum ) {
    if (accum) {
       PAPI_accum(EventSet, &values[0]);
    } else {
       PAPI_read(EventSet, &values[0]);
    }
    return values;
  }

};


/**
 * Recorded Floating point operations
 * ==============================================
 *
 * The figure below shows the number of floating point operations recorded
 * using PAPI throughout the duration of this execution.
 *
 * .. vnv-quick-chart::
 *      :name: FLOPS
 *      :data: fpins 
 *      :title: Floating point operations. 
 *  
 * .. note::
 *    Counters will include any cost associated with injection point tests in child nodes. Users should use caution when using nested profiling with this toolkit.
 *
 */
INJECTION_TEST_R(PNAME,flops, flopsRunner) {
  engine->Put("fpins",runner->count++);
  return SUCCESS;
}


INJECTION_TEST_R(PNAME,flops1, flopsRunner) {
  
  InitalizePAPI();
  if (type == InjectionPointType::Single) {
     engine->Put( "stage", stageId);
     engine->Put( "cycles", 0);
     engine->Put( "fpins",0);
     return SUCCESS;
  } else if (type == InjectionPointType::Begin) {
      runner->addEvent(PAPI_TOT_CYC);
      runner->addEvent(PAPI_FP_INS);
      runner->start();
  } else if ( type == InjectionPointType::End) {
      runner->stop();
  } else {
      runner->iter(true);
  }

  if (runner->values.size() > 0 ) {
        engine->Put( "cycles", (double) (runner->values)[0]);
        engine->Put( "fpins", (double) (runner->values)[1]);
        engine->Put( "stage", stageId);
  }

  //Reset --> This gets the change in cyles.
  runner->reset();

  return SUCCESS;
}















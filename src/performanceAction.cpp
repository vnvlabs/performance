#include "initailize.h"

using namespace VnV;




class PapiEventSet {
    bool initialized = false;
    json config;
    int EventSet = PAPI_NULL;
    std::size_t EventSetCounter = 0;
    std::map<int,int> eventSetMap;
    std::vector<long long> values;
    std::vector<std::string> validEvents;
    

  public:
    PapiEventSet() {}
    PapiEventSet(const json& config) {
      Initialize(config);
    }
    Initialize(const json &config) { 
      if (initialized) return;
      initalized = true;
      
      VnVPapi::InitailizePAPI();
      this->config = config;

      int r = PAPI_create_eventset(&EventSet);
      if (r != PAPI_OK) {
          VnV_Error("Could not create PAPI Event Set (%s)", PAPI_descr_error(r));
      }

      // Get a set of events to try and create. 
      
      for (auto it :  config["metrics"].items() ) {
          auto m = papi_event_set_map.find(it.value());
          if (m != papi_event_set_map.end()) {
            bool added = true;
            for (auto e: m->second.first) {
              if (eventSetMap.find(e) == eventSetMap.end()) {
                int r = PAPI_add_event(EventSet, e);
                if (r == PAPI_OK) {
                    eventSetMap[e] = eventSetMap.size();
                 } else {
                    //VnV_Error("Could not add event %d: %s", e, PAPI_descr_error(r));
                    added = false;
                }
              }
            }
            if (added) {
              validEvents.push_back(it.value());
            }
         }
          
      }
      values.resize(eventSetMap.size(),0);
    }

    void write(IOutputEngine *engine, std::string iter) {
      
      //Read the values      
      std::vector<long long> vals(eventSetMap.size(),0);
      PAPI_read(EventSet, &vals[0]);
      
      //Process the values. 
      engine->Put("iter",iter);
      for (auto it : validEvents) {
         papi_event_set_map[it].second(
           engine, config, eventSetMap, values, vals
         );
      }
      
      //Update the cummulative sums. 
      for (int i = 0; i < vals.size(); i++ ) {
        values[i] += vals[i];  
      }

    }

    void initialize(IOutputEngine *engine) {
        
        //Write out all the valid events 
        for (auto it : validEvents) {
          engine->Put("event_" + it,true);
        }
        
        int r = PAPI_start(EventSet);
    }

    void finalize(IOutputEngine *engine) {
        int r = PAPI_stop(EventSet, &values[0]);
    }
    

    ~PapiEventSet() {
       
       if (EventSet != PAPI_NULL ) {
          PAPI_destroy_eventset(&EventSet);
       }

    }

  };


/**
 * @title Global Application Performance Monitoring.
 * 
 * .. vnv-tag:: div
 *    :style.display: flex
 *    :style.width: 100%
 *    :style.flex-wrap: wrap
 *    
 *    
 *    .. vnv-if:: event_flops 
 *  
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *    
 *    .. vnv-if:: event_cycles
 * 
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *    
 *   
 * 
 */
INJECTION_ACTION(PNAME, flops, "{\"type\":\"object\"}") {

  class PapiAction : public VnV::IAction {

  PapiEventSet eventSet;

  public:
    PapiAction(const json &config) : eventSet(config){ 
   
      implements_injectionPointStart = true;
      implements_injectionPointIter = true;
      implements_injectionPointEnd = true;
      
    }

  
    void initialize() override {
      eventSet.initialize(getEngine());
    }

    void finalize() override {
      eventSet.finalize(getEngine());
    }
    void injectionPointStart(std::string package, std::string name) override{
      eventSet.write(getEngine(), package + ":"+name);
    }

    void injectionPointIteration(std::string stageId) override {
      eventSet.write(getEngine(), stageId); 
    }

    void injectionPointEnd() override {
        eventSet.write(getEngine(),"End");
    }

    ~PapiAction() {}

  };


  return new PapiAction(config);
}




/**
 * @title Injection Point Performance Monitoring.
 * 
 * .. vnv-tag:: div
 *    :style.display: flex
 *    :style.width: 100%
 *    :style.flex-wrap: wrap
 *    
 *    
 *    .. vnv-if:: flops 
 *  
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *    
 *    .. vnv-if:: cycles
 * 
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *    
 *    .. vnv-if:: cycles
 * 
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *
 *    .. vnv-if:: cycles
 * 
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *
 *    .. vnv-if:: cycles
 * 
 *        .. vnv-plotly::
 *                :trace.flops: scatter
 *
 * 
 */
INJECTION_TEST_R(PNAME, monitor, PapiEventSet) {
  
  if (type == InjectionPointType::Single) {
    return FAILURE;
  }

  if (type == InjectionPointType::Begin) {
    runner->Initialize(m_config.getAdditionalParameters());
    runner->initialize(engine);
  } else {
 
    runner->write(engine, stageId)
  
    if (type == InjectionPointType::End) {
      runner->finalize(engine);
    }

  }

  return SUCCESS;
}

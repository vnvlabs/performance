ARG FROM_IMAGE=vnv_base
FROM ${FROM_IMAGE}


#Install the simple application example. 
COPY . ${SOURCE_DIR}/perf

RUN  cd ${SOURCE_DIR}/perf \
     && mkdir build \
     && cd build \
     && cmake -DInjection_DIR=${VNV_DIR}/lib/cmake\
              -DCMAKE_INSTALL_PREFIX=${SOFTWARE_DIR}/plugins/perf .. \
     && make \
     && make install \ 
     && rm -rf ${SOURCE_DIR}/perf

#Register the plugin so it gets loaded automatically by VnV Each time it runs. 
RUN ${VNV_REGISTER} plugin Papi ${SOFTWARE_DIR}/plugins/perf/lib/libpapiTest.so


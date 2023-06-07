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

ENV VNV_PLUGINS = ${VNV_PLUGINS}@Papi:${SOFTWARE_DIR}/plugins/perf/lib/libpapiTest.so


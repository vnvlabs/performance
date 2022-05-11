ARG FROM_IMAGE=vnv_base
FROM ${FROM_IMAGE}:latest

ARG COMMIT_HASH=unknown
LABEL  vnv.version.simple=${COMMIT_HASH}

ENV VNV_CONFIG=${VNV_CONFIG}:${SOURCE_DIR}/perf/.vnv-config.json

#Install the simple application example. 
COPY . ${SOURCE_DIR}/perf  

RUN  cd ${SOURCE_DIR}/perf \
     && mkdir build \
     && cd build \
     && cmake -DInjection_DIR=${VNV_DIR}/lib/cmake\
              -DCMAKE_INSTALL_PREFIX=${SOFTWARE_DIR}/perf .. \
     && make \
     && make install 



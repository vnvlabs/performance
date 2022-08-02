ARG FROM_IMAGE=vnv_base
FROM ${FROM_IMAGE}:latest

ARG COMMIT_HASH=unknown
LABEL  vnv.version.simple=${COMMIT_HASH}

ENV VNV_CONFIG=${VNV_CONFIG}:${SOFTWARE_DIR}/perf/vnv.__registration__

#Install the simple application example. 
COPY . ${SOURCE_DIR}/perf  

RUN  cd ${SOURCE_DIR}/perf \
     && mkdir build \
     && cd build \
     && cmake -DInjection_DIR=${VNV_DIR}/lib/cmake\
              -DCMAKE_INSTALL_PREFIX=${SOFTWARE_DIR}/perf .. \
     && make \
     && make install \ 
     && rm -rf ${SOURCE_DIR}/perf




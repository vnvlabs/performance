
from vnv:latest
COPY . ./perf

# TODO INSTALL PAPI 

#Make the VnV Project 
#RUN cd ./papi && mkdir build && cd build && cmake .. && make && make install 
RUN git clone https://bitbucket.org/icl/papi.git && cd papi/src && sudo sysctl -w kernel.perf_event_paranoid=0 && pwd && ./configure && make && sudo make install && cd utils && ./papi_avail 

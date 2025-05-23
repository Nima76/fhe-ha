# Base image
FROM nimafrj/2204accopenfhe:1.0.3

WORKDIR /app
COPY encrypt-fintech-analytics.cpp /app/encrypt-fintech-analytics.cpp

RUN cp /usr/src/app/openfhe-uniman/CMakeLists.User.txt ./CMakeLists.txt
RUN echo "find_package(Threads REQUIRED)" >> CMakeLists.txt
RUN echo "add_executable(fhe-main encrypt-fintech-analytics.cpp)" >> CMakeLists.txt

RUN mkdir /app/build /app/results /app/data
WORKDIR /app/build
RUN cmake .. && make
RUN mv fhe-main ../
WORKDIR /app
RUN rm -rf ./build CMakeLists.txt encrypt-fintech-analytics.cpp

#RUN ./bdt-hom-eval 3

RUN chmod +x fhe-main

CMD ["bash", "-l"] 






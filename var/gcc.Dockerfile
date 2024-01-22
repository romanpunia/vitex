FROM gcc:latest
ARG CONFIGURE="-DCMAKE_BUILD_TYPE=Release"
ARG COMPILE="-j"
RUN apt update && apt upgrade
RUN apt install -y cmake \
    zlib1g-dev \
    libassimp-dev \
    libfreetype-dev \
    libsdl2-dev \
    libspirv-cross-c-shared-dev glslang-dev glslang-tools \
    libopenal-dev \
    libglew-dev \
    libssl-dev \
    libmongoc-dev libbson-dev \
    libpq-dev \
    libsqlite3-dev
RUN mkdir /home/target/ && mkdir /home/target/make
COPY ./ /home/target/intermediate
WORKDIR /home/target/intermediate
RUN cmake -S=/home/target/intermediate -B=/home/target/make -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=/usr/local/lib $CONFIGURE
RUN make -C /home/target/make $COMPILE
RUN make -C /home/target/make install
ENV PATH="${PATH}:/usr/local/lib"
ENV LD_LIBRARY_PATH="/usr/local/lib"
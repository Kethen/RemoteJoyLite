FROM ubuntu:22.04
RUN export DEBIAN_FRONTEND=noninteractive; apt update; apt install -y make wget xz-utils gcc
RUN cd /; wget https://github.com/mstorsjo/llvm-mingw/releases/download/20241217/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64.tar.xz -O - | xz -d | tar -xv
RUN cd /; wget https://github.com/pspdev/pspdev/releases/download/v20240701/pspdev-ubuntu-latest-x86_64.tar.gz -O - | gzip -d | tar -C /usr/local -x


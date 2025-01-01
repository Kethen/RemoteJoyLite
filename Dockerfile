FROM ubuntu:22.04
RUN export DEBIAN_FRONTEND=noninteractive; apt update; apt install -y gcc-mingw-w64-x86-64 gcc-mingw-w64-i686 g++-mingw-w64-x86-64 g++-mingw-w64-i686 make wget

RUN wget https://github.com/pspdev/pspdev/releases/download/v20240701/pspdev-ubuntu-latest-x86_64.tar.gz -O - | gzip -d | tar -C /usr/local -x


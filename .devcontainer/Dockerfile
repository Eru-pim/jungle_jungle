FROM ubuntu:latest

RUN echo "Cooked_Marsh" > /etc/hostname

ENV TZ=Asia/Seoul LANG=ko_KR.UTF-8 LANGUAGE=ko_KR.UTF-8

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    locales tzdata build-essential sudo gdb vim curl git wget \
    python3 cmake make gcc sudo valgrind telnet net-tools iproute2\
    && locale-gen ko_KR.UTF-8 && update-locale LANG=ko_KR.UTF-8

RUN useradd -m -s /bin/bash Eru-pim && usermod -aG sudo Eru-pim

RUN echo 'Eru-pim:1234' | chpasswd

USER Eru-pim
WORKDIR /home/Eru-pim
FROM ubuntu:20.04

ENV DPDK_VERSION=21.11.2
ENV DPDK_STUFF=dpdk-$DPDK_VERSION.tar.xz
ENV DPDK_HOME=/dpdk

RUN apt-get update -y
RUN apt-get upgrade -y
RUN apt-get install -y curl
RUN apt-get install -y linux-tools-common
RUN apt-get install -y linux-tools-generic
RUN apt-get install -y tcpdump
RUN apt-get install -y net-tools
RUN apt-get install -y ethtool
RUN apt-get install -y coreutils
RUN apt-get install -y iproute2
RUN apt-get install -y vim
RUN apt-get install -y locate
RUN apt-get install -y pciutils

RUN apt-get -qq update \
  && apt-get -qq install --no-install-recommends \
       build-essential \
       ca-certificates \
       libnuma-dev \
       python3-pip \
       python3-pyelftools \
       python3-setuptools \
       wget \
       xz-utils \
  && pip3 install \
       meson \
       ninja \
  && wget --quiet http://fast.dpdk.org/rel/$DPDK_STUFF \
  && mkdir --parents $DPDK_HOME \
  && tar --extract --file=$DPDK_STUFF --directory=$DPDK_HOME --strip-components 1 \
  && apt-get -qq clean \
  && rm --recursive --force /var/lib/apt/lists/* /tmp/* /var/tmp/* \
  && rm --force $DPDK_STUFF

RUN cd $DPDK_HOME \
  && meson build \
  && cd build \
  && ninja \
  && ninja install \
  && ldconfig




ENV RTE_SDK=$DPDK_HOME

RUN useradd -m -G sudo -p mypwd myusr
USER myusr
WORKDIR /upfdir


COPY ./upf .
COPY ./libeng.so .
COPY ./libupfndpi.so .
COPY ./libjansson.so .
COPY ./libjansson.so.4 .
COPY ./libjansson.so.4.11.1 .
COPY ./libisal.so.2 .
COPY ./libisal.so.2.0.28 .
COPY ./libfdt.so.1 .
COPY ./libnl-3.so.200 .
COPY ./libnl-3.so.200.26.0 .
COPY ./libnl-route-3.so.200 .
COPY ./libnl-route-3.so.200.26.0 .
COPY ./libibverbs.so.1 .
COPY ./libmlx4.so.1 .
COPY ./libmlx5.so.1 .
COPY ./libmlx5.so.1.12.28.0 .
COPY ./libaerospike.so .
COPY ./dpnc.json .
COPY ./eng.json .

CMD ["/bin/bash"]



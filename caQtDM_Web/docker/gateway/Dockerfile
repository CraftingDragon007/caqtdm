FROM debian:bookworm-slim AS builder

RUN apt update
RUN apt install -y \
    build-essential \
    git \
    perl \
    libreadline-dev \
    ca-certificates

WORKDIR /usr/local/epics

RUN git clone --branch R7.0.10 --depth 1 --recursive https://github.com/epics-base/epics-base.git base

WORKDIR /usr/local/epics/base
RUN make -j$(nproc)

ENV EPICS_BASE=/usr/local/epics/base
WORKDIR /usr/local/epics/modules
RUN git clone --depth 1 https://github.com/epics-modules/pcas.git pcas

WORKDIR /usr/local/epics/modules/pcas
RUN printf "EPICS_BASE=/usr/local/epics/base\n" > configure/RELEASE \
    && make -j$(nproc)

WORKDIR /usr/local/epics/extensions
RUN git clone --depth 1 --recursive https://github.com/epics-extensions/ca-gateway.git ca-gateway

WORKDIR /usr/local/epics/extensions/ca-gateway

RUN printf "PCAS=/usr/local/epics/modules/pcas\nEPICS_BASE=/usr/local/epics/base\n" > configure/RELEASE \
    && make -j$(nproc)

FROM debian:bookworm-slim

LABEL maintainer="Julian Houba" \
      version="7.0.10-gateway" \
      description="EPICS CA Gateway"

RUN apt update && apt install -y \
    libreadline8 \
    perl \
    ca-certificates \
    bash

RUN apt clean && rm -rf /var/lib/apt/lists/*


RUN groupadd -r epics && useradd -r -g epics epics
USER epics

WORKDIR /app
COPY --from=builder --chown=epics:epics /usr/local/epics /usr/local/epics

ENV EPICS_BASE=/usr/local/epics/base
ENV LD_LIBRARY_PATH=/usr/local/epics/base/lib/linux-x86_64
ENV PATH="/usr/local/epics/base/bin/linux-x86_64:/usr/local/epics/extensions/ca-gateway/bin/linux-x86_64:/usr/local/bin:${PATH}"

ENTRYPOINT ["gateway"]
CMD ["-help"]

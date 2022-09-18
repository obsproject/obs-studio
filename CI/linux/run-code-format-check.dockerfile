FROM ubuntu:22.04

RUN apt update

# Baseline infrastructure
RUN apt install -y \
    apt-transport-https \
    ca-certificates \
    curl \
    git

# Tooling required to run the checks
RUN apt install -y \
    clang-format-13 \
    pip

RUN pip install cmakelang

# BOOSTER
# https://booster.c3bi.pasteur.fr

# base image: Ubuntu
FROM ubuntu:16.04

# File Author / Maintainer
MAINTAINER Frederic Lemoine <frederic.lemoine@pasteur.fr>

COPY . /usr/local/booster

RUN apt-get update --fix-missing \
    && apt-get install -y wget gcc make libgomp1 git \
    && cd /usr/local/booster/src \
    && make \
    && cp booster /usr/local/bin \
    && cd / \
    && rm -rf /usr/local/booster \
    && apt-get remove -y wget gcc make git \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir /pasteur

ENTRYPOINT ["/usr/local/bin/booster"]

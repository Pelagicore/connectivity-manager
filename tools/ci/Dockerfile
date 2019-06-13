FROM ubuntu:18.04

ARG UID
ARG GID

# Install core dependencies
RUN apt-get update && \
    apt-get install -qq -y --option=Dpkg::Options::=--force-confnew \
        build-essential \
        gcc \
        git \
        locales \
        python \
        unzip \
        wget \
        xz-utils
RUN rm -rf /var/lib/apt/lists/*

# Set up locale
RUN /usr/sbin/locale-gen en_US.UTF-8
RUN localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.UTF8

# Replace dash with bash
RUN ln -sf /bin/bash /bin/sh

RUN useradd -U -m user
RUN usermod --uid $UID user
RUN groupmod --gid $GID user

USER user
WORKDIR /home/user/
CMD /bin/bash

FROM ubuntu:18.04

ARG UID
ARG GID

# Install dependencies
RUN apt-get update && \
    apt-get install -qq -y --option=Dpkg::Options::=--force-confnew \
        build-essential \
        clang-format-8 \
        clang-tidy-8 \
        cmake \
        gcc \
        git \
        libglibmm-2.4-dev \
        locales \
        meson \
        python \
        python3 \
        python3-jinja2 \
        python3-pip \
        systemd \
        unzip \
        wget \
        xz-utils
RUN rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-8 1000 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-8 1000 && \
    update-alternatives --install /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-8 1000

# Install googletest package with apt-get above instead when upgrading to Ubuntu >= 19.04.
RUN tag=release-1.8.1 dir=googletest-$tag && \
    wget -O $dir.zip https://github.com/google/googletest/archive/$tag.zip && \
    unzip $dir.zip && \
    mkdir $dir/build && \
    (cd $dir/build; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..) && \
    make -C $dir/build install

# Change hash to tag when 3.0 has been released.
RUN pip3 install https://github.com/Pelagicore/gdbus-codegen-glibmm/archive/41940e0d5d5cacf49c2aa493912664d87cf27200.zip

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

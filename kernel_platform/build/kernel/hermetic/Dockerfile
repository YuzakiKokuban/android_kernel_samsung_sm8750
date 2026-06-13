FROM debian:stable-slim

RUN apt-get -y update                            \
&&  apt-get -y install                           \
# Those are likely essential
    git                                          \
# To be removed
    rsync                                        \
&& apt-get clean                                 \
&& rm -rf /var/lib/apt/lists/*

# Explicitly allow some host binaries for now
RUN mkdir -p /l/bin
RUN ln -s $(which perl) /l/bin/
RUN ln -s $(which rsync) /l/bin/
RUN ln -s $(which sh) /l/bin/

# Allow git usage during the build (e.g. to determine timestamps)
RUN ln -s $(which git) /l/bin/

# Copy the build runner
COPY build /l/build
RUN chmod a+rx /l/build

# Satisfy some bazel requirements to locate a writable cache directory
RUN chmod 777 /home
ENV USER=build
ENV HOME=/home

# Run the build runner if no other command is given
CMD /l/build


FROM alpine:latest

# Setup environment variables
ENV BENTO4_VERSION 1.5.1-629
ENV BENTO4_INSTALL_DIR=/opt/bento4
ENV PATH=/opt/bento4/bin:${PATH}

# Install dependencies
RUN apk update && \
    apk add --no-cache ca-certificates bash python3 make cmake gcc g++ git

# Fetch source
RUN mkdir -p /opt/bento4
COPY ./ /opt/bento4/dist

# Build
RUN rm -rf /opt/bento4/dist/cmakebuild && mkdir -p /opt/bento4/dist/cmakebuild && cd /opt/bento4/dist/cmakebuild && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Install
RUN cd /opt/bento4/dist && python3 Scripts/SdkPackager.py

# Cleanup.
RUN rm -rf /var/cache/apk/* /tmp/*

WORKDIR /opt/bento4

CMD ["bash"]
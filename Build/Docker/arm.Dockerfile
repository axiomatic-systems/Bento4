FROM arm64v8/alpine:latest

# Setup environment variables
ENV BENTO4_VERSION 1.6.0-639

# Install Dependencies
RUN apk update && apk add --no-cache ca-certificates bash python3 make cmake gcc g++ git

# Copy Sources
COPY ./ /tmp/bento4

# Build
RUN rm -rf /tmp/bento4/cmakebuild && mkdir -p /tmp/bento4/cmakebuild/arm64-unknown-linux && cd /tmp/bento4/cmakebuild/arm64-unknown-linux && cmake -DCMAKE_BUILD_TYPE=Release ../.. && make -j$(nproc)

# Install
RUN cd /tmp/bento4 && python3 Scripts/SdkPackager.py arm64-unknown-linux . cmake && mkdir /opt/bento4 && mv /tmp/bento4/SDK/Bento4-SDK-*.arm64-unknown-linux/* /opt/bento4

# === Second Stage ===
FROM arm64v8/alpine:latest
ARG BENTO4_VERSION
LABEL "com.example.vendor"="Axiomatic Systems, LLC."
LABEL version=$BENTO4_VERSION
LABEL maintainer="bok@bok.net"

# Setup environment variables
ENV PATH=/opt/bento4/bin:${PATH}

# Install Dependencies
RUN apk --no-cache add ca-certificates bash python3 libstdc++

# Copy Binaries
COPY --from=0 /opt/bento4 /opt/bento4

WORKDIR /opt/bento4

CMD ["bash"]

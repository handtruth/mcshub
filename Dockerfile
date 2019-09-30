FROM alpine:latest AS tools
RUN apk --no-cache --no-progress add build-base ninja coreutils cmake git python3 \
    && python3 -m pip install meson

FROM tools AS build
ADD . /mcshub
WORKDIR /mcshub
RUN meson -Dprefix=`pwd`/out -Dbuildtype=release -Doptimization=3 build \
    && cd build && ninja && ninja install

FROM alpine:latest
COPY --from=build /mcshub/out/bin/mcshub /usr/local/bin/mcshub
COPY --from=build /mcshub/out/bin/mcping /usr/local/bin/mcping
RUN [ "apk", "--no-cache", "--no-progress", "add", "libstdc++" ]
VOLUME /mcshub
WORKDIR /mcshub
HEALTHCHECK [ "mcping", "localhost" ]
EXPOSE 25565/tcp
LABEL maintainer="ktlo <ktlo@handtruth.com>"
ENTRYPOINT [ "mcshub" ]

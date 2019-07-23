FROM alpine:latest AS tools
RUN apk --no-cache --no-progress add build-base vim coreutils cmake bash

FROM tools AS build
ADD . /mcshub
WORKDIR /mcshub
RUN ./configure && make

FROM alpine:latest
COPY --from=build /mcshub/out/mcshub /usr/local/bin/mcshub
RUN mkdir /mcshub && apk --no-cache --no-progress add libstdc++
WORKDIR /mcshub
VOLUME /mcshub
EXPOSE 25565/tcp
LABEL maintainer="ktlo@handtruth.com"
ENTRYPOINT [ "mcshub" ]

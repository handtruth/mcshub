FROM alpine:latest AS tools
RUN apk add build-base vim coreutils cmake bash git

FROM tools AS build
RUN git clone https://github.com/handtruth/mcshub.git
WORKDIR /mcshub
RUN ./configure && make

FROM alpine:latest
COPY --from=build /mcshub/out/mcshub /usr/local/bin/mcshub
RUN mkdir /mcshub && apk add libstdc++ && cd /mcshub && mcshub --install
WORKDIR /mcshub
EXPOSE 25565/tcp
ENTRYPOINT [ "mcshub" ]

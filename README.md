Minecraft Server Hub
=======================

![MCSHub Logo](other/MCSHub.png)

Description
-----------------------

This is a proxy server that acts like a Minecraft server by it's
newest protocol. It allows you to keep several Minecraft servers
on the same TCP address.

How it works?
-----------------------

Each Minecraft Java Edition client sends the server name that
player writes in address field. MCSHub search a record inside
it's configuration which address field equals client's address.
After that MCSHub connects the client to this server. There is
also a default record that is used when no record is found.

You need different DNS names for the actual IP address for this
to work.

How to use it?
-----------------------

Firstly, you need to install MCSHub. I recommend you to use
docker image. If you have Docker, you can run it like this.
```
docker run -d -v /srv/mcshub:/mcshub:rw -p 25565:25565 \
  --name mcshub handtruth/mcshub
```

On Wiki that is connected to this repository on GitHub you will
find an information about mcshub configuration.

If you look at `/srv/mcshub` directory, you will find
`mcshub.yml` file. There are many options shortly described.

How to build it?
-----------------------

### 1. Build from Source

This software works only with Linux OS. If you want to build it,
you need python3, ninja, bash, wget, cmake and g++ to be
installed first. [Meson](https://mesonbuild.com/index.html)
build system also needs to be installed.

If you using Debian like OS, you can install everything you
need, with *apt* like this:
```sh
apt install -y cmake g++ python3 python3-pip \
  python3-setuptools python3-wheel ninja-build cppcheck \
  && python3 -m pip install --user meson
```

After that you can build MCSHub:
```
meson build && cd build \
  && ninja -Dbuildtype=release -Doptimization=3 -Dsystemd=true
```

If you want to, you can install MCSHub into your system like so.
```
ninja install
```

After installation you can find mcshub configuration at 
`/etc/mcshub` directory. MCSHub is controlled by systemd from 
now on as *mcshub.service*.

Execute this command to remove MCSHub from your system.
```
ninja uninstall
```

### 2. Build Docker Image

Execute the next command in mcshub source directory to build
it as docker image.
```
docker build -t mcshub .
```

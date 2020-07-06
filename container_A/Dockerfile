FROM <image>:<tag>

# install gcc

RUN mkdir /usr/src

RUN mkdir /usr/src/app

COPY relay.h /usr/src/app
COPY first.c /usr/src/app
COPY script.sh /usr/src/app

WORKDIR /usr/src/app

RUN gcc -o first first.c

RUN chmod +x first
RUN chmod +x script.sh

CMD ["./script.sh"]

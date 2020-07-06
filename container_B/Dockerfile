FROM <image>:<tag>

# install gcc

RUN mkdir /usr/src/app

COPY relay.h /usr/src/app
COPY next-wait.c /usr/src/app
COPY next-pass.c /usr/src/app

WORKDIR /usr/src/app

RUN gcc -o next-wait next-wait.c
RUN gcc -o next-pass next-pass.c

RUN chmod +x ./next-wait
RUN chmod +x ./next-pass

CMD ["./script.sh"]

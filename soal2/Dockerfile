FROM ubuntu:latest

WORKDIR /app

COPY . /app

RUN apt update && \
    apt install -y gcc make

EXPOSE 9000

CMD ["./server"]

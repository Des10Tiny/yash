FROM ubuntu:24.04 AS builder

RUN apt-get update && \
    apt-get install -y build-essential cmake && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release  && \ 
    cmake --build build --config Release --parallel   

FROM ubuntu:24.04
WORKDIR /app

COPY --from=builder /src/build/src/yash .

RUN chmod +x ./yash

ENTRYPOINT ["./yash"]
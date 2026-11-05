
FROM gcc:13

WORKDIR /app

COPY . .

RUN gcc -O2 -o server server.c tinyurl.c mongoose.c

EXPOSE 10000

CMD ["./server"]

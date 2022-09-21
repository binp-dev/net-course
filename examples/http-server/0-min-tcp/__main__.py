import asyncio
from asyncio import StreamReader, StreamWriter


async def process(reader: StreamReader, writer: StreamWriter) -> None:
    addr = writer.get_extra_info("peername")
    print(f"Requested from {addr}")

    payload = "Hello World!"

    writer.write(
        "\n".join(
            [
                f"HTTP/1.1 200 OK",
                f"Content-Length: {len(payload)}",
                f"Content-Type: text/plain; charset=utf-8",
                f"",
                payload,
                f"",
            ]
        ).encode("utf-8")
    )
    await writer.drain()
    writer.close()


async def main() -> None:
    server = await asyncio.start_server(process, "127.0.0.1", 8080)

    addrs = ", ".join(str(sock.getsockname()) for sock in server.sockets)
    print(f"Listening on {addrs}")

    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())

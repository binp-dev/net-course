import asyncio
from asyncio import StreamReader, StreamWriter


async def process(reader: StreamReader, writer: StreamWriter) -> None:
    addr = writer.get_extra_info("peername")
    print(f"Accepted: {addr}")

    while True:
        data = await reader.read(256)
        if len(data) == 0:
            break

        writer.write(data)
        await writer.drain()

    print(f"Closed: {addr}")
    writer.close()


async def main() -> None:
    server = await asyncio.start_server(process, "127.0.0.1", 8080)

    addrs = ", ".join(str(sock.getsockname()) for sock in server.sockets)
    print(f"Listening on {addrs}")

    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())

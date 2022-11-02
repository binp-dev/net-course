import asyncio
import asyncudp


async def main():
    sock = await asyncudp.create_socket(local_addr=("0.0.0.0", 9999))
    try:
        while True:
            data, src = await sock.recvfrom()
            print(f"{src}: {data}")
            sock.sendto(data, addr=src)
    finally:
        sock.close()


asyncio.run(main())

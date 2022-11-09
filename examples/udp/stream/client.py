from __future__ import annotations

import asyncio
from asyncudp import create_socket
from dataclasses import dataclass

from common import DataMsg, ReqMsg

chunk_size = 8


@dataclass
class Entry:
    msg: DataMsg
    sent: bool = False


async def main(data: bytes):
    print(f"Sending: {data}")

    server_addr = ("192.168.1.2", 9999)  # put your address here
    sock = await create_socket(local_addr=("0.0.0.0", 8888))

    chunks = [data[i : (i + chunk_size)] for i in range(0, len(data), chunk_size)]
    chunks += [b""]  # empty chunk means stream end

    messages = [Entry(DataMsg(i, c)) for i, c in enumerate(chunks)]

    try:
        while True:
            for e in messages:
                if not e.sent:
                    sock.sendto(e.msg.store(), addr=server_addr)
                    print(f"Sent: {e.msg}")
                    e.sent = True

            data, src = await sock.recvfrom()
            assert src == server_addr
            req = ReqMsg.load(data)
            print(f"Recv: {req}")
            if len(req.indices) == 0:
                # empty request means server got all data
                break
            for i in req.indices:
                assert i < len(messages)
                messages[i].sent = False

    finally:
        sock.close()

    print("Done")


print("Enter data")
data = input().encode()

asyncio.run(main(data))

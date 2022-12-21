from __future__ import annotations

import asyncio
from asyncudp import create_socket
from dataclasses import dataclass

from common import DataMsg, ReqMsg

req_len = 5
timeout = 1.0
retries = 4


async def main():
    server_addr = ("0.0.0.0", 9999)
    sock = await create_socket(local_addr=server_addr)
    client_addr = None

    chunks: list[bytes | None] = []

    print(f"Server listening on {server_addr}")
    try:
        while True:
            try:
                data, src = await asyncio.wait_for(sock.recvfrom(), timeout=timeout)
            except asyncio.TimeoutError:
                if client_addr is None:
                    continue
                print(chunks)
                # send missing chunks requests
                req = ReqMsg([])
                for i, c in enumerate(chunks):
                    if c is None:
                        req.indices.append(i)
                        if len(req.indices) >= req_len:
                            print(f"Send: {req}")
                            sock.sendto(req.store(), client_addr)
                            req = ReqMsg([])
                if len(chunks) == 0 or chunks[-1] != b"":
                    req.indices.append(len(chunks))
                if len(req.indices) > 0:
                    print(f"Send: {req}")
                    sock.sendto(req.store(), client_addr)
                continue

            assert client_addr is None or client_addr == src
            client_addr = src
            msg = DataMsg.load(data)
            print(f"Recv: {msg}")

            ext = msg.index - len(chunks) + 1
            if ext > 0:
                chunks.extend([None] * ext)
            chunks[msg.index] = msg.payload

            if chunks[-1] == b"":
                if all([c is not None for c in chunks]):
                    # notify client about stream end
                    req = ReqMsg([])
                    print(f"Send: {req}")
                    for _ in range(retries):
                        sock.sendto(req.store(), client_addr)
                    break

    finally:
        sock.close()

    print("Whole data got:", b"".join(chunks))


asyncio.run(main())

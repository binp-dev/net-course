import asyncio
from asyncudp import create_socket
from random import Random
from copy import copy

seed = 0xDEADBEEF
drop_p = 0.1
repeat_p = 0.1
repeat_n = 4
postpone_s = 1.0


def poisson(rng: Random, p: float, n: int) -> int:
    return sum([rng.uniform(0.0, 1.0) < p for _ in range(n)])


async def main():
    rng = Random(seed)

    dst = ("192.168.1.2", 7777)  # put your address here
    sock = await create_socket(local_addr=("0.0.0.0", 9999))
    try:
        while True:
            data, src = await sock.recvfrom()
            print(f"{src}: {data}")

            # drop
            if rng.uniform(0.0, 1.0) > drop_p:
                # repeat
                for _ in range(poisson(rng, repeat_p, repeat_n - 1) + 1):
                    # postpone
                    async def send_later(data: bytes, delay: float):
                        await asyncio.sleep(delay)
                        sock.sendto(data, addr=dst)

                    asyncio.create_task(
                        send_later(copy(data), rng.expovariate(1.0 / postpone_s)),
                    )
    finally:
        sock.close()


asyncio.run(main())

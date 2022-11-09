from __future__ import annotations

from dataclasses import dataclass

magic = 0xAB
index_size = 2


@dataclass
class DataMsg:
    index: int
    payload: bytes  # empty payload means stream end

    def store(self) -> bytes:
        return (
            magic.to_bytes(1, "big")
            + self.index.to_bytes(index_size, "big")
            + self.payload
        )

    def load(data: bytes) -> DataMsg:
        assert magic == data[0]
        data = data[1:]
        return DataMsg(
            int.from_bytes(data[0:index_size], "big"),
            data[index_size:],
        )


@dataclass
class ReqMsg:
    indices: list[int]

    def store(self) -> bytes:
        return magic.to_bytes(1, "big") + b"".join(
            [i.to_bytes(index_size, "big") for i in self.indices]
        )

    def load(data: bytes) -> ReqMsg:
        assert magic == data[0]
        data = data[1:]
        return ReqMsg(
            [
                int.from_bytes(data[i : (i + index_size)], "big")
                for i in range(0, len(data), index_size)
            ],
        )

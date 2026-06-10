from abc import abstractmethod
from typing import Protocol

class Python(Protocol):
    @abstractmethod
    def print(self, s: str) -> None:
        raise NotImplementedError


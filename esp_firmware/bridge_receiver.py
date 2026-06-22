#!/usr/bin/env python3
import argparse
import socket
import struct
import time


FRAME_LEN = 512
HEADER_LEN = 12
MAX_PAYLOAD_LEN = FRAME_LEN - HEADER_LEN
FRAME_MAGIC = 0xA5
FRAME_TYPE_DATA = 0x02
HEADER = struct.Struct("<BBIHBxxx")


def checksum8(data: bytes) -> int:
    return sum(data) & 0xFF


def pattern_byte(seq: int, index: int) -> int:
    return (seq * 17 + index * 31) & 0xFF


def validate_frame(data: bytes) -> tuple[bool, int]:
    if len(data) != FRAME_LEN:
        return False, 0
    magic, frame_type, seq, payload_len, header_sum = HEADER.unpack_from(data)
    if magic != FRAME_MAGIC or frame_type != FRAME_TYPE_DATA:
        return False, seq
    if payload_len > MAX_PAYLOAD_LEN:
        return False, seq
    if header_sum != checksum8(data[:8]):
        return False, seq
    payload = data[HEADER_LEN:HEADER_LEN + payload_len]
    for index, value in enumerate(payload):
        if value != pattern_byte(seq, index):
            return False, seq
    return True, seq


def sequence_delta(seq: int, expected_seq: int) -> tuple[int, int]:
    diff = (seq - expected_seq) & 0xFFFFFFFF
    if diff == 0:
        return 0, 0
    if diff < 0x80000000:
        return diff, 0
    return 0, 1


def main() -> None:
    parser = argparse.ArgumentParser(description="Receive Pico->SPI->ESP->UDP bridge frames.")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=5005)
    parser.add_argument("--report", type=float, default=1.0)
    parser.add_argument("--recv-size", type=int, default=2048)
    parser.add_argument("--no-register", action="store_true")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))

    first_addr = None
    expected_seq = None
    total_bytes = 0
    total_start = time.time()
    interval_start = total_start
    interval_bytes = 0
    interval_packets = 0
    interval_frames = 0
    interval_lost = 0
    interval_reordered = 0
    interval_bad = 0

    print(f"listening on {args.host}:{args.port}")

    while True:
        data, addr = sock.recvfrom(args.recv_size)
        now = time.time()
        if first_addr is None:
            first_addr = addr
            print(f"first packet from {addr[0]}:{addr[1]}")
            if not args.no_register:
                sock.sendto(b"PSLAB_UDP_REGISTER", first_addr)

        total_bytes += len(data)
        interval_bytes += len(data)
        interval_packets += 1

        if len(data) % FRAME_LEN != 0:
            interval_bad += 1
        else:
            for offset in range(0, len(data), FRAME_LEN):
                valid, seq = validate_frame(data[offset:offset + FRAME_LEN])
                interval_frames += 1
                if not valid:
                    interval_bad += 1
                    continue
                if expected_seq is not None and seq != expected_seq:
                    lost, reordered = sequence_delta(seq, expected_seq)
                    interval_lost += lost
                    interval_reordered += reordered
                expected_seq = (seq + 1) & 0xFFFFFFFF

        elapsed = now - interval_start
        if elapsed >= args.report:
            total_elapsed = now - total_start
            kbyte_s = interval_bytes / elapsed / 1000.0
            mbit_s = interval_bytes * 8.0 / elapsed / 1_000_000.0
            avg_kbyte_s = total_bytes / total_elapsed / 1000.0
            print(
                f"rate={kbyte_s:9.1f} kB/s {mbit_s:7.3f} Mbit/s | "
                f"pkts={interval_packets:6d} frames={interval_frames:6d} "
                f"lost={interval_lost:5d} reorder={interval_reordered:4d} bad={interval_bad:4d} | "
                f"avg={avg_kbyte_s:9.1f} kB/s total={total_bytes}"
            )
            if first_addr is not None and not args.no_register:
                sock.sendto(b"PSLAB_UDP_REGISTER", first_addr)
            interval_start = now
            interval_bytes = 0
            interval_packets = 0
            interval_frames = 0
            interval_lost = 0
            interval_reordered = 0
            interval_bad = 0


if __name__ == "__main__":
    main()

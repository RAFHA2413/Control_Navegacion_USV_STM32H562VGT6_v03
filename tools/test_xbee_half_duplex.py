#!/usr/bin/env python3
"""Maestro de ensayo XBee AT. Tramas completas y CRC, sin BER/RSSI/SNR inventados."""
import argparse
import csv
import time

COMMAND = b"$PUSVU,0,0,0,0,1\r\n"  # Comando seguro de parada

def crc8(data):
    value = 0
    for octet in data:
        value ^= octet
        for _ in range(8):
            value = ((value << 1) ^ (7 if value & 0x80 else 0)) & 255
    return value

def verify(frame):
    if not frame.startswith(b"$PUSVD,") or not frame.endswith(b"\r\n"):
        return False, "cabecera o CRLF"
    data = frame[:-2]
    if data.count(b"*") != 1:
        return False, "marcador CRC"
    body, hex_crc = data.rsplit(b"*", 1)
    if len(hex_crc) != 2:
        return False, "largo CRC"
    try:
        received = int(hex_crc, 16)
        body.decode("ascii")
    except (ValueError, UnicodeDecodeError):
        return False, "hexadecimal/ASCII"
    if len(body.split(b",")) != 31:
        return False, "31 campos requeridos"
    if crc8(body[1:]) != received:
        return False, "CRC8"
    return True, "OK"

def lines(buffer):
    result = []
    while b"\n" in buffer:
        idx = buffer.index(10)
        item = bytes(buffer[:idx + 1])
        del buffer[:idx + 1]
        start = item.find(b"$")
        if start >= 0:
            result.append(item[start:])
    if len(buffer) > 1024:
        buffer.clear()
    return result

def selftest():
    data = (b"$PUSVD,102831.00,1024.7404,N,07531.9268,W,1,08,1.2,5.0,M,"
            b"12.5,N,185.4,T,185.0,T,+1.2,P,-0.5,R,24.5,C,1,12.4,V,2.3,C,1,STATE,1")
    assert crc8(data[1:]) == 0xF8
    assert len(data + b"*F8\r\n") == 130
    assert verify(data + b"*F8\r\n")[0]
    assert not verify(data + b"*4F\r\n")[0]
    b = bytearray(data[:25])
    assert lines(b) == []
    b.extend(data[25:] + b"*F8\r\n")
    assert lines(b) == [data + b"*F8\r\n"]
    print("Autopruebas: PASS CRC8, 130 bytes, fragmentacion")

def run(args):
    try:
        import serial
    except ImportError:
        raise SystemExit("Instalar dependencia: python -m pip install pyserial")
    rows = []
    with serial.Serial(args.port, args.baud, timeout=0.05) as link:
        for number in range(1, args.cycles + 1):
            started = time.monotonic()
            link.reset_input_buffer()
            link.write(COMMAND)
            link.flush()
            buffer = bytearray()
            state, error, rtt = "TIMEOUT", "", None
            while time.monotonic() - started < args.timeout:
                buffer.extend(link.read(max(1, link.in_waiting)))
                for frame in lines(buffer):
                    if not frame.startswith(b"$PUSVD"):
                        continue
                    ok, why = verify(frame)
                    if ok:
                        state = "OK"
                        rtt = round(1000 * (time.monotonic() - started), 2)
                        break
                    state, error = "INVALIDA", why
                if state == "OK":
                    break
            rows.append({"ciclo":number,"estado":state,"RTT_ms":rtt,"error":error})
            print(f"{number:03d}: {state} RTT={rtt} {error}")
            rest = args.interval - (time.monotonic() - started)
            if rest > 0:
                time.sleep(rest)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["ciclo","estado","RTT_ms","error"])
        w.writeheader()
        w.writerows(rows)
    print(f"CRC validas: {sum(r['estado']=='OK' for r in rows)}/{args.cycles}")
    print("Sin numero de secuencia, el RTT solo es indicativo; no equivale a BER.")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--port", default="COM4")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--cycles", type=int, default=120)
    p.add_argument("--interval", type=float, default=0.4)
    p.add_argument("--timeout", type=float, default=2.0)
    p.add_argument("--output", default="xbee_half_duplex.csv")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    selftest() if args.self_test else run(args)

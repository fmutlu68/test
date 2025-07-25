import socket

PICO_IP = "192.168.1.100"
PICO_PORT = 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2.0)

print(f"Bağlantı kuruluyor: {PICO_IP}:{PICO_PORT}")
try:
    while True:
        msg = input("Komut (LED_ON, LED_OFF, PING, STATUS): ").strip()
        if not msg:
            continue

        sock.sendto(msg.encode(), (PICO_IP, PICO_PORT))

        try:
            data, addr = sock.recvfrom(1024)
            print(f"[Cevap @{addr[0]}:{addr[1]}] {data.decode()}")
        except socket.timeout:
            print("Zaman aşımı: Cevap alınamadı.")
except KeyboardInterrupt:
    print("Çıkılıyor...")

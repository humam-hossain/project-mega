import socket

HOST = '192.168.6.100'  # Replace with the ESP8266's IP address
PORT = 333  # Port number of the TCP server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b'Hello, ESP8266!')
    data = s.recv(1024)

print('Received', repr(data))

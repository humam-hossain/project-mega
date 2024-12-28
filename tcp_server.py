import socket

# Server configuration
SERVER_IP = ""  # Listen on all available network interfaces
SERVER_PORT = 8080  # Port to listen on

def start_server():
    try:
        # Create a socket object
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind the socket to the server IP and port
        server_socket.bind((SERVER_IP, SERVER_PORT))
        print(f"[INFO] Server started, listening on {SERVER_IP}:{SERVER_PORT}")

        # Start listening for incoming connections
        server_socket.listen(1)  # Allow 1 simultaneous connection
        print("[INFO] Waiting for incoming connections...")

        # Accept a client connection
        client_socket, client_address = server_socket.accept()
        print(f"[INFO] Connection established with {client_address}")

        # Receive data from the client
        data = client_socket.recv(1024).decode()
        print(f"[INFO] Received message from client: {data}")

        # Send a response to the client
        response = "Hello from Python TCP Server!"
        print(f"[INFO] Sending response: {response}")
        client_socket.sendall(response.encode())

        # Close the client connection
        client_socket.close()
        print("[INFO] Client connection closed.")

    except Exception as e:
        print(f"[ERROR] An error occurred: {e}")

    finally:
        # Ensure the server socket is closed
        server_socket.close()
        print("[INFO] Server shut down.")

if __name__ == "__main__":
    start_server()

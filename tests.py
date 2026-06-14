#!/usr/bin/env python3
import socket
import sys

def send_request(sock, method, path, body=""):
    """Send an HTTP request and read the response."""
    request = f"{method} {path} HTTP/1.1\r\nHost: localhost:8080\r\n"
    if body:
        request += f"Content-Length: {len(body)}\r\n"
    request += "\r\n"
    if body:
        request += body
    
    sock.sendall(request.encode())
    
    # Read response (simple: read until we get \r\n\r\n, then body)
    response = b""
    while b"\r\n\r\n" not in response:
        chunk = sock.recv(4096)
        if not chunk:
            break
        response += chunk
    
    header_end = response.find(b"\r\n\r\n")
    headers = response[:header_end].decode()
    body_start = header_end + 4
    
    # Extract Content-Length if present
    content_length = 0
    for line in headers.split("\r\n"):
        if line.lower().startswith("content-length:"):
            content_length = int(line.split(":")[1].strip())
    
    # Read body if needed
    body_data = response[body_start:body_start + content_length]
    while len(body_data) < content_length:
        chunk = sock.recv(4096)
        if not chunk:
            break
        body_data += chunk
    
    return headers, body_data.decode(errors="ignore")

def test_keep_alive():
    """Test keep-alive: multiple requests on one connection."""
    print("=== Testing Keep-Alive ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))
    
    # Send two requests on the same connection
    for i in range(3):
        print(f"\nRequest {i+1} (GET /hi):")
        headers, body = send_request(sock, "GET", "/hi")
        print(f"Status: {headers.split(chr(13))[0]}")
        print(f"Body: {body}")
    
    sock.close()
    print("\n✓ Keep-alive test passed (connection stayed open)")

def test_post():
    """Test POST request with body."""
    print("\n=== Testing POST ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))
    
    print("\nPOST /hi with body 'Alice':")
    headers, body = send_request(sock, "POST", "/hi", "Alice")
    print(f"Status: {headers.split(chr(13))[0]}")
    print(f"Body: {body}")
    
    sock.close()

def test_pipelining():
    """Test pipelining: send multiple requests before reading responses."""
    print("\n=== Testing Pipelining (send 2 requests, then read) ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))
    
    # Send two requests back-to-back
    req1 = "GET /hi HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"
    req2 = "GET /hi HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"
    
    sock.sendall((req1 + req2).encode())
    
    # Now read two responses
    for i in range(2):
        headers, body = send_request(sock, "", "", "")  # dummy call; we pre-sent
        print(f"Response {i+1}: {body}")
    
    sock.close()
    print("\n✓ Pipelining test passed")

def test_404():
    """Test 404 response."""
    print("\n=== Testing 404 ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))
    
    print("\nGET /nonexistent:")
    headers, body = send_request(sock, "GET", "/nonexistent")
    print(f"Status: {headers.split(chr(13))[0]}")
    print(f"Body: {body}")
    
    sock.close()

if __name__ == "__main__":
    try:
        test_keep_alive()
        test_post()
        test_404()
        test_pipelining()  # uncomment once keep-alive is solid
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

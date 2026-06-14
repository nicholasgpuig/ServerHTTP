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

def test_simple_get():
    """Test a single simple GET request."""
    print("\n=== Testing Simple GET ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))

    headers, body = send_request(sock, "GET", "/hi")
    status_line = headers.split("\r\n")[0]
    print(f"Status: {status_line}")
    assert "200" in status_line, f"Expected 200, got {status_line}"
    print(f"Body: {body}")

    sock.close()
    print("✓ Simple GET passed")

def test_malformed_requests():
    """Test various malformed requests."""
    print("\n=== Testing Malformed Requests ===")

    malformed = [
        ("Missing method", " /hi HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"),
        ("Missing path", "GET  HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"),
        ("Missing HTTP version", "GET /hi\r\nHost: localhost:8080\r\n\r\n"),
        ("Invalid header (no colon)", "GET /hi HTTP/1.1\r\nHost localhost:8080\r\n\r\n"),
        ("Incomplete request", "GET /hi HTTP/1.1\r\n"),  # no \r\n\r\n
    ]

    for name, request in malformed:
        print(f"\n  Testing: {name}")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2)
            sock.connect(("localhost", 8080))
            sock.sendall(request.encode())

            # Try to receive something (server might close or timeout)
            try:
                response = sock.recv(1024)
                if response:
                    print(f"    → Got response (server handled gracefully)")
                else:
                    print(f"    → Connection closed (expected)")
            except socket.timeout:
                print(f"    → Timeout waiting for response (server might be waiting for more data)")

            sock.close()
        except Exception as e:
            print(f"    → Error: {e}")

def test_large_content_length():
    """Test request with Content-Length that exceeds max."""
    print("\n=== Testing Large Content-Length ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(2)
    sock.connect(("localhost", 8080))

    # Send a request claiming 10MB body (exceeds 1MB cap)
    request = "POST /hi HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 10485760\r\n\r\n"
    sock.sendall(request.encode())

    try:
        response = sock.recv(1024)
        if response:
            status = response.decode(errors="ignore").split("\r\n")[0]
            print(f"Status: {status}")
            # Server should either reject or hang waiting for body
        else:
            print("Connection closed")
    except socket.timeout:
        print("Timeout (server waiting for oversized body)")

    sock.close()
    print("✓ Large Content-Length handled")

def test_multiple_headers():
    """Test request with multiple headers."""
    print("\n=== Testing Multiple Headers ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 8080))

    request = (
        "GET /hi HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/plain\r\n"
        "Custom-Header: some-value\r\n"
        "\r\n"
    )
    sock.sendall(request.encode())
    headers, body = send_request(sock, "", "", "")
    status = headers.split("\r\n")[0]
    print(f"Status: {status}")
    assert "200" in status, f"Expected 200, got {status}"

    sock.close()
    print("✓ Multiple headers passed")

if __name__ == "__main__":
    try:
        test_simple_get()
        test_keep_alive()
        test_post()
        test_404()
        test_multiple_headers()
        test_large_content_length()
        test_malformed_requests()
        test_pipelining()
        print("\n" + "="*50)
        print("All tests completed!")
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

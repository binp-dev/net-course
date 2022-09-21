from http.server import HTTPServer, BaseHTTPRequestHandler


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

        self.wfile.write("Hello World!".encode("utf-8"))
        self.wfile.flush()


def main() -> None:
    addr = ("localhost", 8080)
    httpd = HTTPServer(addr, Handler)
    print(f"Listening on {addr}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()

from http.server import HTTPServer, BaseHTTPRequestHandler


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

        self.wfile.write("<h1>Hello, Docker!</h1>".encode("utf-8"))
        self.wfile.flush()


addr = ("0.0.0.0", 80)
httpd = HTTPServer(addr, Handler)
print(f"Listening on {addr}")
httpd.serve_forever()

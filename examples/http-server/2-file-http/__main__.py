from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import mimetypes


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = Path(self.path[1:])

        if path.is_dir():
            path /= "index.html"

        if not path.exists():
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            self.wfile.flush()
            return

        mime_type = mimetypes.guess_type(path)[0]

        self.send_response(200)
        self.send_header("Content-type", mime_type)
        self.end_headers()

        with open(path, "rb") as f:
            self.wfile.write(f.read())

        self.wfile.flush()


def main() -> None:
    addr = ("localhost", 8080)
    httpd = HTTPServer(addr, Handler)
    print(f"Listening on {addr}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()

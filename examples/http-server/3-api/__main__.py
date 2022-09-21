from http.server import HTTPServer, SimpleHTTPRequestHandler
import json
from random import randrange


class Handler(SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path.strip("/") != "random":
            self.send_response(404)
            self.end_headers()

        try:
            size = int(self.headers["Content-Length"])
            request = json.loads(self.rfile.read(size).decode("utf-8"))

            number = randrange(int(request["begin"]), int(request["end"]))

            response = json.dumps({"number": number}).encode("utf-8")

            self.send_response(200)
            self.end_headers()
            self.wfile.write(response)
            self.wfile.flush()

        except Exception as e:
            print(e)
            self.send_response(500)
            self.end_headers()


def main() -> None:
    addr = ("localhost", 8080)
    httpd = HTTPServer(addr, Handler)
    print(f"Listening on {addr}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()

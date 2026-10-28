from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import time

IMAGE = Path(
    "/home/arduino/solipsis-edge-ai/camera-tests/"
    "retinaface_latest.jpg"
)

class Handler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == "/":
            page = """
            <!DOCTYPE html>
            <html>
            <head>
                <meta charset="utf-8">
                <title>RetinaFace Edge AI</title>
                <style>
                    body {
                        background: #101820;
                        color: white;
                        font-family: Arial, sans-serif;
                        text-align: center;
                        padding: 25px;
                    }
                    img {
                        width: 100%;
                        max-width: 960px;
                        border-radius: 12px;
                    }
                </style>
            </head>
            <body>
                <h1>RetinaFace - Arduino UNO Q</h1>
                <p>Logitech C920 | ncnn Vulkan | Adreno 702</p>
                <img src="/stream">
            </body>
            </html>
            """
            data = page.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        elif self.path == "/stream":
            self.send_response(200)
            self.send_header(
                "Content-Type",
                "multipart/x-mixed-replace; boundary=frame"
            )
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()

            last_mtime = None

            try:
                while True:
                    if IMAGE.exists():
                        mtime = IMAGE.stat().st_mtime_ns

                        if mtime != last_mtime:
                            data = IMAGE.read_bytes()
                            last_mtime = mtime

                            self.wfile.write(b"--frame\r\n")
                            self.wfile.write(
                                b"Content-Type: image/jpeg\r\n"
                            )
                            self.wfile.write(
                                f"Content-Length: {len(data)}\r\n\r\n".encode()
                            )
                            self.wfile.write(data)
                            self.wfile.write(b"\r\n")
                            self.wfile.flush()

                    time.sleep(0.05)

            except (BrokenPipeError, ConnectionResetError):
                pass

        else:
            self.send_error(404)

if __name__ == "__main__":
    server = ThreadingHTTPServer(("0.0.0.0", 8080), Handler)
    print("Servidor MJPEG iniciado en puerto 8080", flush=True)
    server.serve_forever()

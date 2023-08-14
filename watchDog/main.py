import time
import socket
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

lastPos = -1
conn = socket.socket()
serverIP = "127.0.0.1"
serverPort = 8888
path = "/Users/zhangjin/Desktop"
class MyHandler(FileSystemEventHandler):
    def on_modified(self, event):
        print("文件被修改了 %s" % event.src_path)
        difflog = ""
        with open(path,'r') as f:
            newpos = f.tell() 
            f.seek(lastPos)
            difflog = f.read(newpos - lastPos)
        f.close()
        conn.send(difflog.encode())

if __name__ == "__main__":
    with open(path,'r') as f:
        lastPos = f.tell()
    f.close()
    conn.connect(serverIP,serverPort)
    event_handler = MyHandler()
    observer = Observer()
    observer.schedule(event_handler, path, recursive=False)
    observer.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
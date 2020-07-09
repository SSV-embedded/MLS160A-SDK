#!/usr/bin/env python3

from flask import Flask, render_template, url_for
from flask_socketio import SocketIO, emit
import PyDSlog.stream as stream
import threading
import sys
import time

app = Flask(__name__)
app.debug = True
#app.host='0.0.0.0'
socketio = SocketIO(app)


x = stream.MLS160A_stream(sz_block=10, channels_to_use=["ACCX","ACCY","ACCZ","GYRX","GYRY","GYRZ"], frequency=100,
                          port="/dev/ttyR1", baudrate=115200)

def read_stream():

    while True:

        if is_streaming.is_set():

            try:

                sem.acquire()

                v = x.read(transpose=False)

                sem.release()

                socketio.emit('signals', {'ACCX': v[0], 'ACCY': v[1], 'ACCZ': v[2], 'GYRX': v[3], 'GYRY': v[4], 'GYRZ': v[5]})


            except Exception as e:

                raise e



@socketio.on('control')
def handle_message(message):

    if message == "start":
        if not is_streaming.is_set():

            sem.acquire()
            x.connect()
            x.start()
            is_streaming.set()
            sem.release()


    if message == "stop":
        if is_streaming.is_set():

            sem.acquire()
            is_streaming.clear()
            x.stop()
            x.disconnect()
            sem.release()


@app.route('/')
def index():
    return render_template("home.html")

sem = threading.Semaphore()

is_streaming = threading.Event()
is_streaming.clear()

t1 = threading.Thread(target=read_stream, args=())
t1.start()

if __name__ == "__main__":
    socketio.run(app, host='0.0.0.0', port=5000)
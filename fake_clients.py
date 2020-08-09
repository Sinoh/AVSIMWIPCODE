import socket
import sys
import time
import threading
import errno
from datetime import datetime
import random

def updateCar(car):
   car.speed = random.randint(1, 10)
   car.x_pos = car.x_pos + car.speed
   car.y_pos = random.randint(0, 5)


def drive(car):
   while True:
      car.receive_data()
      updateCar(car)
      if not pause:
         car.send_data()
      start = datetime.now()
      time.sleep(0.5)
      end = datetime.now()
      delta = end - start
      sleep_length = delta.seconds + delta.microseconds / 1000000.0
      #print(str(id(car)) + ' slept for: ' + str(sleep_length) + ' seconds')

def watchForCommand():
   global pause
   pause = False
   sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   sock.connect(('', 4001))

   while True:
      data = sock.recv(1)

      if data == b'\x00':
         pause = True
         print("PAUSE")
      elif data == b'\x01':
         pause = False
         print("RESUME")
      else:
         print(data)
         assert(False)

class Car:
   number = 0
   speed = 0
   x_pos = 0.0
   y_pos = 0.0
   z_pos = 0.0
   slot_size = 200
   sock = None
   last_recv_time = None

   def __init__(self, num, x, y, z):
      self.number = num
      self.x_pos = x
      self.y_pos = y
      self.z_pos = z

   def init_socket(self, addr, port):
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.sock.connect((addr, port))
      self.sock.setblocking(0)
      self.last_recv_time = datetime.now()

   def format_num(self, val):
      return '{:013.6f}'.format(val)

   def send_data(self):
      data_str = 'Car Number: ' + str(self.number) + ', Speed: ' + str(car.speed) + ', Gear:-01,PX:' + self.format_num(self.x_pos) + ',PY:' + \
         self.format_num(self.y_pos) + ',PZ:' + self.format_num(self.z_pos) + \
         ',OW:-0.000000,OX:-0.000000,OY:-0.000000,OZ:-0.000000' + '\r'
      num_bytes = self.sock.send(data_str.encode())
      print(str(id(self)) + ' sent: ' + str(num_bytes) + ' bytes sent')

   def receive_data(self):
      data = bytes()

      try:
         new_data = self.sock.recv(self.slot_size)

         while new_data:
            data += new_data
            new_data = self.sock.recv(self.slot_size)
      except socket.error as e:
         err = e.args[0]
         if not (err == errno.EAGAIN or err == errno.EWOULDBLOCK):
            print(e)
            sys.exit(1)

      if data:
         #print('received: ' + str(id(self)) + ' received: ' + str(data))
         time = datetime.now()

         delta = time - self.last_recv_time
         print(str(id(self)) + ' received ' + str(len(data)) + ' bytes, time since last receive or init: ' + \
            str((delta.seconds + delta.microseconds / 1000000.0)))
         self.last_recv_time = time


num_cars = sys.argv[1]
cars = []
start_x = 0.0
start_y = 0.0

threads = []
pause = False

thread = threading.Thread(target=watchForCommand)
thread.start()
threads.append(thread)

time.sleep(0.050)

for i in range(int(num_cars)):
   car = Car(i, start_x, start_y, 0.0)
   start_x += 20
   start_y += 0
   car.init_socket('', 4001)
   car.send_data()
   cars.append(car)

for car in cars:
   thread = threading.Thread(target=drive, args=[car])
   thread.start()
   threads.append(thread)

for thread in threads:
   thread.join()


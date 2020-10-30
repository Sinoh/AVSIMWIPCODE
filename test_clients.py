import socket
import sys
import time
import threading
import errno
from datetime import datetime
import random
import struct

# Time ns-3 expects test_clients to run then pause
synch_time = 1

# Update the car coordinates
# ignores z coord
def updateCar(car):
   car.speed = random.randint(1, 10)
   car.x_pos = car.x_pos + random.randint(0, 5)
   car.y_pos = car.y_pos + random.randint(0, 5)

# Each car has a thread that constanly runs drive
def drive(car):
  loop_count = 0 # used to check that car did in fact drive for synch time
  while True:
    while (drive_lock.locked()): # if can drive
      if loop_count == 1:
        print("Starting drive")
        start = datetime.now() # get start driving time
      updateCar(car)
      loop_count += 1
      end = datetime.now() # get each end time
    if loop_count > 0:  # if reached: end of synch time
      delta = end - start
      sleep_length = delta.seconds + delta.microseconds / 1000000.0
      print(str(car.number) + ' drove for: ' + str(sleep_length) + ' seconds')
      loop_count = 0

def sendCar(clientsocket):
  print("Server sending car information for " + str(len(cars)) + " cars")
  # data_str = str(len(cars))
  # num_bytes = clientsocket.send(data_str.encode())
  # print("Server sent:" + str(num_bytes) + ' bytes ')

  count = 0 # only send one car for now -> testing var
  for car in cars:
    if count == 1:
      return
    data_str = str(car.number) + ':' + str(car.speed) + ':' + car.format_num(car.x_pos) + ':' + car.format_num(car.y_pos) + ':' + car.format_num(car.z_pos) + '\r'
    num_bytes = clientsocket.send(data_str.encode())
    print(str(car.number) + ' sent: ' + str(num_bytes) + ' bytes sent. String: ', data_str)
  print("Done sending")

# Main server thread communicating with NS-3
def watchForCommand():
   global pause
   pause = False
   serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   serverSocket.bind(("localhost", port))
   serverSocket.listen(1)
   print("Connected to port", port)

   while True:
      (clientsocket, address) = serverSocket.accept()
      print("Connection from ", clientsocket)

      # Receive the data in small chunks and retransmit it
      while True:
        try:
          data = clientsocket.recv(20)
        except socket.error:
          print("Socket error occured: ", socket.error)
          sys.exit(1)
        
        dataList = list(data)

        if (len(dataList) > 5 and  dataList[5]== 2):
          print("Recieved simulationNode of len ", len(data))
          print("In bytes", dataList)
          drive_lock.acquire() # signal to cars they can drive
          print("Server: Aquire drive_lock")
          time.sleep(synch_time)
          drive_lock.release() # signal stop simulation
          print("Server: Release drive_lock")
          sendCar(clientsocket) # send information


# Car struct
class Car:
   number = 0
   speed = 0
   x_pos = 0.0
   y_pos = 0.0
   z_pos = 0.0
  #  last_recv_time = None

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


# user input to define number of cars and port
# usage: python3 test_clients.py 1 4001
num_cars = sys.argv[1] 
port = int(sys.argv[2])

# Used as a lock to indicate if the cars can drive
drive_lock = threading.Lock()

cars = []
start_x = 0.0
start_y = 0.0

threads = []
pause = False

thread = threading.Thread(target=watchForCommand)
thread.start()
threads.append(thread)

for i in range(int(num_cars)):
   car = Car(i, start_x, start_y, 0.0)
   start_x += 20
   start_y += 1
   cars.append(car)

# thread for each car
for car in cars:
   thread = threading.Thread(target=drive, args=[car])
   thread.start()
   threads.append(thread)

# for thread in threads:
#    thread.join()
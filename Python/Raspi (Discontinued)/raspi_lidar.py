#################################################################################################################################
"""
Raspi LIDAR Module - Acts as Frost's Depth Perception

An Attempt to convert our working code base onto a Raspberry Pi
Performs the same functions as the original LIDAR Module


************************************************************ARCHIVED*************************************************************
- Determined that the Raspberry Pi's computational power is not enough for all our algorithms, even under best conditions
still too slow
- The amount of hardware used by this project overloads the Raspberry Pi and causes too under voltage warnings from too much 
current draw. 

We stopped the exploration mainly due to this for fear of damaging the Raspberry Pi or lack of sufficient computatinal power
Stopped as of 12/9/16

Written by Kevin Zhang
"""
################################################################################################################################


import time
import serial
import sys
import math
import cv2
import numpy as np
import os
from multiprocessing import Process


class LidarThread(Process):
    """
    multiprocessing class for LIDAR, used for multiprocessing and communicating data to main
    """
    def __init__(self, queue, ser):
        super(LidarThread, self).__init__()
        self.queue = queue
        self.ser = ser
        self.lidar = Lidar(self.ser)
        self.lidar_data = None


    def run(self):
        while True:

            self.lidar_data = self.lidar.get_reading()
            if self.lidar_data is not None:
                self.queue.put((1, self.lidar_data))

        self.ser.close()


class Lidar(object):
    """
    the main LIDAR object, which receives and parses data from the LIDAR
    """
    def __init__(self, ser):
        self.inSync = False
        self.sync = 0
        self.bytesRead = 0
        self.j = 0
        self.counter = 0
        self.dataReady = False
        self.dataArray = []
        self.ser = ser
        self.tempAngle = 0
        self.tempDistance = 0
        self.angle = 0
        self.distance = 0

    def get_reading(self):
        """
        reads data from LIDAR's serial port and parses it into meaningful data
        """
        self.bytesRead = self.ser.inWaiting()
        data = self.ser.read()
        
        if(len(data) > 0):
            #print(sync)
            if ((ord(data) == 0xCC) and (self.sync == 0)) :
                self.sync+=1
            elif ((ord(data) == 0xDD) and (self.sync == 1)) :
                self.sync+=1
            elif ((ord(data) == 0xEE) and (self.sync == 2)) :
                self.sync+=1
            elif ((ord(data) == 0xFF) and (self.sync == 3)) :
                self.sync+=1
            else:
                self.sync = 0

            #print(inSync)
            if(self.inSync):
                if(self.j == 0):
                    #print("HighBit:")
                    #print(ord(data)<<8)
                    self.tempAngle = ord(data) << 8
                    self.j += 1
                    self.dataReady = False
                elif(self.j == 1):
                    #print("LowBit:")
                    #print(ord(data))
                    self.angle = self.tempAngle + ord(data)
                    self.j += 1
                    self.dataReady = False
                elif(self.j == 2):
                    self.tempDistance = ord(data) << 8
                    self.j += 1
                    self.dataReady = False
                elif(self.j == 3):
                    self.distance = self.tempDistance + ord(data)
                    self.j = 0
                    self.dataReady = True
                if(self.sync == 0 and self.dataReady):
                    self.dataArray.append((self.angle, self.distance))
                    #print(angle , distance)
                    #time.sleep()

            if (self.sync == 4):
                self.inSync = True
                self.counter += 1
                result = self.dataArray
                self.dataArray = []
                return result

        return None

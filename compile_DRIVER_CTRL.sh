#!/bin/bash

# compile the RDB client example

g++ -o sampleClientRDB_DRIVER_CTRL ../Common/RDBHandler.cc ExampleConsole_DRIVER_CTRL.cpp -I../Common/

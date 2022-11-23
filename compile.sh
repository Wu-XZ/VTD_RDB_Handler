#!/bin/bash

# compile the RDB client example

g++ -o sampleClientRDB ../Common/RDBHandler.cc ExampleConsole.cpp -I../Common/

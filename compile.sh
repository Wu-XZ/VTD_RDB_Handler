#!/bin/bash

# compile the RDB client example

g++ -o sampleClientRDB ./vtd/RDBHandler.cc RDB_read.cpp -I./vtd/

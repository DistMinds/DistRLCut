#!/usr/bin/make
# Makefile: makefile for Revolver algorithms
# (c) Haobin Tan, 2022


# objects = main.cpp Graph.h LearningAutomaton.h
# all : $(objects)
# 	g++ -fopenmp -O3 main.cpp -o main

# clean:
# 	rm *.o


# CC = g++
CC = mpic++
CFLAGS = -fopenmp -O3
OBJS = main.o
TARGET = main 
RM = rm -f
$(TARGET):$(OBJS) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) 
$(OBJS):%.o:%.cpp Graph_dis_mem.h worker.h CuSP.h
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	-$(RM) $(TARGET) $(OBJS)

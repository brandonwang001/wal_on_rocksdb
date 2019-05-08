ROCKSDB_PATH := /usr/local/Cellar/rocksdb/5.18.3
GLOG_PATH := /usr/local/Cellar/glog/0.4.0
GFLAGS_PATH := /usr/local/Cellar/gflags/2.2.2
PROTOBUF_PATH := /usr/local/Cellar/protobuf/3.7.1
GTEST_PATH := /usr/local/Cellar/gtest/HEAD-39f72ea
WAL_PATH := $(shell pwd)

GXX  := g++
SRCS := $(WAL_PATH)/src/common/error.cc
SRCS += $(WAL_PATH)/src/idl/wal.pb.cc
SRCS += $(WAL_PATH)/src/wal_stream.cc
OBJS += error.o
OBJS += wal.pb.o
OBJS += wal_stream.o
TARGET_NAME := --shared -fPIC -o libwal.so
INCLUDES := -I$(ROCKSDB_PATH)/include
INCLUDES += -I$(GLOG_PATH)/include
INCLUDES += -I$(GFLAGS_PATH)/include
INCLUDES += -I$(PROTOBUF_PATH)/include
INCLUDES += -I$(GTEST_PATH)/include
INCLUDES += -I$(WAL_PATH)/include
INCLUDES += -I$(WAL_PATH)/src
LIBS := -L$(ROCKSDB_PATH)/lib
LIBS += -L$(GLOG_PATH)/lib
LIBS += -L$(GFLAGS_PATH)/lib
LIBS += -L$(PROTOBUF_PATH)/lib
LIBS += -L$(GTEST_PATH)/lib
LIBS += -L$(WAL_PATH)/lib
LIBS += -lrocksdb
LIBS += -lglog
LIBS += -lgflags
LIBS += -lprotobuf
LIBS += -lgtest
FLAGS := -std=c++11

TESTSRCS := $(WAL_PATH)/test/main.cc
TESTSRCS += $(WAL_PATH)/test/wal_stream_test.cc
TESTOBJS := main.o
TESTOBJS += wal_stream_test.o
TESTTARGET := -o wal_test
TESTLIBS := $(LIBS)
TESTLIBS += -Wl:rpath=$(WAL_PATH)/lib
TESTLIBS += -lwal

all: wal waltest 

wal:
	@echo $(SRCS)
	@echo $(TARGET_NAME)
	@echo $(INCLUDES)
	@echo $(LIBS)
	@echo $(FLAGS)
	@pwd
	protoc --proto_path=$(WAL_PATH)/src/idl wal.proto --cpp_out=$(WAL_PATH)/src/idl/
	@pwd
	$(GXX) -c $(SRCS) $(INCLUDES) $(FLAGS)
	$(GXX) $(TARGET_NAME) $(OBJS) $(INCLUDES) $(LIBS)
	rm -rf ./*.o
	mv libwal.so $(WAL_PATH)/lib

waltest:
	@pwd
	protoc --proto_path=$(WAL_PATH)/src/idl wal.proto --cpp_out=$(WAL_PATH)/src/idl/
	@pwd
	$(GXX) -c $(SRCS) $(INCLUDES) $(FLAGS)
	$(GXX) $(TARGET_NAME) $(OBJS) $(INCLUDES) $(LIBS)
	mv libwal.so $(WAL_PATH)/lib
	$(GXX) -c $(TESTSRCS) $(INCLUDES) $(FLAGS)
	$(GXX) $(TESTTARGET) $(TESTOBJS) $(INCLUDES) $(TESTLIBS)
	rm -rf ./*.o
	mv wal_test $(WAL_PATH)/test
	@pwd
	export DYLD_LIBRARY_PATH=$(shell pwd)/lib
	@pwd
clean:
	rm -rf ./*.o

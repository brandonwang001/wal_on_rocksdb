ROCKSDB_PATH := /usr/local/Cellar/rocksdb/5.18.3
GLOG_PATH := /usr/local/Cellar/glog/0.4.0
GFLAGS_PATH := /usr/local/Cellar/gflags/2.2.2
PROTOBUF_PATH := /usr/local/Cellar/protobuf/3.7.1
GTEST_PATH := /usr/local/Cellar/gtest/HEAD-39f72ea
WAL_PATH := .

GXX  := g++
SRCS := $(WAL_PATH)/src/common/error.cc
OBJS += error.o
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

all: wal 

wal:
	@echo $(SRCS)
	@echo $(TARGET_NAME)
	@echo $(INCLUDES)
	@echo $(LIBS)
	@echo $(FLAGS)
	$(GXX) -c $(SRCS) $(INCLUDES) $(FLAGS)
	$(GXX) $(TARGET_NAME) $(OBJS) $(INCLUDES) $(LIBS)

clean:
	rm -rf ./*.o

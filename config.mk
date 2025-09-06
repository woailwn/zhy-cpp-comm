export BUILD_ROOT = $(shell pwd)

export INCLUDE_ROOT=$(BUILD_ROOT)/include

BUILD_DIR = $(BUILD_ROOT)/app/ 	 \
			$(BUILD_ROOT)/logic/ \
			$(BUILD_ROOT)/misc/  \
			$(BUILD_ROOT)/net/   \
			$(BUILD_ROOT)/proc/  \
			$(BUILD_ROOT)/signal/ 
		
export DEBUG = true

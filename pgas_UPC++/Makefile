UPCXX = upcxx
CONDUIT = udp
CFLAGS = -Wall -Wextra -O2 -std=c++17

SRCS = main.cpp
TARGET = main

all: $(TARGET)

$(TARGET): $(SRCS)
	UPCXX_GASNET_CONDUIT=$(CONDUIT) $(UPCXX) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OUTPUT)

run: all
ifndef ARG
	$(error ARG (number of processes) not set. Użyj 'make run ARG=4')
endif
	UPCXX_GASNET_CONDUIT=$(CONDUIT) upcxx-run -n $(ARG) $$(upcxx-nodes nodes) ./$(TARGET)

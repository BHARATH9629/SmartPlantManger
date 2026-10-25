# ============================================================
# Makefile for Smart Plant Manager (C) — MinGW on Windows
# Builds both the Console version (plant_manager.exe) 
# and the Web Server version (server.exe)
#
# Usage:  mingw32-make        (build both)
#         mingw32-make clean  (remove build files)
# ============================================================

CC      = gcc

# Targets
CONSOLE_TARGET = plant_manager.exe
SERVER_TARGET  = server.exe

# MinGW MySQL paths
MYSQL_INC = C:/Program Files/MySQL/MySQL Server 8.0/include
MYSQL_LIB = C:/Program Files/MySQL/MySQL Server 8.0/lib

CFLAGS  = -Wall -Wextra -O2 -I"$(MYSQL_INC)"
LDFLAGS = -L"$(MYSQL_LIB)" -lmysql

# Sources
CONSOLE_SRCS = main.c dao.c ui.c env.c
CONSOLE_OBJS = $(CONSOLE_SRCS:.c=.o)

SERVER_SRCS  = server.c http.c db.c ai.c env.c
SERVER_OBJS  = $(SERVER_SRCS:.c=.o)

# Build both by default
all: $(CONSOLE_TARGET) $(SERVER_TARGET)

# Console target
$(CONSOLE_TARGET): $(CONSOLE_OBJS)
	$(CC) $(CONSOLE_OBJS) -o $(CONSOLE_TARGET) $(LDFLAGS)
	@echo [OK] Console build successful: $(CONSOLE_TARGET)

# Server target
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER_TARGET) $(LDFLAGS) -lws2_32 -lwinhttp
	@echo [OK] Server build successful: $(SERVER_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q *.o $(CONSOLE_TARGET) $(SERVER_TARGET) 2>nul || true
	@echo [OK] Clean completed.
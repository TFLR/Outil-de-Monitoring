CC = gcc
CFLAGS = -Wall -g

# Liste des fichiers sources
SRCS = main.c  menu.c file_properties.c standard_permissions.c extended_permissions.c 

# Liste des fichiers objets correspondants
OBJS = $(SRCS:.c=.o)

# Nom de l'exécutable
TARGET = file_permissions

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

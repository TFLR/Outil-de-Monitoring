CC = gcc
CFLAGS = -Wall -g

# Liste des fichiers sources
SRCS = projet.c gestiondroit/main.c gestiondroit/extended_permissions.c gestiondroit/file_properties.c gestiondroit/menu.c gestiondroit/standard_permissions.c

# Liste des fichiers objets correspondants
OBJS = $(SRCS:.c=.o)

# Nom de l'exécutable
TARGET = projet

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

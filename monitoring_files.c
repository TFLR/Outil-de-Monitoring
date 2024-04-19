#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

#define LOG_INTERVAL 1
#define MAX_FILES 1000
#define MAX_PATH_LENGTH 100
#define configFile "./config.txt"


time_t lastLogTime = 0;

typedef struct
{
    char filename[256];
    mode_t permissions;
} FileInfo;

char** getPathsFromFile();
void logEvent(const char *event);
void monitorFileProperties(char **filePaths, int numPaths, FileInfo fileInfos[]);
FileInfo *scanDirectories(char **filePaths, int numPaths);
mode_t compareFilePermissions(const char *filename, mode_t previousPermissions);
int getFileIndex(const char *filename, FileInfo fileInfos[], int numFiles);
void updatePermissions(FileInfo *fileInfo, mode_t newPermissions);
void extractFilesToWatch(char **filePaths, int numPaths, char *fileToWatch[]);

int main()
{  
    char **filePaths = getPathsFromFile(); // Remplacer avec une liste de chemins à surveiller
    int numPaths = 0;
    while (filePaths[numPaths] != NULL)
    {
        numPaths++;
        //printf("%d\n", numPaths);
    }
    // Affichage des chemins
    // printf("Liste des chemins depuis main() :\n");
    // for (int i = 0; i < numPaths; i++)
    // {
    //     printf("%s\n", filePaths[i]);
    // }
    char *fileToWatch[MAX_FILES + 1];
    //extractFilesToWatch(filePaths, numPaths, fileToWatch);
    FileInfo *fileInfos = scanDirectories(filePaths, numPaths);
    // FileInfo *fileInfos = scanFilePaths(filePaths, &numPaths);
    monitorFileProperties(filePaths, numPaths, fileInfos);

    return 0;
}

char** getPathsFromFile()
{
    FILE* file = fopen(configFile, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", configFile);
        exit(1);
    }

    char** paths = NULL;
    char line[MAX_PATH_LENGTH];
    int i = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Supprimer les caractères de fin de ligne
        char* cleanLine = strtok(line, "\r\n");
        // Ignorer les lignes vides ou non valides
        if (cleanLine == NULL || cleanLine[0] == '\0')
            continue;

        // Réallouer de la mémoire pour les pointeurs de chaînes de caractères
        char** temp = realloc(paths, (i + 1) * sizeof(char*));
        if (temp == NULL)
        {
            fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
            exit(1);
        }
        paths = temp;

        // Allouer de la mémoire pour la nouvelle chaîne et la copier
        paths[i] = strdup(cleanLine);
        if (paths[i] == NULL)
        {
            fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
            exit(1);
        }
        i++;
    }
    fclose(file);

    // Terminer le tableau par un pointeur nul
    char** temp = realloc(paths, (i + 1) * sizeof(char*));
    if (temp == NULL)
    {
        fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
        exit(1);
    }
    paths = temp;
    paths[i] = NULL; // Terminer le tableau par un pointeur nul

    return paths;
}

void monitorFileProperties(char **filePaths, int numPaths, FileInfo fileInfos[])
{
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd[numPaths];

    for (int i = 0; i < numPaths; i++)
    {
        wd[i] = inotify_add_watch(fd, filePaths[i], IN_ATTRIB | IN_MODIFY | IN_CREATE | IN_DELETE);
        if (wd[i] < 0)
        {
            perror("inotify_add_watch");
            exit(EXIT_FAILURE);
        }
    }

    char buffer[4096];
    ssize_t len;

    while (1)
    {
        len = read(fd, buffer, sizeof(buffer));
        if (len < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        char *ptr = buffer;
        while (ptr < buffer + len)
        {
            struct inotify_event *event = (struct inotify_event *)ptr;

            // Appeler logEvent pour chaque événement
            // logEvent("Event detected");
            // printf("%s\n", event->name);
            // printf("%s\n", event->mask & IN_ATTRIB ? "IN_ATTRIB" : "");

            if (event->mask & IN_ATTRIB)
            {
                printf("ALERTE");
                int i;
                struct stat fileStat;
                char fullPath[MAX_FILES];

                // printf("%s\n", event->name);

                strcpy(fullPath, filePaths[0]);
                sprintf(fullPath + strlen(filePaths[0]), "/%s", event->name);

                while (stat(fullPath, &fileStat) == -1)
                {
                    i += 1;
                    // printf("%s\n", fullPath);

                    strcpy(fullPath, filePaths[i]);
                    // printf("CCCCCCCCCCCCCCCCCCC\n");

                    sprintf(fullPath + strlen(filePaths[i]), "/%s", event->name);
                    // printf("DDDDDDDDDDDDD\n");

                    if (stat(fullPath, &fileStat) == 0)
                    {
                        i = 0;
                        break;
                    }
                }

                int numFiles = MAX_FILES;

                int index = getFileIndex(fullPath, fileInfos, numFiles);
                if (index != -1)
                {

                    compareFilePermissions(fullPath, fileInfos[index].permissions);

                    updatePermissions(&fileInfos[index], fileStat.st_mode);
                    // printf("\n");
                }

                if (stat(fullPath, &fileStat) == -1)
                {
                    perror("stat");
                    exit(EXIT_FAILURE);
                }

                /*mode_t changedMode = fileStat.st_mode;
                char modeStr[11];
                mode_to_str(changedMode, modeStr);

                char message[256];
                snprintf(message, sizeof(message), "Permissions of %s have been changed to : %s.", event->name, modeStr);
                logEvent(message);*/
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    close(fd);
}

void logEvent(const char *event)
{
    time_t currentTime;
    time(&currentTime);
    if (currentTime - lastLogTime < LOG_INTERVAL)
    {
        return;
    }

    FILE *logFile = fopen("file_monitor.log", "a");
    if (logFile == NULL)
    {
        printf("Failed to open log file.\n");
        return;
    }

    lastLogTime = currentTime;
    char *timestamp = ctime(&currentTime);
    timestamp[strlen(timestamp) - 1] = '\0';

    fprintf(logFile, "[%s] %s\n", timestamp, event);
    fclose(logFile);
}

FileInfo *scanDirectories(char **filePaths, int numPaths)
{
    FileInfo* fileInfos = malloc(MAX_FILES * sizeof(FileInfo));
    if (fileInfos == NULL)
    {
        fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
        exit(EXIT_FAILURE);
    }
    int numFiles = 0;

    for (int i = 0; i < numPaths; ++i)
    {
        DIR *dir = opendir(filePaths[i]);
        if (dir == NULL)
        {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Ignorer les entrées spéciales "." et ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullPath[MAX_PATH_LENGTH];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", filePaths[i], entry->d_name);

            struct stat fileStat;
            if (stat(fullPath, &fileStat) == -1)
            {
                perror("stat");
                exit(EXIT_FAILURE);
            }

            FileInfo fileInfo;
            strncpy(fileInfo.filename, fullPath, sizeof(fileInfo.filename) - 1);
            fileInfo.filename[sizeof(fileInfo.filename) - 1] = '\0';
            fileInfo.permissions = fileStat.st_mode;

            // Vérifier si le nombre de fichiers dépasse MAX_FILES
            if (numFiles >= MAX_FILES)
            {
                fprintf(stderr, "Nombre maximal de fichiers atteint. Ignorer les fichiers supplémentaires.\n");
                break;
            }

            // Ajouter le fichier à la liste des FileInfo
            fileInfos[numFiles++] = fileInfo;
        }

        closedir(dir);
    }

    return fileInfos;
}

mode_t compareFilePermissions(const char *filename, mode_t previousPermissions)
{
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1)
    {
        perror("stat compare file perm");
        exit(EXIT_FAILURE);
    }

    if (!S_ISREG(fileStat.st_mode))
    {
        fprintf(stderr, "%s n'est pas un fichier régulier.\n", filename);
        return previousPermissions;
    }

    if (fileStat.st_mode != previousPermissions)
    {
        char message[512];
        snprintf(message, sizeof(message), "Les permissions de %s ont été modifiées :", filename);

        if ((fileStat.st_mode & S_IRUSR) != (previousPermissions & S_IRUSR))
            strcat(message, ((fileStat.st_mode & S_IRUSR) ? " Ajout du droit de Lecture pour le propriétaire, " : " Suppression du droit de Lecture pour le propriétaire, "));
        if ((fileStat.st_mode & S_IWUSR) != (previousPermissions & S_IWUSR))
            strcat(message, ((fileStat.st_mode & S_IWUSR) ? " Ajout du droit d'Ecriture pour le propriétaire, " : " Suppression du droit d'Ecriture pour le propriétaire, "));
        if ((fileStat.st_mode & S_IXUSR) != (previousPermissions & S_IXUSR))
            strcat(message, ((fileStat.st_mode & S_IXUSR) ? " Ajout du droit d'Execution pour le propriétaire, " : " Suppression du droit de d'Execution pour le propriétaire, "));
        if ((fileStat.st_mode & S_IRGRP) != (previousPermissions & S_IRGRP))
            strcat(message, ((fileStat.st_mode & S_IRGRP) ? " Ajout du droit de Lecture pour le groupe propriétaire, " : " Suppression du droit de Lecture pour le groupe propriétaire, "));
        if ((fileStat.st_mode & S_IWGRP) != (previousPermissions & S_IWGRP))
            strcat(message, ((fileStat.st_mode & S_IWGRP) ? " Ajout du droit d'Ecriture pour le groupe propriétaire, " : " Suppression du droit d'Ecriture pour le groupe propriétaire, "));
        if ((fileStat.st_mode & S_IXGRP) != (previousPermissions & S_IXGRP))
            strcat(message, ((fileStat.st_mode & S_IXGRP) ? " Ajout du droit d'Execution pour le groupe propriétaire, " : " Suppression du droit d'Execution' pour le groupe propriétaire, "));
        if ((fileStat.st_mode & S_IROTH) != (previousPermissions & S_IROTH))
            strcat(message, ((fileStat.st_mode & S_IROTH) ? " Ajout du droit de Lecture pour les autres utilisateurs, " : " Suppression du droit de Lecture pour les autres utilisateurs, "));
        if ((fileStat.st_mode & S_IWOTH) != (previousPermissions & S_IWOTH))
            strcat(message, ((fileStat.st_mode & S_IWOTH) ? " Ajout du droit d'Ecriture pour les autres utilisateurs, " : " Suppression du droit d'Ecriture pour les autres utilisateurs, "));
        if ((fileStat.st_mode & S_IXOTH) != (previousPermissions & S_IXOTH))
            strcat(message, ((fileStat.st_mode & S_IXOTH) ? " Ajout du droit d'Execution pour les autres utilisateurs, " : " Suppression du droit d'Execution pour les autres utilisateurs, "));

        logEvent(message);

        return fileStat.st_mode;
    }

    return previousPermissions;
}

int getFileIndex(const char *filename, FileInfo fileInfos[], int numFiles)
{
    for (int i = 0; i < numFiles; ++i)
    {
        if (strcmp(filename, fileInfos[i].filename) == 0)
        {
            return i;
        }
    }

    return -1;
}

void updatePermissions(FileInfo *fileInfo, mode_t newPermissions)
{
    fileInfo->permissions = newPermissions;
}

void extractFilesToWatch(char **filePaths, int numPaths, char *fileToWatch[])
{
    int fileToWatchIndex = 0;

    for (int i = 0; i < numPaths; ++i)
    {
        struct stat st;
        if (stat(filePaths[i], &st) == 0)
        {
            if (S_ISDIR(st.st_mode)) // Si c'est un répertoire
            {
                DIR *dir = opendir(filePaths[i]);
                if (dir == NULL)
                {
                    perror("opendir");
                    exit(EXIT_FAILURE);
                }

                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    // Ignorer les entrées spéciales "." et ".."
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;

                    // Construire le chemin complet du fichier à surveiller
                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s/%s", filePaths[i], entry->d_name);

                    // Ajouter le fichier à surveiller à la liste
                    fileToWatch[fileToWatchIndex++] = strdup(fullPath);
                    if (fileToWatch[fileToWatchIndex - 1] == NULL)
                    {
                        perror("Erreur lors de l'allocation de mémoire");
                        exit(EXIT_FAILURE);
                    }
                }

                closedir(dir);
            }
            else // Si c'est un fichier
            {
                char *lastSlash = strrchr(filePaths[i], '/');
                if (lastSlash != NULL)
                {
                    char *afterSlash = strdup(lastSlash + 1);

                    if (afterSlash == NULL)
                    {
                        perror("Erreur lors de l'allocation de mémoire");
                        exit(EXIT_FAILURE);
                    }

                    fileToWatch[fileToWatchIndex++] = strdup(afterSlash);
                    if (fileToWatch[fileToWatchIndex - 1] == NULL)
                    {
                        perror("Erreur lors de l'allocation de mémoire");
                        exit(EXIT_FAILURE);
                    }

                    free(afterSlash);
                }
            }
        }
        else
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }
    }

    // Terminer le tableau des noms de fichiers à surveiller avec NULL
    fileToWatch[fileToWatchIndex] = NULL;
}

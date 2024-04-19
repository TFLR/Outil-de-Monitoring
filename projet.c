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
#include <stdbool.h>
#include <ctype.h>

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

struct file
{
    char path[MAX_PATH_LENGTH];
};

struct file files[100];
int fileCount = 0;

char** getPathsFromFile();
void logEvent(const char *event);
void monitorFileProperties(char **filePaths, int numPaths, FileInfo fileInfos[]);
FileInfo *scanDirectories(char **filePaths, int numPaths);
mode_t compareFilePermissions(const char *filename, mode_t previousPermissions);
int getFileIndex(const char *filename, FileInfo fileInfos[], int numFiles);
void updatePermissions(FileInfo *fileInfo, mode_t newPermissions);
void extractFilesToWatch(char **filePaths, int numPaths, char *fileToWatch[]);
bool fileExists(const char *path);
void addFile(const char *path);
void removeFile(const char *path);
void displayFiles();

int main()
{  
    int choice;
    char path[MAX_PATH_LENGTH];
    char **filePaths = getPathsFromFile();
    int numPaths = 0;
    while (filePaths[numPaths] != NULL)
    {
        numPaths++;
    }
    char *fileToWatch[MAX_FILES + 1];

    do
    {
        printf("\nMenu :\n");
        printf("1. Ajouter un chemin de fichier à surveiller\n");
        printf("2. Supprimer un chemin du fichier surveillé\n");
        printf("3. Afficher la liste des fichiers surveillés\n");
        printf("4. Lancer le monitoring\n");
        printf("5. Quitter\n");
        printf("Entrez votre choix : ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            printf("chemin du fichier à surveiller : ");
            scanf("%s", path);
            addFile(path);
            break;
        case 2:
            printf("le chemin du fichier à supprimer : ");
            scanf("%s", path);
            removeFile(path);
            break;
        case 3:
            displayFiles();
            break;
        case 4:
            FileInfo *fileInfos = scanDirectories(filePaths, numPaths);
            monitorFileProperties(filePaths, numPaths, fileInfos);
            break;
        case 5:
            printf("terminé.\n");
            break;
        default:
            printf("Choix invalide. Veuillez entrer un nombre entre 1 et 4.\n");
        }
    } while (choice != 4);

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

            if (event->mask & IN_ATTRIB)
            {
                printf("ALERTE");
                int i = 0;
                struct stat fileStat;
                char fullPath[MAX_FILES];
                strcpy(fullPath, filePaths[0]);
                sprintf(fullPath + strlen(filePaths[0]), "/%s", event->name);

                while (stat(fullPath, &fileStat) == -1)
                {
                    i += 1;

                    strcpy(fullPath, filePaths[i]);

                    sprintf(fullPath + strlen(filePaths[i]), "/%s", event->name);

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
                }

                if (stat(fullPath, &fileStat) == -1)
                {
                    perror("stat");
                    exit(EXIT_FAILURE);
                }
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

bool fileExists(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file != NULL)
    {
        fclose(file);
        return true;
    }
    return false;
}

void addFile(const char *path)
{
    if (!fileExists(path))
    {
        printf("Le fichier n'existe pas.\n");
        return;
    }

    FILE *fichier = fopen(configFile, "r+");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }

    // Déplacer le curseur à la fin du fichier
    fseek(fichier, 0, SEEK_END);
    
    // Vérifier si le fichier est vide ou si la dernière ligne est vide
    bool isEmptyFile = (ftell(fichier) == 0);
    if (!isEmptyFile)
    {
        // Reculer le curseur jusqu'à la dernière ligne non vide
        fseek(fichier, -1, SEEK_CUR);
        while (fgetc(fichier) == '\n')
        {
            fseek(fichier, -2, SEEK_CUR);
            if (ftell(fichier) <= 0)
            {
                isEmptyFile = true; // Si le fichier est vide après vérification, sortir de la boucle
                break;
            }
        }
    }

    // Ajouter un saut de ligne si nécessaire
    if (!isEmptyFile)
    {
        fprintf(fichier, "\n");
    }

    // Ajouter le nouveau chemin
    fprintf(fichier, "%s", path);

    fclose(fichier);
    printf("Fichier ajouté avec succès !\n");
}

void removeFile(const char *path)
{
    FILE *fichier = fopen(configFile, "r");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }

    FILE *tempFile = fopen("./temp.txt", "w");
    if (tempFile == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier temporaire\n");
        exit(1);
    }

    char line[MAX_PATH_LENGTH];
    bool removed = false;
    while (fgets(line, sizeof(line), fichier) != NULL)
    {
        char *cleanLine = strtok(line, "\r\n");

        if (strcmp(cleanLine, path) != 0)
        {
            fputs(line, tempFile);
            fputs("\n", tempFile);
        }
        else
        {
            removed = true;
        }
    }

    fclose(fichier);
    fclose(tempFile);

    if (removed)
    {
        remove(configFile);
        rename("./temp.txt", configFile);
        printf("Fichier supprimé avec succès !\n");
    }
    else
    {
        remove("./temp.txt");
        printf("Le fichier n'a pas été trouvé dans la liste.\n");
    }
}

void displayFiles()
{
    printf("Liste des fichiers surveillés :\n");

    FILE *fichier = fopen(configFile, "r");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }

    char line[MAX_PATH_LENGTH];
    int fileNumber = 1;
    while (fgets(line, sizeof(line), fichier) != NULL)
    {
        printf("%d. %s", fileNumber, line);
        fileNumber++;
    }

    fclose(fichier);
}

#include <gtk/gtk.h>
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
#include <pthread.h>

#define LOG_INTERVAL 1
#define MAX_FILES 1000
#define MAX_PATH_LENGTH 100
#define configFile "./config.txt"

time_t lastLogTime = 0;

pthread_t fileMonitoringThread;

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

char **getPathsFromFile();
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

GtkWidget *window;
GtkWidget *addButton;
GtkWidget *removeButton;
GtkWidget *displayButton;
GtkWidget *logButton;
GtkWidget *textView;
GtkWidget *logWindow;
GtkWidget *logTextView = NULL;

void onAddButtonClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Ajouter un chemin",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Annuler",
                                         GTK_RESPONSE_CANCEL,
                                         "_Ajouter",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        addFile(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void onRemoveButtonClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Supprimer un chemin",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Annuler",
                                         GTK_RESPONSE_CANCEL,
                                         "_Supprimer",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        removeFile(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void update_textview(const gchar *text)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

    gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
}

void onDisplayButtonClicked(GtkWidget *widget, gpointer data)
{
    char **filePaths = getPathsFromFile();

    GtkWidget *dialog;
    dialog = gtk_dialog_new_with_buttons("Liste des chemins surveillés",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL,
                                         "_Fermer",
                                         GTK_RESPONSE_CLOSE,
                                         NULL);

    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *label = gtk_label_new(NULL);

    char *pathsText = g_strdup("");
    int i = 0;
    while (filePaths[i] != NULL)
    {
        char *line = g_strdup_printf("%d. %s\n", i + 1, filePaths[i]);
        pathsText = g_strconcat(pathsText, line, NULL);
        g_free(line);
        i++;
    }

    gtk_label_set_text(GTK_LABEL(label), pathsText);
    g_free(pathsText);

    gtk_container_add(GTK_CONTAINER(contentArea), label);

    g_strfreev(filePaths);

    gtk_widget_show_all(dialog);
}

gboolean refreshLog(gpointer data) {
    if (logTextView == NULL) {
        return G_RESOURCE_ERROR;
    }

    FILE *logFile = fopen("file_monitor.log", "r");
    if (logFile == NULL) {
        printf("Impossible d'ouvrir le fichier de journalisation.\n");
        return G_SOURCE_CONTINUE;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(logTextView));
    gtk_text_buffer_set_text(buffer, "", -1);

    char line[512];
    while (fgets(line, sizeof(line), logFile) != NULL) {
        gtk_text_buffer_insert_at_cursor(buffer, line, -1);
    }
    fclose(logFile);

    return G_SOURCE_CONTINUE;
}

void *fileMonitoringFunction(void *arg) {
    char **filePaths = getPathsFromFile();
    int numPaths = 0;
    while (filePaths[numPaths] != NULL)
        numPaths++;

    FileInfo *fileInfos = scanDirectories(filePaths, numPaths);

    monitorFileProperties(filePaths, numPaths, fileInfos);

    g_strfreev(filePaths);
    free(fileInfos);

    return NULL;
}

void onLogWindowDestroy(GtkWidget *widget, gpointer data) {
    if (logTextView != NULL) {
        gtk_widget_destroy(logTextView);
        logTextView = NULL;
    }
}

void onLogButtonClicked(GtkWidget *widget, gpointer data) {
    logWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(logWindow), "Journal des événements");
    gtk_container_set_border_width(GTK_CONTAINER(logWindow), 10);
    
    g_signal_connect(logWindow, "destroy", G_CALLBACK(onLogWindowDestroy), NULL);

    logTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(logTextView), FALSE);
    gtk_container_add(GTK_CONTAINER(logWindow), logTextView);

    gtk_widget_show_all(logWindow);

    pthread_create(&fileMonitoringThread, NULL, fileMonitoringFunction, NULL);

    g_timeout_add_seconds(LOG_INTERVAL, refreshLog, NULL);
}

int main(int argc, char *argv[]) {
    
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Monitor");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    addButton = gtk_button_new_with_label("Ajouter un chemin");
    g_signal_connect(addButton, "clicked", G_CALLBACK(onAddButtonClicked), NULL);

    removeButton = gtk_button_new_with_label("Supprimer un chemin");
    g_signal_connect(removeButton, "clicked", G_CALLBACK(onRemoveButtonClicked), NULL);

    displayButton = gtk_button_new_with_label("Afficher les chemins");
    g_signal_connect(displayButton, "clicked", G_CALLBACK(onDisplayButtonClicked), NULL);

    logButton = gtk_button_new_with_label("Afficher les logs");
    g_signal_connect(logButton, "clicked", G_CALLBACK(onLogButtonClicked), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), addButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), removeButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), displayButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), logButton, TRUE, TRUE, 0);

    gtk_widget_show_all(window);

    gtk_main();

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
                isEmptyFile = true;
                break;
            }
        }
    }

    if (!isEmptyFile)
    {
        fprintf(fichier, "\n");
    }

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
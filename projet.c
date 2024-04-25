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
#include <libgen.h>
#include <pwd.h>

#define LOG_INTERVAL 1
#define MAX_FILES 1000
#define MAX_PATH_LENGTH 256
#define MAX_HASH_LENGTH 129
#define configFile "./config.txt"
#define SENSITIVE_YES 1
#define SENSITIVE_NO 0

time_t lastLogTime = 0;

pthread_t Thread;
pthread_t fileMonitoringThread;

typedef struct
{
    char filename[256];
    mode_t permissions;
    bool sensitive;
} FileInfo;
typedef enum
{
    OWNER,
    GROUP,
    OTHERS,
} PermissionType;

typedef struct
{
    gchar *path;
    mode_t mode;
    gboolean addPermission;
    GtkWidget *infoLabel;
    PermissionType type;
} PermissionData;
struct file
{
    char path[MAX_PATH_LENGTH];
};

struct file files[100];
int fileCount = 0;
int numPaths = 0;
struct stat buf;
char command[1000];
char output[1000];

// fonction gestion droit
void ajouteperms(char *fichier, int addid);
void supprimeperms(char *fichier, int suppid);
int menuperms();
int menupermsetendue();
int menu(char *fichier);
void ajoutepermsetendue(char *fichier, int addidetendue);
void supprimepermsetendue(char *fichier, int suppidetendue);
void proprietefile(char *fichier);
int gestiondroit();

char **getPathsFromFile();
void logEvent(const char *event);
void monitorFileProperties(char **filePaths, int numPaths, FileInfo fileInfos[]);
FileInfo *scanDirectories(char **filePaths, int numPaths);
mode_t compareFilePermissions(const char *filename, mode_t previousPermissions);
int getFileIndex(const char *filename, FileInfo fileInfos[], int numFiles);
void updatePermissions(FileInfo *fileInfo, mode_t newPermissions);
void extractFilesToWatch(char **filePaths, int numPaths, char *fileToWatch[]);
bool fileExists(const char *path);
void addFile(const char *path, bool sensitive, FileInfo *fileInfos);
void removeFilePathByIndex(int index);
void displayFiles();
void parseFilePaths(char *filePaths[], int size);
void onAddButtonClicked(GtkWidget *widget, gpointer data);
void onRemoveButtonClicked(GtkWidget *widget, gpointer data);
void update_textview(const gchar *text);
void onDisplayButtonClicked(GtkWidget *widget, gpointer data);
void onLogButtonClicked(GtkWidget *widget, gpointer data);
void *fileMonitoringFunction();
void onLogWindowDestroy(GtkWidget *widget, gpointer data);
void setFileSensitivity(FileInfo *fileInfo, bool sensitive);
char *calculateSHA512(const char *path);
void *monitorFilesHash();
void onPathButtonClicked(GtkWidget *widget, gpointer data);
void modifyPermissions(GtkWidget *widget, gpointer mode_ptr);
void updatePermissionsDisplay(GtkWidget *label, const char *path);
void addPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel, PermissionType type);
void removePermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel, PermissionType type);
void addSpecialPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel);
void removeSpecialPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel);
gchar *formatPermissions(mode_t mode);
gchar *formatSinglePermissionSet(const gchar *label, mode_t perms);
gchar *formatSpecialPermissions(mode_t mode);
mode_t calculateMode(mode_t baseMode, PermissionType type);

GtkWidget *window;
GtkWidget *addButton;
GtkWidget *removeButton;
GtkWidget *displayButton;
GtkWidget *logButton;
GtkWidget *textView;
GtkWidget *logWindow;
GtkWidget *logTextView = NULL;

int main(int argc, char *argv[])
{

    char path[MAX_PATH_LENGTH];
    bool sensitive;
    int intValue;
    int number;
    FileInfo fileInfos[MAX_FILES];
    char **filePaths = getPathsFromFile();
    pthread_create(&Thread, NULL, monitorFilesHash, configFile);

    int choice;
    do
    {
        printf("\nMenu :\n");
        printf("1. Ajouter un chemin de fichier à surveiller\n");
        printf("2. Supprimer un chemin du fichier surveillé\n");
        printf("3. Afficher la liste des fichiers surveillés\n");
        printf("4. Lancer le monitoring\n");
        printf("5. Lancer le programme de gestion d'accès\n");
        printf("6. Interface graphique\n");
        printf("7. Quitter\n");
        printf("Entrez votre choix : ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
        {
            printf("Chemin du fichier à surveiller : ");
            scanf("%s", path);
            printf("Le fichier est-il sensible ? (1 pour oui, 0 pour non) : ");
            scanf("%d", &intValue);
            sensitive = (intValue != 0);
            addFile(path, sensitive != 0, fileInfos);
            break;
        }
        case 2:
            printf("le numéro du chemin à supprimer : ");
            scanf("%d", &number);
            removeFilePathByIndex(number);
            break;
        case 3:
            displayFiles();
            break;
        case 4:
            pthread_create(&fileMonitoringThread, NULL, fileMonitoringFunction, NULL);
            printf("Le monitoring est lancé !\n");
            break;
        case 5:
            gestiondroit();
            break;
        case 6:
            gtk_init(&argc, &argv);
            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(window), "File Monitor");
            gtk_container_set_border_width(GTK_CONTAINER(window), 10);
            g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

            addButton = gtk_button_new_with_label("Ajouter un chemin");
            FileInfo *fileInfos = scanDirectories(filePaths, numPaths);
            g_signal_connect(addButton, "clicked", G_CALLBACK(onAddButtonClicked), fileInfos);

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

            break;
        case 7:
            printf("Terminé.\n");
            break;
        default:
            printf("Choix invalide. Veuillez entrer un nombre entre 1 et 6.\n");
        }
    } while (choice != 7);

    for (int i = 0; i < numPaths; i++)
    {
        free(filePaths[i]);
    }
    free(filePaths);

    return 0;
}

void parseFilePaths(char *filePaths[], int size)
{
    for (int i = 0; i < size; i++)
    {
        char *filePath = filePaths[i];
        int len = strlen(filePath);

        // Parcourir les caractères du chemin de fichier en partant de la fin
        for (int j = len - 1; j >= 0; j--)
        {
            if (filePath[j] == '.')
            { // Si le caractère est un point
                // Trouver le dernier '/'
                int k;
                for (k = j - 1; k >= 0; k--)
                {
                    if (filePath[k] == '/')
                    {
                        break;
                    }
                }

                // Couper la chaîne à partir de ce point
                if (k >= 0)
                {
                    filePath[k] = '\0';
                }
                break; // Sortir de la boucle une fois trouvé le point
            }
        }
    }
}

char **getPathsFromFile()
{
    FILE *file = fopen(configFile, "a+");
    if (file == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", configFile);
        exit(1);
    }

    char **paths = NULL;
    char line[MAX_PATH_LENGTH];
    int i = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Supprimer les caractères de fin de ligne
        char *cleanLine = strtok(line, "\r\n");
        if (cleanLine == NULL || cleanLine[0] == '\0')
            continue;

        char path[MAX_PATH_LENGTH];
        if (sscanf(cleanLine, "%s", path) != 1)
        {
            fprintf(stderr, "Erreur lors de la lecture du chemin depuis le fichier\n");
            exit(1);
        }

        // Réallouer de la mémoire pour les pointeurs de chaînes de caractères
        char **temp = realloc(paths, (i + 1) * sizeof(char *));
        if (temp == NULL)
        {
            fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
            exit(1);
        }
        paths = temp;

        // Allouer de la mémoire pour la nouvelle chaîne et la copier
        paths[i] = strdup(path);
        if (paths[i] == NULL)
        {
            fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
            exit(1);
        }
        i++;
    }
    fclose(file);

    char **temp = realloc(paths, (i + 1) * sizeof(char *));
    if (temp == NULL)
    {
        fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
        exit(1);
    }
    paths = temp;
    paths[i] = NULL;

    return paths;
}

void monitorFileProperties(char **filePaths, int numPaths, FileInfo fileInfos[])
{
    if (filePaths == NULL || numPaths <= 0)
    {
        printf("No file paths provided for monitoring.\n");
        return;
    }

    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd[numPaths];

    for (int i = 0; i < numPaths; i++)
    {
        wd[i] = inotify_add_watch(fd, filePaths[i], IN_ATTRIB);
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
                    perror("stat1");
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
    FileInfo *fileInfos = malloc(MAX_FILES * sizeof(FileInfo));
    if (fileInfos == NULL)
    {
        fprintf(stderr, "Erreur lors de l'allocation de mémoire\n");
        exit(1);
    }
    int numFiles = 0;

    for (int i = 0; i < numPaths; ++i)
    {
        struct stat st;
        if (stat(filePaths[i], &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
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
                    // snprintf(fullPath, sizeof(fullPath), "%s/%s", filePaths[i], entry->d_name);

                    strncpy(fullPath, filePaths[i], MAX_PATH_LENGTH);
                    strncat(fullPath, "/", sizeof(fullPath) - strlen(fullPath) - 1);
                    strncat(fullPath, entry->d_name, sizeof(fullPath) - strlen(fullPath) - 1);

                    struct stat fileStat;
                    if (stat(fullPath, &fileStat) == -1)
                    {
                        perror("stat2");
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
            else if (S_ISREG(st.st_mode))
            {
                FileInfo fileInfo;
                strncpy(fileInfo.filename, filePaths[i], sizeof(fileInfo.filename) - 1);
                fileInfo.filename[sizeof(fileInfo.filename) - 1] = '\0';
                fileInfo.permissions = st.st_mode;

                fileInfos[numFiles++] = fileInfo;
            }
            else
            {
                fprintf(stderr, "%s n'est ni un répertoire ni un fichier régulier.\n", filePaths[i]);
            }
        }
        else
        {
            perror("stat3");
            exit(EXIT_FAILURE);
        }
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

void setFileSensitivity(FileInfo *fileInfo, bool sensitive)
{
    fileInfo->sensitive = sensitive;
}

void addFile(const char *path, bool sensitive, FileInfo *fileInfos)
{
    if (!fileExists(path))
    {
        printf("Le fichier %s n'existe pas.\n", path);
        return;
    }

    FILE *fichier = fopen(configFile, "a+");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }

    fseek(fichier, 0, SEEK_END);

    fprintf(fichier, "%s %d %s\n", path, sensitive ? 1 : 0, calculateSHA512(path));

    if (fileCount < MAX_FILES)
    {
        strncpy(fileInfos[fileCount].filename, path, MAX_PATH_LENGTH - 1);
        fileInfos[fileCount].filename[MAX_PATH_LENGTH - 1] = '\0';
        fileInfos[fileCount].sensitive = sensitive;
        fileCount++;

        if (sensitive)
        {
            if (chmod(path, 0640) != 0)
            {
                fprintf(stderr, "Erreur lors de la modification des autorisations du fichier %s\n", path);
            }
        }
    }
    else
    {
        fprintf(stderr, "Nombre maximal de fichiers atteint.\n");
    }

    fclose(fichier);
    printf("Fichier ajouté avec succès : %s\n", path);
}

void removeFilePathByIndex(int index)
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
    int lineNumber = 0;
    while (fgets(line, sizeof(line), fichier) != NULL)
    {
        lineNumber++;

        if (lineNumber != index)
        {
            fputs(line, tempFile);
        }
    }
    pthread_create(&fileMonitoringThread, NULL, fileMonitoringFunction, NULL);
    fclose(fichier);
    fclose(tempFile);

    remove(configFile);
    rename("./temp.txt", configFile);
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
        char path[MAX_PATH_LENGTH];
        int sensitivity;

        if (sscanf(line, "%s %d", path, &sensitivity) == 2)
        {
            printf("%d. %s (Sensible: %s)\n", fileNumber, path, sensitivity ? "Oui" : "Non");
            fileNumber++;
        }
    }
    fclose(fichier);
}

char *calculateSHA512(const char *path)
{

    if (stat(path, &buf) != 0 || S_ISDIR(buf.st_mode))
    {
        fprintf(stderr, " ");
        return NULL;
    }
    snprintf(command, sizeof(command), "sha512sum %s", path);

    FILE *pipe = popen(command, "r");
    if (pipe == NULL)
    {
        fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
        return NULL;
    }
    fgets(output, sizeof(output), pipe);

    pclose(pipe);

    output[strcspn(output, "\n")] = '\0';

    char *hash = strtok(output, " ");
    if (hash == NULL)
    {
        fprintf(stderr, "Erreur lors de l'allocation de mémoire.\n");
        return NULL;
    }
    return hash;
}
void *monitorFilesHash()
{
    while (1)
    {
        FILE *config = fopen(configFile, "r");
        if (config == NULL)
        {
            fprintf(stderr, "Error opening configuration file\n");
            exit(1);
        }

        char line[MAX_PATH_LENGTH];
        while (fgets(line, sizeof(line), config) != NULL)
        {
            char path[MAX_PATH_LENGTH];
            int sensitivity;
            char storedHash[MAX_HASH_LENGTH];

            if (sscanf(line, "%s %d %s", path, &sensitivity, storedHash) == 3)
            {
                char *hash = calculateSHA512(path);
                if (hash == NULL)
                {
                    if (stat(path, &buf) != 0 || S_ISDIR(buf.st_mode))
                    {
                        fprintf(stderr, " ");
                    }else{
                        fprintf(stderr, "Error calculating hash for file: %s\n", path);
                    }
                    continue;
                }

                if (strcmp(hash, storedHash) != 0)
                {
                    char logMessage[512];
                    snprintf(logMessage, sizeof(logMessage), "Changement de Hash detecté pour le fichier : %s", path);
                    logEvent(logMessage);
                }
            }
        }

        fclose(config);

        sleep(60);
    }
    return NULL;
}

void onAddButtonClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *checkButton;
    FileInfo *fileInfos = (FileInfo *)data;

    dialog = gtk_dialog_new_with_buttons("Ajouter un chemin",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL,
                                         "_Annuler",
                                         GTK_RESPONSE_CANCEL,
                                         "_Ajouter",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    label = gtk_label_new("Entrez le chemin du fichier à surveiller :");
    gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);

    checkButton = gtk_check_button_new_with_label("Sensible");
    gtk_box_pack_start(GTK_BOX(content_area), checkButton, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton));
        addFile(text, active, fileInfos);
    }

    gtk_widget_destroy(dialog);
}

void onRemoveButtonClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *entry;

    dialog = gtk_dialog_new_with_buttons("Supprimer un chemin",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL,
                                         "_Annuler",
                                         GTK_RESPONSE_CANCEL,
                                         "_Supprimer",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    label = gtk_label_new("Entrez le numéro du fichier à supprimer :");
    gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        int index = atoi(text);
        if (pthread_cancel(fileMonitoringThread) == 0)
            {
                pthread_cancel(fileMonitoringThread);
                printf("Le monitoring est arrêté !\n");
            }
        removeFilePathByIndex(index);
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

void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void onDisplayButtonClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *scrolled_window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *sensitive_label;

    // Création du dialogue sans modalité pour tester
    dialog = gtk_dialog_new_with_buttons("Liste des fichiers surveillés",
                                         GTK_WINDOW(window),
                                         0, // Aucune modalité
                                         "_Fermer", GTK_RESPONSE_CLOSE,
                                         NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 300);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scrolled_window), vbox);

    FILE *fichier = fopen(configFile, "r");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }

    char line[MAX_PATH_LENGTH];
    int lineNumber = 1;
    while (fgets(line, sizeof(line), fichier) != NULL)
    {
        char path[MAX_PATH_LENGTH];
        int sensitivity;

        if (sscanf(line, "%s %d", path, &sensitivity) == 2) {
            // Créer une boîte horizontale pour contenir le numéro de ligne, le bouton et l'indication de sensibilité
            hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gchar *label_text = g_strdup_printf("%d.", lineNumber);
            label = gtk_label_new(label_text);
            g_free(label_text);

            button = gtk_button_new_with_label(path);
            sensitive_label = gtk_label_new(sensitivity ? "Sensible: Oui" : "Sensible: Non");

            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), sensitive_label, FALSE, FALSE, 0);

            gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
            
            // Connecter le bouton au callback
            g_signal_connect(button, "clicked", G_CALLBACK(onPathButtonClicked), NULL);
            lineNumber++;
        }
    }

    fclose(fichier);

    gtk_widget_show_all(dialog);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
}

gboolean refreshLog(gpointer data)
{
    if (logTextView == NULL)
    {
        return G_RESOURCE_ERROR;
    }

    FILE *logFile = fopen("file_monitor.log", "r");
    if (logFile == NULL)
    {
        printf("Impossible d'ouvrir le fichier de journalisation.\n");
        return G_SOURCE_CONTINUE;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(logTextView));
    gtk_text_buffer_set_text(buffer, "", -1);

    char line[512];
    while (fgets(line, sizeof(line), logFile) != NULL)
    {
        gtk_text_buffer_insert_at_cursor(buffer, line, -1);
    }
    fclose(logFile);

    return G_SOURCE_CONTINUE;
}

void *fileMonitoringFunction()
{
    char **filePaths = getPathsFromFile();
    int numPaths = 0;
    while (filePaths[numPaths] != NULL)
        numPaths++;

    parseFilePaths(filePaths, numPaths);
    FileInfo *fileInfos = scanDirectories(filePaths, numPaths);

    monitorFileProperties(filePaths, numPaths, fileInfos);

    g_strfreev(filePaths);
    free(fileInfos);

    return NULL;
}

void onLogWindowDestroy(GtkWidget *widget, gpointer data)
{
    if (logTextView != NULL)
    {
        gtk_widget_destroy(logTextView);
        logTextView = NULL;
    }
}

void onLogButtonClicked(GtkWidget *widget, gpointer data)
{
    logWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(logWindow), "Journal des événements");
    gtk_container_set_border_width(GTK_CONTAINER(logWindow), 10);
    gtk_window_resize(GTK_WINDOW(logWindow), 1300, 300);

    g_signal_connect(logWindow, "destroy", G_CALLBACK(onLogWindowDestroy), NULL);

    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   GTK_POLICY_ALWAYS,
                                   GTK_POLICY_ALWAYS);

    gtk_container_add(GTK_CONTAINER(logWindow), scrolledWindow);

    logTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(logTextView), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), logTextView);

    gtk_widget_show_all(logWindow);

    pthread_create(&fileMonitoringThread, NULL, fileMonitoringFunction, NULL);

    g_timeout_add_seconds(LOG_INTERVAL, refreshLog, NULL);
}
void onPathButtonClicked(GtkWidget *widget, gpointer data)
{
    const gchar *path = gtk_button_get_label(GTK_BUTTON(widget));

    GtkWidget *pathWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(pathWindow), path);
    gtk_window_set_default_size(GTK_WINDOW(pathWindow), 400, 700);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(pathWindow), vbox);

    GtkWidget *infoLabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), infoLabel, FALSE, FALSE, 0);
    updatePermissionsDisplay(infoLabel, path);

    addPermissionButtons(vbox, path, infoLabel, OWNER);
    removePermissionButtons(vbox, path, infoLabel, OWNER);
    addPermissionButtons(vbox, path, infoLabel, GROUP);
    removePermissionButtons(vbox, path, infoLabel, GROUP);
    addPermissionButtons(vbox, path, infoLabel, OTHERS);
    removePermissionButtons(vbox, path, infoLabel, OTHERS);

    GtkWidget *addSpecialLabel = gtk_label_new("Ajouter droits spéciaux :");
    gtk_box_pack_start(GTK_BOX(vbox), addSpecialLabel, FALSE, FALSE, 0);
    addSpecialPermissionButtons(vbox, path, infoLabel);

    GtkWidget *removeSpecialLabel = gtk_label_new("Supprimer droits spéciaux :");
    gtk_box_pack_start(GTK_BOX(vbox), removeSpecialLabel, FALSE, FALSE, 0);
    removeSpecialPermissionButtons(vbox, path, infoLabel);

    // Création du GtkButtonBox pour le bouton Fermer
    GtkWidget *buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_END);
    GtkWidget *closeButton = gtk_button_new_with_label("Fermer");
    gtk_container_add(GTK_CONTAINER(buttonBox), closeButton);
    gtk_box_pack_end(GTK_BOX(vbox), buttonBox, FALSE, FALSE, 0);
    g_signal_connect_swapped(closeButton, "clicked", G_CALLBACK(gtk_widget_destroy), pathWindow);

    gtk_widget_show_all(pathWindow);
}


void addPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel, PermissionType type)
{
    gchar *labelText = NULL;
    switch (type)
    {
    case OWNER:
        labelText = "Ajouter droits propriétaire:";
        break;
    case GROUP:
        labelText = "Ajouter droits groupe:";
        break;
    case OTHERS:
        labelText = "Ajouter droits autres:";
        break;
    }

    GtkWidget *label = gtk_label_new(labelText);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *addBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), addBox, FALSE, FALSE, 0);

    mode_t modes[] = {S_IRUSR, S_IWUSR, S_IXUSR}; // Base permissions for owner
    const char *labels[] = {"Lecture", "Écriture", "Exécution"};

    for (int i = 0; i < 3; i++)
    {
        GtkWidget *button = gtk_button_new_with_label(labels[i]);
        PermissionData *permData = g_new(PermissionData, 1);
        permData->path = g_strdup(path);
        permData->mode = calculateMode(modes[i], type); // Adjust mode based on the type
        permData->addPermission = TRUE;
        permData->infoLabel = infoLabel;

        gtk_box_pack_start(GTK_BOX(addBox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(modifyPermissions), permData);

    }
}

void removePermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel, PermissionType type)
{
    gchar *labelText = NULL;
    switch (type)
    {
    case OWNER:
        labelText = "Supprimer droits propriétaire:";
        break;
    case GROUP:
        labelText = "Supprimer droits groupe:";
        break;
    case OTHERS:
        labelText = "Supprimer droits autres:";
        break;
    }

    GtkWidget *label = gtk_label_new(labelText);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *removeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), removeBox, FALSE, FALSE, 0);

    mode_t modes[] = {S_IRUSR, S_IWUSR, S_IXUSR};
    const char *labels[] = {"Lecture", "Écriture", "Exécution"};

    for (int i = 0; i < 3; i++)
    {
        GtkWidget *button = gtk_button_new_with_label(labels[i]);
        PermissionData *permData = g_new(PermissionData, 1);
        permData->path = g_strdup(path);
        permData->mode = calculateMode(modes[i], type); // Adjust mode based on the type
        permData->addPermission = FALSE;
        permData->infoLabel = infoLabel;

        gtk_box_pack_start(GTK_BOX(removeBox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(modifyPermissions), permData);

    }
}
void addSpecialPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel)
{
    GtkWidget *specialBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), specialBox, FALSE, FALSE, 0);

    const char *specialLabels[] = {"Set UID", "Set GID", "Sticky Bit"};
    mode_t specialModes[] = {S_ISUID, S_ISGID, S_ISVTX}; // Corresponding special modes

    for (int i = 0; i < 3; i++)
    {
        GtkWidget *button = gtk_button_new_with_label(specialLabels[i]);
        PermissionData *permData = g_new(PermissionData, 1);
        permData->path = g_strdup(path);
        permData->mode = specialModes[i];
        permData->addPermission = TRUE; // Special permissions can only be added
        permData->infoLabel = infoLabel;

        gtk_box_pack_start(GTK_BOX(specialBox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(modifyPermissions), permData);
    }
}
void removeSpecialPermissionButtons(GtkWidget *vbox, const gchar *path, GtkWidget *infoLabel)
{
    GtkWidget *removeSpecialBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), removeSpecialBox, FALSE, FALSE, 0);

    const char *specialLabels[] = {"Supprimer Set UID", "Supprimer Set GID", "Supprimer Sticky Bit"};
    mode_t specialModes[] = {S_ISUID, S_ISGID, S_ISVTX}; // Corresponding special modes

    for (int i = 0; i < 3; i++)
    {
        GtkWidget *button = gtk_button_new_with_label(specialLabels[i]);
        PermissionData *permData = g_new(PermissionData, 1);
        permData->path = g_strdup(path);
        permData->mode = specialModes[i];
        permData->addPermission = FALSE; // Special permissions can only be removed
        permData->infoLabel = infoLabel;

        gtk_box_pack_start(GTK_BOX(removeSpecialBox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(modifyPermissions), permData);

    }
}
void modifyPermissions(GtkWidget *widget, gpointer data)
{
    PermissionData *permData = (PermissionData *)data;
    struct stat statbuf;

    // Obtention des permissions actuelles
    if (stat(permData->path, &statbuf) != 0)
    {
        gchar *errorMsg = g_strdup_printf("Erreur lors de la récupération des permissions pour %s: %s", permData->path, strerror(errno));
        gtk_label_set_text(GTK_LABEL(permData->infoLabel), errorMsg);
        g_free(errorMsg);
        return;
    }

    mode_t newMode = statbuf.st_mode;
    if (permData->addPermission)
    {
        newMode |= permData->mode; // Ajouter les droits
    }
    else
    {
        newMode &= ~permData->mode; // Supprimer les droits
    }

    // Application des nouvelles permissions
    if (chmod(permData->path, newMode) != 0)
    {
        gchar *errorMsg = g_strdup_printf("Erreur lors de la modification des permissions pour %s: %s", permData->path, strerror(errno));
        gtk_label_set_text(GTK_LABEL(permData->infoLabel), errorMsg);
        g_free(errorMsg);
    }
    else
    {
        gchar *successMsg = g_strdup_printf("Permissions mises à jour pour %s", permData->path);
        gtk_label_set_text(GTK_LABEL(permData->infoLabel), successMsg);
        g_free(successMsg);
    }

    // Mise à jour de l'affichage des permissions
    updatePermissionsDisplay(permData->infoLabel, permData->path);
}



void updatePermissionsDisplay(GtkWidget *infoLabel, const gchar *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == 0)
    {
        gchar *ownerPerms = g_strdup_printf("Propriétaire: %s%s%s",
                                            (statbuf.st_mode & S_IRUSR) ? "Lecture " : "",
                                            (statbuf.st_mode & S_IWUSR) ? "Écriture " : "",
                                            (statbuf.st_mode & S_IXUSR) ? "Exécution " : "");

        gchar *groupPerms = g_strdup_printf("Groupe: %s%s%s",
                                            (statbuf.st_mode & S_IRGRP) ? "Lecture " : "",
                                            (statbuf.st_mode & S_IWGRP) ? "Écriture " : "",
                                            (statbuf.st_mode & S_IXGRP) ? "Exécution " : "");

        gchar *otherPerms = g_strdup_printf("Autres: %s%s%s",
                                            (statbuf.st_mode & S_IROTH) ? "Lecture " : "",
                                            (statbuf.st_mode & S_IWOTH) ? "Écriture " : "",
                                            (statbuf.st_mode & S_IXOTH) ? "Exécution " : "");

        gchar *specialPerms = g_strdup_printf("Spécial: %s%s%s",
                                              (statbuf.st_mode & S_ISUID) ? "Set UID " : "",
                                              (statbuf.st_mode & S_ISGID) ? "Set GID " : "",
                                              (statbuf.st_mode & S_ISVTX) ? "Sticky Bit " : "");

        gchar *fullPermissions = g_strdup_printf("Permissions actuelles :\n%s\n%s\n%s\n%s",
                                                 ownerPerms,
                                                 groupPerms,
                                                 otherPerms,
                                                 specialPerms);
        gtk_label_set_text(GTK_LABEL(infoLabel), fullPermissions);

        g_free(ownerPerms);
        g_free(groupPerms);
        g_free(otherPerms);
        g_free(specialPerms);
        g_free(fullPermissions);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(infoLabel), "Impossible de récupérer les permissions.");
    }
}

mode_t calculateMode(mode_t baseMode, PermissionType type)
{
    switch (type)
    {
    case OWNER:
        return baseMode;
    case GROUP:
        return baseMode >> 3; // Shift to group bits
    case OTHERS:
        return baseMode >> 6; // Shift to others bits
    }
    return baseMode; // Fallback to owner if no type matches
}

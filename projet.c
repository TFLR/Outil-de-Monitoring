#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_PATH_LENGTH 100
#define configFile "./config.txt"

struct file
{
    char path[MAX_PATH_LENGTH];
};

struct file files[100];
int fileCount = 0;

bool fileExists(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file != NULL)
    {
        fclose(file);
        // printf("yes");
        return true;
    }
    // printf("no");
    return false;
}

void addFile(const char *path)
{
    if (!fileExists(path))
    {
        printf("Le fichier n'existe pas.\n");
        return;
    }

    FILE *fichier = fopen(configFile, "a");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }
    
    fseek(fichier, 0, SEEK_END);
    if (ftell(fichier) != 0)
    {
        fprintf(fichier, "\n");
    }

    strcpy(files[fileCount].path, path);
    fileCount++;
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

int main()
{
    char** paths = getPathsFromFile();
    for (int i = 0; paths[i] != NULL; i++)
    {
        printf("%s\n", paths[i]);
    }

    int choice;
    char path[MAX_PATH_LENGTH];
    do
    {
        printf("\nMenu :\n");
        printf("1. Ajouter un chemin de fichier à surveiller\n");
        printf("2. Supprimer un chemin du fichier surveillé\n");
        printf("3. Afficher la liste des fichiers surveillés\n");
        printf("4. Quitter\n");
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
            printf("terminé.\n");
            break;
        default:
            printf("Choix invalide. Veuillez entrer un nombre entre 1 et 4.\n");
        }
    } while (choice != 4);

    return 0;
}
//
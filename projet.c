#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PATH_LENGTH 100

struct file {
    char path[MAX_PATH_LENGTH];
};

struct file files[100];
int fileCount = 0;

bool fileExists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file != NULL) {
        fclose(file);
        return true;
    }
    return false;
}

void write_to_file(const char *filename, const char *text) {
    FILE *file = fopen(filename, "a");
    if (file != NULL) {
        fputs(text, file);
        fclose(file);
    } else {
        g_print("Impossible d'écrire dans le fichier.\n");
    }
}

void load_file(GtkWidget *text_view, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        char text[1000];
        gtk_text_buffer_set_text(buffer, "", -1);
        while (fgets(text, sizeof(text), file) != NULL) {
            gtk_text_buffer_insert_at_cursor(buffer, text, -1);
        }
        fclose(file);
    } else {
        g_print("Impossible d'ouvrir le fichier.\n");
    }
}

void delete_from_file(const char *filename, const char *text_to_delete) {
    FILE *fichier = fopen(filename, "r");
    if (fichier == NULL)
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de configuration\n");
        exit(1);
    }
    char temp_filename[] = "temp.txt";
    FILE *tempFile = fopen(temp_filename, "w");
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

        if (strcmp(cleanLine, text_to_delete) != 0)
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
        remove(filename);
        rename(temp_filename,filename);
        printf("Fichier supprimé avec succès !\n");
    }
    else
    {
        remove("./temp.txt");
        printf("Le fichier n'a pas été trouvé dans la liste.\n");
    }
}

void on_add_content_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *text_view = GTK_WIDGET(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    write_to_file("liste.txt", text);
    write_to_file("liste.txt", "\n");

    g_free(text);
}

void on_remove_content_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *text_view = GTK_WIDGET(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    delete_from_file("liste.txt", text);

    g_free(text);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *text_view;
    GtkWidget *add_button;
    GtkWidget *remove_button;
    GtkWidget *hbox;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de fichiers");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 50);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    text_view = gtk_text_view_new();
    gtk_widget_set_size_request(text_view, 200, -1); // Fixer la largeur de la zone de texte

    add_button = gtk_button_new_with_label("Ajouter");
    remove_button = gtk_button_new_with_label("Supprimer");

    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_content_clicked), text_view);
    g_signal_connect(remove_button, "clicked", G_CALLBACK(on_remove_content_clicked), text_view);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), text_view, TRUE, TRUE, 0); // La zone de texte utilise l'espace disponible
    gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), remove_button, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), hbox);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
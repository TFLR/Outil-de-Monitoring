#ifndef FILE_PERMISSIONS_H
#define FILE_PERMISSIONS_H

#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<dirent.h>
#include<string.h>
#include<pwd.h>
#include<libgen.h>
#include<sys/types.h>
#include<regex.h>
#include<ctype.h>
#include<unistd.h>

void ajouteperms(char *fichier, int addid);
void supprimeperms(char *fichier, int suppid);
int menuperms();
int menupermsetendue();
int menu(char *fichier);
void ajoutepermsetendue(char *fichier, int addidetendue);
void supprimepermsetendue(char *fichier, int suppidetendue);
void proprietefile(char *fichier);

#endif

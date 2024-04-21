#include "file_permissions.h"

int proprietefile(char *fichier) {

	char *file = basename(fichier);			//recupere le nom du fichier
	char *repertoire = dirname(fichier);		//recupere le repertoire du fichier

	char infofichier[PATH_MAX]; 
	strcpy(infofichier, repertoire);
	strcat(infofichier, "/");			//Concatene le fichier au repertoire pour avoir le chemin complet dans une nouvel variable
	strcat(infofichier, file);
	
	struct stat avantsuid;
	stat(infofichier, &avantsuid);
	
	char *detaildroit;

	struct stat info;
	stat(infofichier, &info);
	struct passwd *userinfo = getpwuid(info.st_uid);

	if (strcmp(repertoire,"/") == 0) {
		return 1;
	}

	printf("\n\n########## Informations actuelles du fichier ##########\n\n");
	printf("Dossier : %s\n", repertoire);


    if ((info.st_mode & S_ISGID) && (info.st_mode & S_ISUID) && (info.st_mode & S_ISVTX)) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SGID est attribué a ce répertoire, les fichiers crées sous ce répertoire seront attribué au groupe propriétaire de ce répertoire\nLe SUID est attribué a ce répertoire, aucun effet. Le SUID est n'est pas effectif sur les répertoires\nLe STICKY BIT est attribué a ce répertoire, les fichiers sous ce répertoire ne peuvent être supprimée que par propriétaire du fichier)";
	   	} else {
    		detaildroit="(Le SGID est attribué a ce fichier, le fichier est exécuter avec les droits du groupe propriétaire\nLe SUID est attribué a ce fichier, le fichier est exécuté avec les droits de l'utilisateur propriétaire\nLe STICKY BIT est attribué a ce fichier, aucun effet. Le STICKY BIT est effectif seulement sur les répertoires.)";
		} 
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if ((info.st_mode & S_ISGID) && (info.st_mode & S_ISUID)) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SGID est attribué a ce répertoire, les fichiers crées sous ce répertoire seront attribué au groupe propriétaire de ce répertoire\nLe SUID est attribué a ce répertoire, aucun effet. Le SUID est n'est pas effectif sur les répertoires)";
	   	} else {
    		detaildroit="(Le SGID est attribué a ce fichier, le fichier est exécuter avec les droits du groupe propriétaire\nLe SUID est attribué a ce fichier, le fichier est exécuté avec les droits de l'utilisateur propriétaire.)";
		} 
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if ((info.st_mode & S_ISUID) && (info.st_mode & S_ISVTX)) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SUID est attribué a ce répertoire, aucun effet. Le SUID est n'est pas effectif sur les répertoires\ntLe STICKY BIT est attribué a ce répertoire, les fichiers sous ce répertoire ne peuvent être supprimée que par propriétaire du fichier)";
	   	} else {
    		detaildroit="(Le SUID est attribué a ce fichier, le fichier est exécuté avec les droits de l'utilisateur propriétaire\nLe STICKY BIT est attribué a ce fichier, aucun effet. Le STICKY BIT est effectif seulement sur les répertoires.)";
		} 
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if ((info.st_mode & S_ISGID) && (info.st_mode & S_ISVTX)) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SGID est attribué a ce répertoire, les fichiers crées sous ce répertoire seront attribué au groupe propriétaire de ce répertoire\nLe STICKY BIT est attribué a ce répertoire, les fichiers sous ce répertoire ne peuvent être supprimée que par propriétaire du fichier)";
	   	} else {
    		detaildroit="(Le SGID est attribué a ce fichier, le fichier est exécuter avec les droits du groupe propriétaire\nLe STICKY BIT est attribué a ce fichier, aucun effet. Le STICKY BIT est effectif seulement sur les répertoires.)";
		} 
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if (info.st_mode & S_ISGID) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SGID est attribué a ce répertoire, les fichiers crées sous ce répertoire seront attribué au groupe propriétaire de ce répertoire)";
	   	} else {
    		detaildroit="(Le SGID est attribué a ce fichier, le fichier est exécuter avec les droits du groupe propriétaire.)";
	   	}
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if (info.st_mode & S_ISUID) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le SUID est attribué a ce répertoire, aucun effet. Le SUID est n'est pas effectif sur les répertoires)";
	   	} else {
    		detaildroit="(Le SUID est attribué a ce fichier, le fichier est exécuté avec les droits de l'utilisateur propriétaire.)";
	   	}
		printf("Fichier : %s \n%s\n", file, detaildroit);
	} else if (info.st_mode & S_ISVTX) {
    	if (S_ISDIR(info.st_mode)) {
	   		detaildroit="(Le STICKY BIT est attribué a ce répertoire, les fichiers sous ce répertoire ne peuvent être supprimée que par propriétaire du fichier)";
	   	} else {
    		detaildroit="(Le STICKY BIT est attribué a ce fichier, aucun effet. Le STICKY BIT est effectif seulement sur les répertoires. Son but est de permettre la supression d'un fichier dans un répertoire, seulement par le propriétaire de ce fichier)";
	   	}
		printf("Fichier : %s \n%s\n", file, detaildroit);
	}

	else {
		printf("Fichier : %s\n", file);
	}

				
	printf("Nom Propriétaire : %s\n", userinfo->pw_name);		// Recupere le nom d'utilisateur a partir du UID


	printf("Droits Propriétaire : ");
	if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_IWUSR) && (info.st_mode & S_ISUID) && (info.st_mode & S_IXUSR)) { 
		printf("Lecture, Ecriture, Exécution, [SUID]\n");
	} else if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_IWUSR) && (info.st_mode & S_ISUID)) {
		printf("Lecture, Ecriture, [SUID]\n");
	} else if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_IWUSR) && (info.st_mode & S_IXUSR)) {
	       	printf("Lecture, Ecriture, Exécution\n");	
	} else if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_IWUSR)) {
		printf("Lecture, Ecriture\n");
	} else if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_ISUID)) {
		printf("Lecture, [SUID]\n");
	} else if ((info.st_mode & S_IRUSR ) && (info.st_mode & S_IXUSR)) {
		printf("Lecture, Execution\n");
	} else if ((info.st_mode & S_IWUSR ) && (info.st_mode & S_ISUID)) {
		printf("Ecriture, [SUID]\n");
	} else if ((info.st_mode & S_IWUSR) && (info.st_mode & S_IXUSR)) {		//Verifie les droits du propriètaire du fichier
		printf("Ecriture, Execution\n");
	} else if ((info.st_mode & S_IXUSR ) && (info.st_mode & S_ISUID)) {
		printf("Exécution, [SUID]\n");
	} else if (info.st_mode & S_ISUID) {
		printf("[SUID]\n");
	} else if (info.st_mode & S_IRUSR) {
		printf("Lecture\n");
    } else if (info.st_mode & S_IWUSR) {
		printf("Ecriture\n");
	} else if (info.st_mode & S_IXUSR) {
		printf("Exécution\n");
	} else {
		printf("Pas de permissions\n");
	}


	printf("Droits Groupe : ");
	if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_IWGRP) && (info.st_mode & S_IXGRP) && (info.st_mode & S_ISGID)) { 
		printf("Lecture, Ecriture, Exécution, [SGID]\n");
	} else if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_IWGRP) && (info.st_mode & S_ISGID)) {
		printf("Lecture, Ecriture, [SGID]\n");
	} else if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_IWGRP) && (info.st_mode & S_IXGRP)) {
		printf("Lecture, Ecriture, Exécution\n");
	} else if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_IWGRP)) {
		printf("Lecture, Ecriture\n");
	} else if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_ISGID)) {
		printf("Lecture, [SGID]\n");
	} else if ((info.st_mode & S_IRGRP ) && (info.st_mode & S_IXGRP)) {
		printf("Lecture, Exécution\n");
	} else if ((info.st_mode & S_IWGRP) && (info.st_mode & S_ISGID)) {
		printf("Ecriture, [SGID]\n");
	} else if ((info.st_mode & S_IWGRP) && (info.st_mode & S_IXGRP)) {		//Meme chose pour le groupe proprieraire
		printf("Ecriture, Exécution\n");
	} else if ((info.st_mode & S_IXGRP ) && (info.st_mode & S_ISGID)) {
		printf("Exécution, [SGID]\n");
	} else if (info.st_mode & S_ISGID) {
		printf("[SGID]\n");
	} else if (info.st_mode & S_IRGRP) {
		printf("Lecture\n");
    } else if (info.st_mode & S_IWGRP) {
		printf("Ecriture\n");
	} else if (info.st_mode & S_IXGRP) {
		printf("Exécution\n");
	} else {
		printf("Pas de permissions\n");
	}


	printf("Droits Autres : ");
	if ((info.st_mode & S_IROTH ) && (info.st_mode & S_IWOTH) && (info.st_mode & S_ISVTX) && (info.st_mode & S_IXOTH)) { 
		printf("Lecture, Ecriture, Exécution, [STICKY]\n\n\n\n");
	} else if ((info.st_mode & S_IROTH ) && (info.st_mode & S_IWOTH) && (info.st_mode & S_ISVTX)) {
		printf("Lecture, Ecriture, [STICKY]\n\n\n\n");
	} else if (((info.st_mode & S_IROTH ) && (info.st_mode & S_IWOTH) && (info.st_mode & S_IXOTH))) {
		printf("Lecture, Ecriture, Exécution\n\n\n\n");	
	} else if ((info.st_mode & S_IROTH ) && (info.st_mode & S_IWOTH)) {
		printf("Lecture, Ecriture\n\n\n\n");
	} else if ((info.st_mode & S_IROTH ) && (info.st_mode & S_ISVTX)) {
		printf("Lecture, [STICKY]\n\n\n\n");
 	} else if ((info.st_mode & S_IROTH ) && (info.st_mode & S_IXOTH)) {
		printf("Lecture, Exécution\n\n\n\n");	
	} else if ((info.st_mode & S_IWOTH ) && (info.st_mode & S_ISVTX)) {
		printf("Ecriture, [STICKY]\n\n\n\n");
	} else if ((info.st_mode & S_IWOTH) && (info.st_mode & S_IXOTH)) {		//Meme chose pour les autres
		printf("Ecriture, Exécution\n\n\n\n");
	} else if ((info.st_mode & S_IXOTH ) && (info.st_mode & S_ISVTX)) {
		printf("Exécution, [STICKY]\n");
	} else if (info.st_mode & S_ISVTX) {
		printf("[STICKY]");
	} else if (info.st_mode & S_IROTH) {
		printf("Lecture\n\n\n\n");
    } else if (info.st_mode & S_IWOTH) {
		printf("Ecriture\n\n\n\n");
	} else if (info.st_mode & S_IXOTH) {
		printf("Exécution\n\n\n\n");
	} else {
		printf("Pas de permissions\n\n\n\n");
	}
	file = NULL;			// reinitialise ces variables
    repertoire = NULL;
    return 0;
}
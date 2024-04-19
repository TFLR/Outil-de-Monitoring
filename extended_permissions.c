#include "file_permissions.h"


void ajoutepermsetendue(char *fichier, int addidetendue) {

	struct stat infofichier;
	char *copyfichier = malloc(strlen(fichier) + 1);
	strcpy(copyfichier, fichier);

	if (stat(fichier, &infofichier) != 0) {
		perror("Erreur lors de la récupération des permissions");
		free(copyfichier);
		return;
	}

	mode_t newpermsetendue = infofichier.st_mode;

	if (addidetendue == 4) {
		newpermsetendue |= S_ISUID;
	} else if (addidetendue == 5){
		newpermsetendue |= S_ISGID;
	} else if (addidetendue == 6){
		newpermsetendue |= S_ISVTX;
	}
		chmod(fichier, newpermsetendue);
		proprietefile(copyfichier);
		free(copyfichier);
		return;
}

void supprimepermsetendue(char *fichier, int addidetendue) {

	struct stat infofichier;
	char *copyfichier = malloc(strlen(fichier) + 1);
	strcpy(copyfichier, fichier);

	if (stat(fichier, &infofichier) != 0) {
		perror("Erreur lors de la récupération des permissions");
		free(copyfichier);
		return;
	}
	mode_t newpermsetendue = infofichier.st_mode;

	if (addidetendue == 4) {
		newpermsetendue &= ~S_ISUID;
	} else if (addidetendue == 5){
		newpermsetendue &= ~S_ISGID;
	} else if (addidetendue == 6){
		newpermsetendue &= ~S_ISVTX;
	}
		chmod(fichier, newpermsetendue);
		proprietefile(copyfichier);
		free(copyfichier);
		return;
}

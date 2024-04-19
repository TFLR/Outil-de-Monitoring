#include "file_permissions.h"

void ajouteperms(char *fichier, int addid){
	
	printf("\nfichier: %s", fichier);
	char permission[5];
	struct stat infofichier;
	stat(fichier, &infofichier);
	char *copyfichier = malloc(strlen(fichier) + 1);

	//addid 1 = utilisateur propriétaire
	//addid 2 = groupe propriétaire
	//addid 3 = autres
	
	strcpy(copyfichier, fichier);

	printf("\n\nQuelle permissions voulez-vous ajouter a ce fichier ?\n (R) pour Lecture, (W) pour Ecriture, (X) pour Execution\n\n");
	printf("Exemples:\n'RW' pour ajouter les droits lecture et écriture\n'x' pour ajouter le droit d'exécution\n\n (r / w / x) > ");
	scanf(" %s", permission);

    printf("%s", permission);


	mode_t newperms = infofichier.st_mode;

	while (1) {
		switch(addid){
		case 1:
			
			// Partie utilisateur propriétaire
			for (int i = 0; permission[i] != '\0'; i++) {
				permission[i] = tolower(permission[i]);
	    		

	    		if (permission[i] == 'r') {
				    newperms |= S_IRUSR;
				} else if (permission[i] == 'w') {
				    newperms |= S_IWUSR;
				} else if (permission[i] == 'x') {
				    newperms |= S_IXUSR;
				} 		
			}
			chmod(fichier, newperms);
			proprietefile(copyfichier);
			free(copyfichier);
			return;

		case 2:
			// Partie groupe propriétaire
			for (int i = 0; permission[i] != '\0'; i++) {
				permission[i] = tolower(permission[i]);
	    		

	    		if (permission[i] == 'r') {
				    newperms |= S_IRGRP;
				} else if (permission[i] == 'w') {
				    newperms |= S_IWGRP;
				} else if (permission[i] == 'x') {
				    newperms |= S_IXGRP;
				} 			
			}		
			chmod(fichier, newperms);
			proprietefile(copyfichier);
			free(copyfichier);
			return;

		case 3:
			// Partie autres
			for (int i = 0; permission[i] != '\0'; i++) {
				permission[i] = tolower(permission[i]);
	    		

	    		if (permission[i] == 'r') {
				    newperms |= S_IROTH;
				} else if (permission[i] == 'w') {
				    newperms |= S_IWOTH;
				} else if (permission[i] == 'x') {
				    newperms |= S_IXOTH;
				}
			}

			chmod(fichier, newperms);
			proprietefile(copyfichier);
			free(copyfichier);
			return;
		}
	}
	return;
}


void supprimeperms(char *fichier, int suppid){
	
	printf("\nfichier: %s", fichier);
	char permission[5];
	struct stat infofichier;
	stat(fichier, &infofichier);
	char *copyfichiersupp = malloc(strlen(fichier) + 1);

	//suppid 1 = utilisateur propriétaire
	//suppid 2 = groupe propriétaire
	//suppid 3 = autres
	
	strcpy(copyfichiersupp, fichier);

	printf("\n\nQuelle permissions voulez-vous supprimer a ce fichier ?\n (R) pour Lecture, (W) pour Ecriture, (X) pour Execution\n\n");
	printf("Exemples:\n'RW' pour supprimer les droits lecture et écriture\n'x' pour supprimer le droit d'exécution\n\n> ");
	scanf("%s", permission);

    //printf("%s", permission);
    
    // Vider le tampon (si un utilisateur envoie un long input seul le 1er caractère est pris en compte)
	int c;
	while ((c = getchar()) != '\n' && c != EOF) {}

	mode_t rmvperms = infofichier.st_mode;

	switch(suppid){
	case 1:		
		// Partie utilisateur propriétaire
		for (int i = 0; permission[i] != '\0'; i++) {
			permission[i] = tolower(permission[i]);   		

    		if (permission[i] == 'r') {
			    rmvperms &= ~S_IRUSR;
			} else if (permission[i] == 'w') {
			    rmvperms &= ~S_IWUSR;
			} else if (permission[i] == 'x') {
			    rmvperms &= ~S_IXUSR;
			}
		}
		chmod(fichier, rmvperms);
		proprietefile(copyfichiersupp);
		free(copyfichiersupp);
		break;

	case 2:
		// Partie groupe propriétaire
		for (int i = 0; permission[i] != '\0'; i++) {
			permission[i] = tolower(permission[i]);
    		

    		if (permission[i] == 'r') {
			    rmvperms &= ~S_IRGRP;
			} else if (permission[i] == 'w') {
			    rmvperms &= ~S_IWGRP;
			} else if (permission[i] == 'x') {
			    rmvperms &= ~S_IXGRP;
			}
		}
		chmod(fichier, rmvperms);
		proprietefile(copyfichiersupp);
		free(copyfichiersupp);
		break;

	case 3:
		// Partie autres
		for (int i = 0; permission[i] != '\0'; i++) {
			permission[i] = tolower(permission[i]);
    		

    		if (permission[i] == 'r') {
			    rmvperms &= ~S_IROTH;
			} else if (permission[i] == 'w') {
			    rmvperms &= ~S_IWOTH;
			} else if (permission[i] == 'x') {
			    rmvperms &= ~S_IXOTH;
			}
		}
		chmod(fichier, rmvperms);
		proprietefile(copyfichiersupp);
		free(copyfichiersupp);
		break;
	}
	return;
}


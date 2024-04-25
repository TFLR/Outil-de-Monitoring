#include "file_permissions.h"

int gestiondroit(){
	char nomfichier[512];
	char nomfichier2[512];	// Pour éviter que la fonction proprietefile() interfere avec le chemin absolue a cause de la fonction dirname() 
	char nomfichier3[512];	// Entrée manuelle d'un chemin vers un fichier
	int racinedir;	// Pour vérifier si le fichier sélectionné est un répertoire racine (dans "/")
	int racinedir2; // Pour entrée manuelle du chemin vers un fichier

	FILE* fichier_config = fopen("./config.txt", "r"); // Ouvre le fichier "config.txt" en lecture
	printf("Lecture du fichier de configuration...\n\n");

	while (1){
		
		if (fichier_config == NULL) {
    		printf("Erreur lors de l'ouverture du fichier config.txt.\n");
   			return 1;
		}

		while (fgets(nomfichier, sizeof(nomfichier), fichier_config) != NULL) {

    		char* chemin = strtok(nomfichier, " "); //s'arrete au moment où le programme trouve un espace et ignore le reste de la ligne
    		printf("Fichier sélectionné : %s\n", nomfichier);


			// Vérifie si le fichier ou repertoire entrée existe ou pas
			if (access(chemin, F_OK) != -1) {
        		//printf("Le fichier ou répertoire existe.\n");
        		strcpy(nomfichier2,chemin);

				//affiche les informations du fichier
				racinedir = proprietefile(nomfichier2);
				
				if (racinedir == 1){
					printf("\nLe fichier sélectionné est un répertoire système. Il n'est pas possible de modifier les droits d'accès à ce répertoire.\n");
					printf("Passage au fichier suivant..\n");
					sleep(3);
					system("clear");
					continue;
				} else {
					while (1){
						printf("Voulez-vous modifier les permissions d'accès au fichier: %s ? (O/N)\n", nomfichier);
						char modifchoix[5];
						scanf(" %s", modifchoix);
						getchar(); // Empeche le caractère "\n" d'interférer avec l'itération suivante

						if (modifchoix[0] == 'O' || modifchoix[0] == 'o') {
							menu(chemin);
							break;
						} else if (modifchoix[0] == 'N' || modifchoix[0] == 'n') {
							printf("\nPassage au fichier suivant.\n");
							system("clear");
							break;
						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles [ ! ]\n");
						}
					}
				}	   		
    		} else {
    			printf("\n[ ! ] Ce fichier ou répertoire n'existe pas. [ ! ]\n\n");
    		}
    	}

    	printf("\nPlus aucune entrée dans le fichier de configuration.\n");	
    	// Demander à l'utilisateur s'il souhaite modifier les accès d'un autre fichier
        char reponse[2];
        
		
        while(1){

			printf("\nIl y a t-il un autre fichier a modifier (à entrer manuellement ensuite) ? (O/N)\nO = Oui\nN = Non (Quitte le programme de gestion de droits)\n");

        	printf("> ");
        	scanf(" %s", reponse);

	        if ((strcmp(reponse, "n") == 0) || (strcmp(reponse, "N") == 0)) {
	        	printf("\nAu revoir !\n");
	            return 0;

			} else if ((strcmp(reponse, "o") == 0) || (strcmp(reponse, "O") == 0)) {
				break;
			} else {
				system("clear");
				printf("[ ! ] Entrée incorrecte [ ! ]\n\n");
			}
		}

		while(1){
			printf("Entrez le nom du fichier(chemin absolue):\n> ");
			scanf("%s", nomfichier3);

			if (access(nomfichier3, F_OK) != -1) {


	        	strcpy(nomfichier2,nomfichier3);

				//affiche les informations du fichier
				racinedir2 = proprietefile(nomfichier2);
				if (racinedir2 == 1){
						printf("\nLe fichier sélectionné est un répertoire racine. Il n'est pas possible de modifier les droits d'accès à ce répertoire.\n");
						sleep(3);
						system("clear");
						break;
				} else {
				menu(nomfichier3);
				break;
	    		}
	    	} else {
	    		printf("\n[ ! ] Le fichier ou répertoire saisie n'existe pas [ ! ]\n");
	    	}
    	}    	
	}
	fclose(fichier_config);
	return 0;
}
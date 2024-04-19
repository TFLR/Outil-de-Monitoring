#include "file_permissions.h"

int gestiondroit(){
	char nomfichier[100];
	char nomfichier2[100];	// Pour éviter que la fonction proprietefile() interfere avec le chemin absolue a cause de la fonction dirname() 
	char nomfichier3[100];	// Entrée manuelle

	FILE* fichier_config = fopen("../config.txt", "r"); // Ouvre le fichier "config.txt" en lecture
	printf("Lecture du fichier de configuration...\n\n");

	while (1){
		
		if (fichier_config == NULL) {
    		printf("Erreur lors de l'ouverture du fichier config.txt.\n");
   			return 1;
		}

		while (fgets(nomfichier, sizeof(nomfichier), fichier_config) != NULL) {
    		nomfichier[strcspn(nomfichier, "\n")] = '\0'; // Supprime le saut de ligne à la fin de la ligne lue


    		printf("Fichier sélectionné : %s\n", nomfichier);


			// Vérifie si le fichier ou repertoire entrée existe ou pas
			if (access(nomfichier, F_OK) != -1) {
        		//printf("Le fichier ou répertoire existe.\n");
        		strcpy(nomfichier2,nomfichier);

				//affiche les informations du fichier
				proprietefile(nomfichier2);
				

				while (1){
					printf("Voulez-vous modifier les permissions d'accès au fichier: %s ? (O/N)\n", nomfichier);
					char modifchoix[5];
					scanf(" %s", modifchoix);
					getchar(); // Empeche le caractère "\n" d'interférer avec l'itération suivante

					if (modifchoix[0] == 'O' || modifchoix[0] == 'o') {
						menu(nomfichier);
						break;
					} else if (modifchoix[0] == 'N' || modifchoix[0] == 'n') {
						printf("\nPassage au fichier suivant.\n");
						system("clear");
						break;
					} else {
						printf("\n[ ! ] Veuillez choisir une des options disponibles [ ! ]\n");
					}
				}	   		
    		} else {
    			printf("\n[ ! ] Ce fichier ou répertoire n'existe pas. [ ! ]\n\n");
    		}
    	}

    	printf("\nPlus aucune entrée dans le fichier de configuration.\n");	
    	// Demander à l'utilisateur s'il souhaite modifier les accès d'un autre fichier
        char reponse[2];
        
		printf("\nIl y a t-il un autre fichier a modifier (à entrer manuellement ensuite) ? (O/N)\nO = Oui\nN = Non (Quitte le programme de gestion de droits)\n");

        while(1){

        	printf("> ");
        	scanf(" %s", reponse);

	        if ((strcmp(reponse, "n") == 0) || (strcmp(reponse, "N") == 0)) {
	        	printf("\nAu revoir !\n");
	            return 0;

			} else if ((strcmp(reponse, "o") == 0) || (strcmp(reponse, "O") == 0)) {
				break;
			} else {
				printf("[ ! ] Entrée incorrecte [ ! ]\n\n");
			}
		}

		printf("Entrez le nom du fichier(chemin absolue):\n> ");
		scanf("%s", nomfichier3);

		if (access(nomfichier3, F_OK) != -1) {

        	strcpy(nomfichier2,nomfichier3);

			//affiche les informations du fichier
			proprietefile(nomfichier2);
			
			menu(nomfichier3);
    	} else {
    		printf("\n[ ! ] Le fichier ou répertoire saisie n'existe pas [ ! ]\n");
    	}    	
	}
	fclose(fichier_config);
	return 0;
}

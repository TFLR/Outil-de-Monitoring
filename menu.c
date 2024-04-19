#include "file_permissions.h"

//2e menu, pour changer (supprimer ou ajouter) les permissions d'accès
int menuperms(){

	int options;
	int changepermission;
	char input[10];
	char input2[10];

	while (1) {

		printf("\n\n\t\tMENU PERMISSIONS STANDARD\n");
		printf("\n\n1. Changer les permissions de l'utilisateur propriétaire\n");
		printf("2. Changer les permissions du groupe propriétaire\n");
		printf("3. Changer les permissions des autres\n");
		printf("4. Retour au menu principal\n\nVotre choix > ");
		
		scanf("%s", input);

		// Vérifie si l'entrée utilisateur est un nombre
		if (isdigit(input[0])) {
			options = atoi(input);

			switch(options){
				// Utilisateur propriétaire
			case 1:		
				while (1) {

					printf("\nPour l'utilisateur propriétaire: (Tapez 1 pour ajouter ou 2 pour supprimer une ou plusieurs permissions)");
					printf("\n\n1. Ajouter une ou des permission(s)\n");
					printf("2. Supprimer une ou des permission(s)\n\nVotre choix > ");


					scanf("%s", input2);
					if (isdigit(input2[0])) {
						changepermission = atoi(input2);
						if (changepermission == 1){
							return 11;

						} else if (changepermission == 2) {
							return 12;

						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
						}
					} else {
						printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
					}
				}
				// Groupe propriétaire
			case 2:
				while (1) {
					printf("\nPour le groupe propriétaire: (Tapez 1 pour ajouter ou 2 pour supprimer une ou plusieurs permissions)");
					printf("\n\n1. Ajouter une ou des permission(s)\n");
					printf("2. Supprimer une ou des permission(s)\n\nVotre choix > ");

					scanf("%s", input2);
					if (isdigit(input2[0])) {
						changepermission = atoi(input2);
						if (changepermission == 1){
							return 21;

						} else if (changepermission == 2) {
							return 22;

						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
						}
					} else {
						printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
					}
				}
				// Autres
			case 3:
				while (1) {
					printf("\nPour les autres: (Tapez 1 pour ajouter ou 2 pour supprimer une ou plusieurs permissions)");
					printf("\n\n1. Ajouter une ou des permission(s)\n");
					printf("2. Supprimer une ou des permission(s)\n\nVotre choix > ");

					scanf("%s", input2);
					if (isdigit(input2[0])) {
						changepermission = atoi(input2);
						if (changepermission == 1){
							return 31;

						} else if (changepermission == 2) {
							return 32;

						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
						}
					} else {
						printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
					}
				}
			case 4:
				return 0;
			default:

				printf("\n[ ! ] Veuillez choisir une des options ci-dessous [ ! ]\n");

			}
		} else {
			printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
		}
	}
	return 0;
}


int menupermsetendue(){
	int optionsetendue;
	int changepermissionetendue;
	char input[10];
	char input2[10];

	while (1) {

		printf("\n\n\t\tMENU PERMISSIONS ETENDUES\n");
		printf("\n\n1. Ajouter une permission étendues\n");
		printf("2. Supprimer une permission étendues\n");
		printf("3. Retour au menu principal\n\nVotre choix > ");
		
		scanf("%s", input);

		// Vérifie si l'entrée utilisateur est un nombre
		if (isdigit(input[0])) {
			optionsetendue = atoi(input);
			switch(optionsetendue){
				// Ajouter permissions étendues
			case 1:		
				while (1) {

					printf("\n\nQuelle permissions étendues voulez-vous ajouter ?\n");
					printf("1. Set owner User ID (SUID)\n2. Set owner Group ID (GUID)\n3. Sticky Bit\n4. Quitter\n\nVotre choix > ");

					scanf("%s", input2);
					if (isdigit(input2[0])) {
						changepermissionetendue = atoi(input2);
						if (changepermissionetendue == 1){
							return 41;
						} else if (changepermissionetendue == 2) {
							return 51;
						} else if (changepermissionetendue == 3) {
							return 61;
						} else if (changepermissionetendue == 4) {
							return 0;
						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
						}
					} else {
						printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
					}
				}
				// Supprimer permissions étendues
			case 2:
				while (1) {
					printf("\n\nQuelle permissions étendues voulez-vous supprimer ?\n");
					printf("1. Set owner User ID (SUID)\n2. Set owner Group ID (GUID)\n3. Sticky Bit\n4. Quitter\n\nVotre choix > ");

					scanf("%s", input2);
					if (isdigit(input2[0])) {
						changepermissionetendue = atoi(input2);
						if (changepermissionetendue == 1){
							return 42;
						} else if (changepermissionetendue == 2) {
							return 52;
						} else if (changepermissionetendue == 3) {
							return 62;
						} else if (changepermissionetendue == 4) {
							return 0;
						} else {
							printf("\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
						}
					} else {
						printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
					}
				}
				// Quitter
			case 3:
				return 0;
			default:
				printf("\n[ ! ] Veuillez choisir une des options ci-dessous [ ! ]\n");
			}
		} else {
			printf("\n[ ! ] Veuillez entrer un entier [ ! ]\n");
		}
	}
	return 0;
}


int menu(char *fichier){
	char *choix = malloc(100);
	int permchoix;
	int permetenduechoix;

	while (1) {

		printf("\n\n\t\tMENU PRINCIPAL\n");
		//printf("\nAvant ajouteperms choix: %s", choix);
		printf("\n\n1. Changer les permissions d'accès standard de ce fichier(Lecture, écriture et éxecution)\n2. Changer les permissions d'accès étendues de ce fichier(SUID, SGID et Sticky BIT)\n3. Quitter le gestionnaire d'accès pour ce fichier\n\nVotre choix > ");
		
		scanf("%s2",choix);
		printf("\n");
		if (strcmp(choix, "1") == 0){
			permchoix=menuperms();

			if (permchoix == 11) {					//utilisateur propriétaire
				ajouteperms(fichier,1);
				//printf("Après ajouteperms choix: %d", choix[0]);
				free(choix);
			} else if (permchoix == 12) {
				supprimeperms(fichier,1);
			} else if (permchoix == 21) {			//groupe propriétaire
				ajouteperms(fichier,2);
			} else if (permchoix == 22) {
				supprimeperms(fichier,2);
			} else if (permchoix == 31) {			//autres
				ajouteperms(fichier,3);
			} else if (permchoix == 32) {
				supprimeperms(fichier,3);
			}

		} else if (strcmp(choix,"2") == 0) {
			permetenduechoix=menupermsetendue(fichier);
			if (permetenduechoix == 41) {					//SUID
				ajoutepermsetendue(fichier,4);
			} else if (permetenduechoix == 42) {
				supprimepermsetendue(fichier,4);
			} else if (permetenduechoix == 51) {			//GUID
				ajoutepermsetendue(fichier,5);
			} else if (permetenduechoix == 52) {
				supprimepermsetendue(fichier,5);
			} else if (permetenduechoix == 61) {			//Sticky bit
				ajoutepermsetendue(fichier,6);
			} else if (permetenduechoix == 62) {
				supprimepermsetendue(fichier,6);
			}
		} else if (strcmp(choix,"3") == 0) {
			free(choix);
			return 0;		
		} else {
			printf("\n\t###################################\n\n[ ! ] Veuillez choisir une des options disponibles ci-dessous [ ! ]\n");
		}
	}
	return 0;
}

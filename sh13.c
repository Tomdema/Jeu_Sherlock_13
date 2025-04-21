#include <SDL2/SDL.h>        // Librairie pour la gestion graphique
#include <SDL2/SDL_image.h>   // Librairie pour charger des images
#include <SDL2/SDL_ttf.h>     // Librairie pour afficher du texte avec des polices
#include <pthread.h>     // Pour utiliser les threads
#include <unistd.h>      // Pour sleep, close...
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// === Variables globales ===
pthread_t thread_serveur_tcp_id;         // Thread pour le serveur TCP (client en écoute)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour les accès concurrents
char gbuffer[256];                       // Buffer pour messages reçus du serveur
char gServerIpAddress[256];              // Adresse IP du serveur principal
int gServerPort;                         // Port du serveur principal
char gClientIpAddress[256];              // Adresse IP du client (pour que le serveur l'appelle)
int gClientPort;                         // Port d'écoute du client
char gName[256];                         // Nom du joueur
char gNames[4][256];                     // Noms des 4 joueurs
int gId;                                 // ID attribué par le serveur
int joueurSel;                           // Joueur sélectionné
int objetSel;                            // Objet sélectionné
int guiltSel;                            // Personnage suspecté
int guiltGuess[13];                      // Suppositions sur les 13 personnages
int tableCartes[4][8];                   // Tableau de référence sur les objets détenus par joueurs
int b[3];                                // Cartes reçues par le joueur
int goEnabled;                           // Activation du bouton "GO"
int connectEnabled;                      // Activation du bouton "Connect"

char *nbobjets[] = {"5", "5", "5", "5", "4", "3", "3", "3"}; // Nb d'occurrences pour chaque objet
char *nbnoms[] = {"Sebastian Moran", "irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};

volatile int synchro;  // Synchronisation entre thread TCP et boucle principale

// Thread qui joue le rôle de serveur TCP pour recevoir des messages du serveur principal
void *fn_serveur_tcp(void *arg)
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("sockfd error\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = gClientPort;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("bind error\n");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            printf("accept error\n");
            exit(1);
        }

        bzero(gbuffer, 256);
        n = read(newsockfd, gbuffer, 255);
        if (n < 0) {
            printf("read error\n");
            exit(1);
        }

        // Signale qu'un message a été reçu, la boucle principale peut le traiter
        synchro = 1;

        // Attend que la boucle principale remette synchro à 0
        while (synchro);
    }
}

// Fonction pour envoyer un message au serveur principal
void sendMessageToServer(char *ipAddress, int portno, char *mess)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char sendbuffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(ipAddress);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(1);
    }

    sprintf(sendbuffer, "%s\n", mess);
    n = write(sockfd, sendbuffer, strlen(sendbuffer));

    close(sockfd);
}


// === Suite du main : initialisation et boucle principale ===
int main(int argc, char ** argv)
{
    int ret;
    int i, j;

    int quit = 0;                      // Variable pour quitter la boucle principale
    SDL_Event event;                  // Gestionnaire d'événements SDL
    int mx, my;                       // Coordonnées de la souris
    char sendBuffer[256];            // Buffer pour les messages envoyés au serveur
    char lname[256];
    int id;

    // Vérifie qu'on a bien 6 arguments (IP serveur, port serveur, IP client, port client, nom joueur)
    if (argc < 6) {
        printf("<app> <Main server ip address> <Main server port> <Client ip address> <Client port> <player name>\n");
        exit(1);
    }

    // Récupération des arguments
    strcpy(gServerIpAddress, argv[1]);
    gServerPort = atoi(argv[2]);
    strcpy(gClientIpAddress, argv[3]);
    gClientPort = atoi(argv[4]);
    strcpy(gName, argv[5]);

    // Initialisation de SDL et SDL_ttf
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // Création de la fenêtre du jeu
    SDL_Window * window = SDL_CreateWindow("SDL2 SH13",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);

    // Création du renderer (permet de dessiner dans la fenêtre)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    // Chargement des images pour les cartes et objets
    SDL_Surface *deck[13], *objet[8], *gobutton, *connectbutton;

    // Chargement des 13 cartes personnages
    for (i = 0; i < 13; i++) {
        char filename[64];
        sprintf(filename, "SH13_%d.png", i);
        deck[i] = IMG_Load(filename);
    }

    // Chargement des images des objets (pipe, ampoule, etc.)
    objet[0] = IMG_Load("SH13_pipe_120x120.png");
    objet[1] = IMG_Load("SH13_ampoule_120x120.png");
    objet[2] = IMG_Load("SH13_poing_120x120.png");
    objet[3] = IMG_Load("SH13_couronne_120x120.png");
    objet[4] = IMG_Load("SH13_carnet_120x120.png");
    objet[5] = IMG_Load("SH13_collier_120x120.png");
    objet[6] = IMG_Load("SH13_oeil_120x120.png");
    objet[7] = IMG_Load("SH13_crane_120x120.png");

    // Boutons pour les actions principales
    gobutton = IMG_Load("gobutton.png");
    connectbutton = IMG_Load("connectbutton.png");

    // Initialisation des noms des joueurs à "-" (aucun nom pour l'instant)
    for (i = 0; i < 4; i++) {
        strcpy(gNames[i], "-");
    }

    // Initialisation des sélections à -1 (aucune sélection)
    joueurSel = -1;
    objetSel = -1;
    guiltSel = -1;

    // Initialisation des 3 cartes du joueur à -1 (pas encore reçues)
    b[0] = -1; b[1] = -1; b[2] = -1;

    // Réinitialise toutes les suppositions de culpabilité
    for (i = 0; i < 13; i++)
        guiltGuess[i] = 0;

    // Réinitialise la table des objets détenus à -1 (inconnu)
    for (i = 0; i < 4; i++)
        for (j = 0; j < 8; j++)
            tableCartes[i][j] = -1;

    // Le bouton "GO" est désactivé par défaut, mais "Connect" est activé
    goEnabled = 0;
    connectEnabled = 1;

    // Création des textures SDL à partir des surfaces
    SDL_Texture *texture_deck[13], *texture_gobutton, *texture_connectbutton, *texture_objet[8];
    for (i = 0; i < 13; i++)
        texture_deck[i] = SDL_CreateTextureFromSurface(renderer, deck[i]);
    for (i = 0; i < 8; i++)
        texture_objet[i] = SDL_CreateTextureFromSurface(renderer, objet[i]);
    texture_gobutton = SDL_CreateTextureFromSurface(renderer, gobutton);
    texture_connectbutton = SDL_CreateTextureFromSurface(renderer, connectbutton);

    // Chargement de la police pour l'affichage de texte
    TTF_Font* Sans = TTF_OpenFont("sans.ttf", 15);
    printf("Sans=%p\n", Sans);

    // Lancement du thread serveur TCP qui écoute les messages entrants
    printf("Creation du thread serveur tcp !\n");
    synchro = 0;
    ret = pthread_create(&thread_serveur_tcp_id, NULL, fn_serveur_tcp, NULL);

    // === Boucle principale ===
    while (!quit)
    {
        if (SDL_PollEvent(&event)) // Récupère un événement SDL (clic, fermeture...)
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    quit = 1; // Quitte la boucle si l'utilisateur ferme la fenêtre
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    SDL_GetMouseState(&mx, &my); // Récupère la position de la souris

                    // Si clic sur le bouton "Connect"
                    if ((mx < 200) && (my < 50) && (connectEnabled == 1))
                    {
                        sprintf(sendBuffer, "C %s %d %s", gClientIpAddress, gClientPort, gName);
                        sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer); // Envoie la demande de connexion
                        connectEnabled = 0;
                    }
                    // Si clic sur un joueur dans la zone joueurs
                    else if ((mx >= 0) && (mx < 200) && (my >= 90) && (my < 330))
                    {
                        joueurSel = (my - 90) / 60;
                        guiltSel = -1;
                    }
                    // Si clic sur un objet
                    else if ((mx >= 200) && (mx < 680) && (my >= 0) && (my < 90))
                    {
                        objetSel = (mx - 200) / 60;
                        guiltSel = -1;
                    }
                    // Si clic sur un nom de personnage (accusation)
                    else if ((mx >= 100) && (mx < 250) && (my >= 350) && (my < 740))
                    {
                        joueurSel = -1;
                        objetSel = -1;
                        guiltSel = (my - 350) / 30;
                    }
                    // Si clic sur une case de supposition
                    else if ((mx >= 250) && (mx < 300) && (my >= 350) && (my < 740))
                    {
                        int ind = (my - 350) / 30;
                        guiltGuess[ind] = 1 - guiltGuess[ind]; // Change l'état (coché/décoché)
                    }
                    // Si clic sur le bouton "GO"
                    else if ((mx >= 500) && (mx < 700) && (my >= 350) && (my < 450) && (goEnabled == 1))
                    {
                        printf("go! joueur=%d objet=%d guilt=%d\n", joueurSel, objetSel, guiltSel);
                        if (guiltSel != -1)
                        {
                            sprintf(sendBuffer, "G %d %d", gId, guiltSel);
                            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
                        }
                        else if ((objetSel != -1) && (joueurSel == -1))
                        {
                            sprintf(sendBuffer, "O %d %d", gId, objetSel);
                            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
                        }
                        else if ((objetSel != -1) && (joueurSel != -1))
                        {
                            sprintf(sendBuffer, "S %d %d %d", gId, joueurSel, objetSel);
                            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
                        }
                    }
                    else // Clic ailleurs : reset des sélections
                    {
                        joueurSel = -1;
                        objetSel = -1;
                        guiltSel = -1;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    SDL_GetMouseState(&mx, &my); // Pour gérer les effets visuels si besoin
                    break;
            }
        }


        // === Traitement des messages reçus du serveur (thread TCP) ===
		if (synchro == 1){
    		// Affiche le message reçu (pour debug)
    		printf("consomme |%s|\n", gbuffer);

    	// Lecture du type de message via le 1er caractère
    		switch (gbuffer[0])
    		{
        		// 'I' : serveur envoie l'identifiant du joueur
        		case 'I':
            		sscanf(gbuffer, "I %d", &gId);
            		break;

        		// 'L' : serveur envoie la liste des noms des 4 joueurs
        		case 'L':
            		sscanf(gbuffer, "L %s %s %s %s", gNames[0], gNames[1], gNames[2], gNames[3]);
            		break;

        		// 'D' : joueur reçoit ses 3 cartes secrètes (indices dans le deck)
        		case 'D':
            		sscanf(gbuffer, "D %d %d %d", &b[0], &b[1], &b[2]);
            		break;

        		// 'M' : le serveur annonce qui est le joueur courant (tour en cours)
        		case 'M': {
            		int joueurCourant;
            		sscanf(gbuffer, "M %d", &joueurCourant);
            		if (gId == joueurCourant)
                		goEnabled = 1; // active le bouton GO si c'est notre tour
            		else
                		goEnabled = 0; // sinon, on ne peut pas jouer
            		break;
        		}

        		// 'V' : valeur d'un objet dans la tableCartes révélée par le serveur
        		case 'V': {
            		int i, j, val;
            		sscanf(gbuffer, "V %d %d %d", &i, &j, &val);
            		if (val == 100)
                		tableCartes[i][j] = 100; // 100 signifie présence certaine de l’objet
            		else
                		tableCartes[i][j] = val;  // valeur binaire 0 ou 1
            		break;
        		}
    		}
    		// Une fois traité, remet la synchro à 0 pour écouter le prochain message
    		synchro = 0;
		}

// === Rendu graphique initial : fond et zone grille ===
		SDL_Rect dstrect_grille = {512 - 250, 10, 500, 350}; // Position de la grille d'enquête
		SDL_Rect dstrect_image = {0, 0, 500, 330};            // Image principale (non utilisée ici)
		SDL_Rect dstrect_image1 = {0, 340, 250, 165};         // Zone secondaire (non utilisée ici)

		// Remplit le fond de la fenêtre avec une couleur rose pâle
		SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
		SDL_Rect rect = {0, 0, 1024, 768}; 
		SDL_RenderFillRect(renderer, &rect);

		// Si un joueur est sélectionné, on surligne sa ligne
		if (joueurSel != -1)
		{
   			SDL_SetRenderDrawColor(renderer, 255, 180, 180, 255);
    		SDL_Rect rect1 = {0, 90 + joueurSel * 60, 200, 60}; 
    		SDL_RenderFillRect(renderer, &rect1);
		}

		// Si un objet est sélectionné, on surligne sa colonne
		if (objetSel != -1)
		{
    		SDL_SetRenderDrawColor(renderer, 180, 255, 180, 255);
    		SDL_Rect rect1 = {200 + objetSel * 60, 0, 60, 90}; 
    		SDL_RenderFillRect(renderer, &rect1);
		}

		// Si un personnage est suspecté, on surligne sa ligne
		if (guiltSel != -1)
		{
    		SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
    		SDL_Rect rect1 = {100, 350 + guiltSel * 30, 150, 30}; 
    		SDL_RenderFillRect(renderer, &rect1);
		}

		// === Affichage des icônes des objets en haut de la grille ===
		{
    SDL_Rect dstrect_pipe     = {210, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    SDL_Rect dstrect_ampoule  = {270, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    SDL_Rect dstrect_poing    = {330, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    SDL_Rect dstrect_couronne = {390, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    SDL_Rect dstrect_carnet   = {450, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
    SDL_Rect dstrect_collier  = {510, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
    SDL_Rect dstrect_oeil     = {570, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
    SDL_Rect dstrect_crane    = {630, 10, 40, 40}; SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
		}

// === Affichage du nombre d'occurrences par objet ===
SDL_Color col1 = {0, 0, 0}; // couleur noire pour le texte
for (i = 0; i < 8; i++)
{
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbobjets[i], col1);
    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect Message_rect;
    Message_rect.x = 230 + i * 60;
    Message_rect.y = 50;
    Message_rect.w = surfaceMessage->w;
    Message_rect.h = surfaceMessage->h;

    SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
    SDL_DestroyTexture(Message);
    SDL_FreeSurface(surfaceMessage);
}

// === Affichage des noms des 13 personnages à gauche ===
for (i = 0; i < 13; i++)
{
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbnoms[i], col1);
    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect Message_rect;
    Message_rect.x = 105;
    Message_rect.y = 350 + i * 30;
    Message_rect.w = surfaceMessage->w;
    Message_rect.h = surfaceMessage->h;

    SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
    SDL_DestroyTexture(Message);
    SDL_FreeSurface(surfaceMessage);
}

// === Affichage de la grille tableCartes ===
for (i = 0; i < 4; i++)
    for (j = 0; j < 8; j++)
    {
        if (tableCartes[i][j] != -1)
        {
            char mess[10];
            if (tableCartes[i][j] == 100)
                sprintf(mess, "*"); // étoile pour objet trouvé
            else
                sprintf(mess, "%d", tableCartes[i][j]);

            SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, mess, col1);
            SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

            SDL_Rect Message_rect;
            Message_rect.x = 230 + j * 60;
            Message_rect.y = 110 + i * 60;
            Message_rect.w = surfaceMessage->w;
            Message_rect.h = surfaceMessage->h;

            SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
            SDL_DestroyTexture(Message);
            SDL_FreeSurface(surfaceMessage);
        }
    }
// === Affichage des objets associés à chaque personnage (légende à gauche) ===

// Pour chaque personnage, on affiche les icônes des objets qui lui sont liés.
// Les objets sont représentés par de petites images 30x30 à gauche de leur nom.

// Sebastian Moran : crâne et poing
{ SDL_Rect dstrect = {0, 350, 30, 30}; SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 350, 30, 30}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect); }

// Irene Adler : crâne, ampoule, collier
{ SDL_Rect dstrect = {0, 380, 30, 30}; SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 380, 30, 30}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 380, 30, 30}; SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect); }

// Inspector Lestrade : couronne, œil, carnet
{ SDL_Rect dstrect = {0, 410, 30, 30}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 410, 30, 30}; SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 410, 30, 30}; SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect); }

// Inspector Gregson : couronne, poing, carnet
{ SDL_Rect dstrect = {0, 440, 30, 30}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 440, 30, 30}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 440, 30, 30}; SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect); }

// Inspector Baynes : couronne, ampoule
{ SDL_Rect dstrect = {0, 470, 30, 30}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 470, 30, 30}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect); }

// Inspector Bradstreet : couronne, poing
{ SDL_Rect dstrect = {0, 500, 30, 30}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 500, 30, 30}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect); }

// Inspector Hopkins : couronne, pipe, œil
{ SDL_Rect dstrect = {0, 530, 30, 30}; SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 530, 30, 30}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 530, 30, 30}; SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect); }

// Sherlock Holmes : pipe, ampoule, poing
{ SDL_Rect dstrect = {0, 560, 30, 30}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 560, 30, 30}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 560, 30, 30}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect); }

// John Watson : pipe, œil, poing
{ SDL_Rect dstrect = {0, 590, 30, 30}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 590, 30, 30}; SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 590, 30, 30}; SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect); }

// Mycroft Holmes : pipe, ampoule, carnet
{ SDL_Rect dstrect = {0, 620, 30, 30}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 620, 30, 30}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect); }
{ SDL_Rect dstrect = {60, 620, 30, 30}; SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect); }

// Mrs. Hudson : pipe, collier
{ SDL_Rect dstrect = {0, 650, 30, 30}; SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 650, 30, 30}; SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect); }

// Mary Morstan : carnet, collier
{ SDL_Rect dstrect = {0, 680, 30, 30}; SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 680, 30, 30}; SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect); }

// James Moriarty : crâne, ampoule
{ SDL_Rect dstrect = {0, 710, 30, 30}; SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect); }
{ SDL_Rect dstrect = {30, 710, 30, 30}; SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect); }

// === Affichage des croix de supposition du joueur ===
SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // couleur rouge
for (i = 0; i < 13; i++)
{
    if (guiltGuess[i])
    {
        SDL_RenderDrawLine(renderer, 250, 350 + i * 30, 300, 380 + i * 30);
        SDL_RenderDrawLine(renderer, 250, 380 + i * 30, 300, 350 + i * 30);
    }
}

// === Affichage des lignes de la grille d'enquête (traits noirs) ===
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // noir

// lignes horizontales du tableau objets/joueurs
SDL_RenderDrawLine(renderer, 0, 90, 680, 90);
SDL_RenderDrawLine(renderer, 0, 150, 680, 150);
SDL_RenderDrawLine(renderer, 0, 210, 680, 210);
SDL_RenderDrawLine(renderer, 0, 270, 680, 270);
SDL_RenderDrawLine(renderer, 0, 330, 680, 330);

// colonnes verticales objets
SDL_RenderDrawLine(renderer, 200, 0, 200, 330);
SDL_RenderDrawLine(renderer, 260, 0, 260, 330);
SDL_RenderDrawLine(renderer, 320, 0, 320, 330);
SDL_RenderDrawLine(renderer, 380, 0, 380, 330);
SDL_RenderDrawLine(renderer, 440, 0, 440, 330);
SDL_RenderDrawLine(renderer, 500, 0, 500, 330);
SDL_RenderDrawLine(renderer, 560, 0, 560, 330);
SDL_RenderDrawLine(renderer, 620, 0, 620, 330);
SDL_RenderDrawLine(renderer, 680, 0, 680, 330);

// tableau de suspects (à gauche)
for (i = 0; i < 14; i++)
    SDL_RenderDrawLine(renderer, 0, 350 + i * 30, 300, 350 + i * 30);
SDL_RenderDrawLine(renderer, 100, 350, 100, 740);
SDL_RenderDrawLine(renderer, 250, 350, 250, 740);
SDL_RenderDrawLine(renderer, 300, 350, 300, 740);

// === Affichage des cartes du joueur ===
if (b[0] != -1)
{
    SDL_Rect dstrect = {750, 0, 250, 165};
    SDL_RenderCopy(renderer, texture_deck[b[0]], NULL, &dstrect);
}
if (b[1] != -1)
{
    SDL_Rect dstrect = {750, 200, 250, 165};
    SDL_RenderCopy(renderer, texture_deck[b[1]], NULL, &dstrect);
}
if (b[2] != -1)
{
    SDL_Rect dstrect = {750, 400, 250, 165};
    SDL_RenderCopy(renderer, texture_deck[b[2]], NULL, &dstrect);
}

// === Bouton "GO" pour passer son tour ou répondre (s'affiche uniquement si activé) ===
if (goEnabled == 1)
{
    SDL_Rect dstrect = {500, 350, 200, 150};
    SDL_RenderCopy(renderer, texture_gobutton, NULL, &dstrect);
}

// === Bouton "Connect" pour se connecter au serveur (avant début du jeu) ===
if (connectEnabled == 1)
{
    SDL_Rect dstrect = {0, 0, 200, 50};
    SDL_RenderCopy(renderer, texture_connectbutton, NULL, &dstrect);
}

// === Affichage des noms des joueurs connectés (à gauche) ===
SDL_Color col = {0, 0, 0}; // texte noir
for (i = 0; i < 4; i++)
{
    if (strlen(gNames[i]) > 0)
    {
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, gNames[i], col);
        SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

        SDL_Rect Message_rect;
        Message_rect.x = 10;
        Message_rect.y = 110 + i * 60;
        Message_rect.w = surfaceMessage->w;
        Message_rect.h = surfaceMessage->h;

        SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
        SDL_DestroyTexture(Message);
        SDL_FreeSurface(surfaceMessage);
    }
}

// Rafraîchit l'écran
SDL_RenderPresent(renderer);}
// Libération mémoire des ressources SDL
SDL_DestroyTexture(texture_deck[0]);
SDL_DestroyTexture(texture_deck[1]);
SDL_FreeSurface(deck[0]);
SDL_FreeSurface(deck[1]);
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);

// Fermeture de SDL proprement
SDL_Quit();

return 0;
}
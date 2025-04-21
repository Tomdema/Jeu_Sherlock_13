// === Code commenté pour débutants ===

#include <stdio.h>      // entrées/sorties standard
#include <stdlib.h>     // fonctions utilitaires (malloc, rand, exit...)
#include <string.h>     // manipulation de chaînes de caractères
#include <unistd.h>     // fonctions système POSIX (close, read, write...)
#include <sys/types.h>
#include <sys/socket.h> // pour la gestion des sockets (communication réseau)
#include <netinet/in.h> // structures pour adresses Internet (struct sockaddr_in)
#include <netdb.h>      // fonctions pour résoudre les noms de domaines
#include <arpa/inet.h>  // fonctions de conversion d'adresses IP

// Déclaration d'une structure pour stocker les infos d'un client
struct _client {
    char ipAddress[40]; // Adresse IP du client
    int port;           // Port utilisé par le client
    char name[40];      // Nom du joueur
} tcpClients[4];        // Tableau contenant jusqu'à 4 clients

int nbClients;              // Nombre de clients connectés
int fsmServer;              // État du serveur : 0 = attente, 1 = partie en cours
int deck[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // Jeu de 13 cartes (1 carte par personnage)
int tableCartes[4][8];      // Matrice représentant les objets détenus par chaque joueur

// Liste des noms de personnages
char *nomcartes[] = {
    "Sebastian Moran", "irene Adler", "inspector Lestrade",
    "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
    "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
    "Mrs. Hudson", "Mary Morstan", "James Moriarty"
};

int joueurCourant; // Index du joueur dont c'est le tour

// Fonction d'affichage d'erreur système et arrêt du programme
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Fonction pour mélanger le deck de cartes aléatoirement
void melangerDeck() {
    int i;
    int index1, index2, tmp;
    for (i = 0; i < 1000; i++) {
        index1 = rand() % 13;
        index2 = rand() % 13;
        tmp = deck[index1];
        deck[index1] = deck[index2];
        deck[index2] = tmp;
    }
}

// Crée la table représentant les objets associés à chaque joueur en fonction de ses cartes
void createTable() {
    int i, j, c;

    // Initialisation à zéro de la matrice tableCartes
    for (i = 0; i < 4; i++)
        for (j = 0; j < 8; j++)
            tableCartes[i][j] = 0;

    // Attribution des objets à chaque joueur selon ses 3 cartes
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            c = deck[i * 3 + j]; // Carte c du joueur i
            switch (c) {
                case 0: tableCartes[i][7]++; tableCartes[i][2]++; break;
                case 1: tableCartes[i][7]++; tableCartes[i][1]++; tableCartes[i][5]++; break;
                case 2: tableCartes[i][3]++; tableCartes[i][6]++; tableCartes[i][4]++; break;
                case 3: tableCartes[i][3]++; tableCartes[i][2]++; tableCartes[i][4]++; break;
                case 4: tableCartes[i][3]++; tableCartes[i][1]++; break;
                case 5: tableCartes[i][3]++; tableCartes[i][2]++; break;
                case 6: tableCartes[i][3]++; tableCartes[i][0]++; tableCartes[i][6]++; break;
                case 7: tableCartes[i][0]++; tableCartes[i][1]++; tableCartes[i][2]++; break;
                case 8: tableCartes[i][0]++; tableCartes[i][6]++; tableCartes[i][2]++; break;
                case 9: tableCartes[i][0]++; tableCartes[i][1]++; tableCartes[i][4]++; break;
                case 10: tableCartes[i][0]++; tableCartes[i][5]++; break;
                case 11: tableCartes[i][4]++; tableCartes[i][5]++; break;
                case 12: tableCartes[i][7]++; tableCartes[i][1]++; break;
            }
        }
    }
}

// Affiche le deck mélangé et la matrice des objets des joueurs
void printDeck() {
    int i, j;
    for (i = 0; i < 13; i++)
        printf("%d %s\n", deck[i], nomcartes[deck[i]]);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++)
            printf("%2.2d ", tableCartes[i][j]);
        puts("");
    }
}

// Affiche la liste des clients connectés avec leurs infos
void printClients() {
    int i;
    for (i = 0; i < nbClients; i++)
        printf("%d: %s %5.5d %s\n", i, tcpClients[i].ipAddress,
               tcpClients[i].port, tcpClients[i].name);
}

// Trouve l'index d'un client dans le tableau selon son nom
int findClientByName(char *name) {
    int i;
    for (i = 0; i < nbClients; i++)
        if (strcmp(tcpClients[i].name, name) == 0)
            return i;
    return -1; // Nom non trouvé
}

// Envoie un message à un client en TCP (connexion temporaire)
void sendMessageToClient(char *clientip, int clientport, char *mess) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(clientport);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(1);
    }

    sprintf(buffer, "%s\n", mess); // ajoute un saut de ligne
    n = write(sockfd, buffer, strlen(buffer));
    close(sockfd); // ferme la socket après envoi
}

// Envoie un message à tous les clients connectés
void broadcastMessage(char *mess) {
    int i;
    for (i = 0; i < nbClients; i++)
        sendMessageToClient(tcpClients[i].ipAddress,
                            tcpClients[i].port,
                            mess);
}


// === main commenté ligne par ligne ===
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;              // descripteurs de socket + port
    socklen_t clilen;                           // taille de la structure d'adresse du client
    char buffer[256];                           // buffer pour les messages reçus
    struct sockaddr_in serv_addr, cli_addr;     // adresses serveur et client
    int n, i;                                   // compteurs

    char com;                                   // type de commande (ex : 'C', 'G', etc.)
    char clientIpAddress[256], clientName[256]; // infos du client qui se connecte
    int clientPort;                             // port du client
    int id;                                     // identifiant du client
    char reply[256];                            // buffer de réponse à envoyer

    // Vérifie qu'un port est bien fourni en argument
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Création d'une socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Préparation de la structure d'adresse serveur
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Association de la socket à l'adresse IP et au port
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Mise en écoute de la socket (max 5 connexions en attente)
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Affiche le deck avant et après mélange
    printDeck();
    melangerDeck();
    createTable();
    printDeck();

    joueurCourant = 0; // Le premier joueur est le joueur 0

    // Initialisation des infos clients
    for (i = 0; i < 4; i++) {
        strcpy(tcpClients[i].ipAddress, "localhost");
        tcpClients[i].port = -1;
        strcpy(tcpClients[i].name, "-");
    }

    // Boucle principale : attend les messages des clients
    while (1) {
        // Attend une connexion client
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        // Lecture du message client
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");

        // Affiche le message reçu (debug)
        printf("Received packet from %s:%d\nData: [%s]\n\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

        // Étape 1 : connexion des clients (fsmServer = 0)
        if (fsmServer == 0) {
            switch (buffer[0]) {
                case 'C': // Connexion d'un client
                    sscanf(buffer, "%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                    printf("COM=%c ipAddress=%s port=%d name=%s\n", com, clientIpAddress, clientPort, clientName);

                    // Enregistre le client
                    strcpy(tcpClients[nbClients].ipAddress, clientIpAddress);
                    tcpClients[nbClients].port = clientPort;
                    strcpy(tcpClients[nbClients].name, clientName);
                    nbClients++;

                    printClients();

                    // Trouve son identifiant
                    id = findClientByName(clientName);
                    printf("id=%d\n", id);

                    // Envoie son id au client
                    sprintf(reply, "I %d", id);
                    sendMessageToClient(tcpClients[id].ipAddress, tcpClients[id].port, reply);

                    // Diffuse la liste des joueurs à tous les clients
                    sprintf(reply, "L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
                    broadcastMessage(reply);

                    // Si tous les clients sont connectés, démarrage du jeu
                    if (nbClients == 4) {
                        for (int j = 0; j < 4; j++) {
                            // Envoie les 3 cartes du joueur j
                            sprintf(reply, "D %d %d %d", deck[j * 3], deck[j * 3 + 1], deck[j * 3 + 2]);
                            sendMessageToClient(tcpClients[j].ipAddress, tcpClients[j].port, reply);

                            // Envoie la ligne d'objets pour ce joueur
                            for (int k = 0; k < 8; k++) {
                                sprintf(reply, "V %d %d %d", j, k, tableCartes[j][k]);
                                sendMessageToClient(tcpClients[j].ipAddress, tcpClients[j].port, reply);
                            }
                        }
                        // Envoie le tour du premier joueur (joueur 0)
                        sprintf(reply, "M %d", joueurCourant);
                        broadcastMessage(reply);
                        fsmServer = 1; // Passage à l'étape suivante (jeu en cours)
                    }
                    break;
            }
        }
        // Étape 2 : jeu en cours (fsmServer = 1)
        else if (fsmServer == 1) {
            switch (buffer[0]) {
                case 'G': { // Accusation d'un joueur
                    int id, suspect;
                    sscanf(buffer, "G %d %d", &id, &suspect);
                    if (deck[12] == suspect) {
                        printf("%s a trouvé le coupable !\n", tcpClients[id].name);
                        sprintf(reply, "%s a trouvé le coupable !", tcpClients[id].name);
                        broadcastMessage(reply);
                    } else {
                        printf("%s a donné une mauvaise réponse !\n", tcpClients[id].name);
                        joueurCourant = (joueurCourant + 1) % 4;
                        sprintf(reply, "M %d", joueurCourant);
                        broadcastMessage(reply);
                    }
                    break;
                }
                case 'O': { // Question ouverte sur un objet
                    int id, objet;
                    sscanf(buffer, "O %d %d", &id, &objet);
                    for (int i = 0; i < 4; i++) {
                        if (i != id) {
                            if (tableCartes[i][objet] > 0)
                                sprintf(reply, "V %d %d 100", i, objet);
                            else
                                sprintf(reply, "V %d %d 0", i, objet);
                            broadcastMessage(reply);
                        }
                    }
                    joueurCourant = (joueurCourant + 1) % 4;
                    sprintf(reply, "M %d", joueurCourant);
                    broadcastMessage(reply);
                    break;
                }
                case 'S': { // Question ciblée
                    int id, cible, objet;
                    sscanf(buffer, "S %d %d %d", &id, &cible, &objet);
                    sprintf(reply, "V %d %d %d", cible, objet, tableCartes[cible][objet]);
                    broadcastMessage(reply);
                    joueurCourant = (joueurCourant + 1) % 4;
                    sprintf(reply, "M %d", joueurCourant);
                    broadcastMessage(reply);
                    break;
                }
                default:
                    break;
            }
        }

        close(newsockfd); // ferme la socket client
    }
    close(sockfd); // ferme la socket serveur (jamais atteint ici)
    return 0;
}

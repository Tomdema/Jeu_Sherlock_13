# Jeu_Sherlock_13
Lancer le jeu SH13

Ce projet est une version réseau du jeu Sherlock Holmes 13 (SH13), comportant un serveur central et quatre clients graphiques SDL.
Voici la procédure détaillée pour exécuter le jeu sur macOS.

Installation des dépendances
Avant de compiler le projet, vous devez installer les bibliothèques SDL nécessaires. Ouvrez un terminal et tapez la commande suivante : 
brew install sdl2 sdl2_image sdl2_ttf

Compilation du projet
Le projet est compilé à l’aide d’un Makefile. Pour compiler à la fois le serveur et le client, tapez simplement :
make

Deux exécutables seront générés dans le dossier courant :

server : le programme serveur
client : l’interface graphique SDL pour les joueur

Lancement automatique du serveur et des clients
Un script run_game.sh est fourni pour démarrer automatiquement le serveur et les quatre clients.

Avant de l’exécuter, assurez-vous qu’il est exécutable :
chmod +x run_game.sh

Puis lancez le jeu avec :
./run_game.sh

Le script ouvre cinq terminaux : un pour le serveur et un pour chaque joueur (Tom-Dema, Eyadema, Eya-Tom, Wiyao), avec des ports dédiés.

Réinitialisation du projet
Avant de relancer une session, vous pouvez nettoyer les fichiers précédents avec :
make clean
Puis relancez la compilation et l’exécution.

Informations supplémentaires
Le port par défaut du serveur est 1234.
Les clients utilisent les ports 5001, 5002, 5003 et 5004.
Chaque client doit s'exécuter dans un terminal distinct, car il ouvre une interface graphique indépendante.

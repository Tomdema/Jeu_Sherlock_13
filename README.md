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




Lancer le jeu SH13 (Linux)

Voici la procédure détaillée pour exécuter le jeu sur un système Linux.

Avant de compiler le projet, vous devez installer les bibliothèques SDL nécessaires. Ouvrez un terminal et tapez la commande suivante :
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev.
Cela installera les dépendances requises pour faire fonctionner l’interface graphique du client.

Le projet est compilé à l’aide d’un Makefile. Pour compiler à la fois le serveur et le client, tapez simplement make dans le dossier du projet. Deux exécutables seront alors générés : server pour le serveur central et client pour l’interface graphique des joueurs.

Un script run_game.sh est fourni pour démarrer automatiquement le serveur et les quatre clients. Avant de l’exécuter, assurez-vous qu’il est exécutable en tapant chmod +x run_game.sh, puis lancez le jeu avec ./run_game.sh. Ce script ouvrira cinq terminaux (via gnome-terminal) : un pour le serveur et un pour chaque joueur (Tom-Dema, Eyadema, Eya-Tom, Wiyao), chacun utilisant un port distinct pour communiquer.

Avant de relancer une session de jeu, vous pouvez nettoyer les fichiers de compilation précédents avec la commande make clean, puis relancer la compilation avec make et l’exécution avec ./run_game.sh.

Le port par défaut du serveur est 1234, tandis que les clients utilisent respectivement les ports 5001, 5002, 5003 et 5004. Chaque client doit s’exécuter dans un terminal distinct, car il ouvre une interface graphique indépendante basée sur SDL2. Le projet est compatible avec la plupart des distributions Linux, et peut être adapté à différents environnements de bureau selon le terminal utilisé (gnome-terminal, xterm, etc.).

#!/bin/bash

# Nom du binaire serveur et clients
SERVER_EXEC=./server
CLIENT_EXEC=./client

# Adresse IP locale
IP=127.0.0.1

# Port serveur
SERVER_PORT=1234

# Ports clients (doivent être différents)
CLIENT_PORTS=(5001 5002 5003 5004)

# Noms des joueurs
CLIENT_NAMES=("Alice" "Bob" "Carol" "Dave")

# Se placer dans le dossier du script
cd "$(dirname "$0")"

# Compilation automatique
echo "🔧 Compilation..."
make clean && make

# Vérifie que les binaires existent
if [[ ! -f $SERVER_EXEC || ! -f $CLIENT_EXEC ]]; then
  echo "❌ Compilation échouée. Exécutables introuvables."
  exit 1
fi

# Lancer le serveur dans un nouveau terminal macOS
echo "🚀 Lancement du serveur..."
osascript <<EOF
tell application "Terminal"
    do script "cd \"$(pwd)\" && $SERVER_EXEC $SERVER_PORT"
end tell
EOF

# Petite pause pour laisser le serveur démarrer
sleep 1

# Lancer les 4 clients dans 4 nouveaux terminaux macOS
echo "🎮 Lancement des clients..."
for i in {0..3}; do
  osascript <<EOF
tell application "Terminal"
    do script "cd \"$(pwd)\" && $CLIENT_EXEC $IP $SERVER_PORT $IP ${CLIENT_PORTS[$i]} ${CLIENT_NAMES[$i]}"
end tell
EOF
done

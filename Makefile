# === Fichiers source ===
CLIENT_SRC = sh13.c
SERVER_SRC = server.c

# === Noms des exécutables ===
CLIENT_EXE = client
SERVER_EXE = server

# === Préfixe Homebrew (déjà détecté) ===
BREW_PREFIX := /opt/homebrew

# === Flags de compilation ===
CFLAGS = -I$(BREW_PREFIX)/include -I$(BREW_PREFIX)/opt/sdl2/include
LDFLAGS_CLIENT = -L$(BREW_PREFIX)/lib -L$(BREW_PREFIX)/opt/sdl2/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lpthread
LDFLAGS_SERVER = -lpthread

# === Règle par défaut ===
all: $(CLIENT_EXE) $(SERVER_EXE)

# === Compilation client SDL ===
$(CLIENT_EXE): $(CLIENT_SRC)
	@echo "🔧 Compilation du client SDL ($(CLIENT_SRC))"
	@gcc $(CLIENT_SRC) -o $(CLIENT_EXE) $(CFLAGS) $(LDFLAGS_CLIENT)
	@echo "✅ Client compilé : ./$(CLIENT_EXE)"

# === Compilation serveur ===
$(SERVER_EXE): $(SERVER_SRC)
	@echo "🔧 Compilation du serveur ($(SERVER_SRC))"
	@gcc $(SERVER_SRC) -o $(SERVER_EXE) $(CFLAGS) $(LDFLAGS_SERVER)
	@echo "✅ Serveur compilé : ./$(SERVER_EXE)"

# === Nettoyage ===
clean:
	rm -f $(CLIENT_EXE) $(SERVER_EXE)

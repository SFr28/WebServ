# Webserv – École 42

## Description
Webserv est un projet visant à implémenter un serveur HTTP en C++ 98, capable de gérer plusieurs clients simultanément, de traiter des requêtes HTTP, et de servir des pages web statiques et dynamiques (CGI).

---

## Fonctionnalités
- **Gestion des requêtes HTTP** (GET, POST, DELETE).
- **Parsing des requêtes et réponses HTTP**.
- **Parsing de la configuration** du serveur.
- **Exécution de CGI** pour les pages dynamiques.
- **Gestion des connexions simultanées** avec `epoll()`.
- **Gestion des erreurs HTTP** (404, 500, etc.).

---

## Contributions
- **Saïna Fraslin** : Exécution du serveur et gestion des CGI.
- **Solenne Vincen** @Silver-444 : Parsing des requêtes et réponses HTTP.
- **Lilou Perez** @liperez42 : Parsing de la configuration et création du site de test.

---

## Prérequis
- Système **Linux** ou **macOS**.
- Compilateur **C++** compatible avec le standard C++98.
- `make` installé.

---

## Compilation et exécution
1. Cloner le dépôt :
   ```bash
   git clone <url-du-depot> webserv
   cd webserv
   ```
2. Compiler le projet:
    ```bash
    make
    ```
3. Lancer le projet avec la configuration par défaut:
    ```bash
    ./webserv
    ```

---

## Commandes Makefile
- `make` : Compile le projet.
- `make clean` : Nettoie les fichiers objets.
- `make fclean` : Nettoie tout.
- `make re` : Recompile tout depuis zéro.


